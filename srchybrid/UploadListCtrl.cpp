//this file is part of eMule
//Copyright (C)2002-2006 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "UploadListCtrl.h"
#include "TransferWnd.h"
#include "otherfunctions.h"
#include "MenuCmds.h"
#include "ClientDetailDialog.h"
#include "KademliaWnd.h"
#include "emuledlg.h"
#include "friendlist.h"
#include "MemDC.h"
#include "KnownFile.h"
#include "SharedFileList.h"
#include "UpDownClient.h"
#include "ClientCredits.h"
#include "ChatWnd.h"
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/net/KademliaUDPListener.h"
#include "UploadQueue.h"
#include "ToolTipCtrlX.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


// CUploadListCtrl

IMPLEMENT_DYNAMIC(CUploadListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CUploadListCtrl, CMuleListCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
	ON_NOTIFY_REFLECT(LVN_GETINFOTIP, OnLvnGetInfoTip)
END_MESSAGE_MAP()

CUploadListCtrl::CUploadListCtrl()
	: CListCtrlItemWalk(this)
{
	m_tooltip = new CToolTipCtrlX;
	SetGeneralPurposeFind(true, false);
}

void CUploadListCtrl::Init()
{
	SetName(_T("UploadListCtrl"));
	CImageList ilDummyImageList; //dummy list for getting the proper height of listview entries
	ilDummyImageList.Create(1, theApp.GetSmallSytemIconSize().cy,theApp.m_iDfltImageListColorFlags|ILC_MASK, 1, 1); 
	SetImageList(&ilDummyImageList, LVSIL_SMALL);
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	ilDummyImageList.Detach();

	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	CToolTipCtrl* tooltip = GetToolTips();
	if (tooltip){
		m_tooltip->SubclassWindow(tooltip->m_hWnd);
		tooltip->ModifyStyle(0, TTS_NOPREFIX);
		tooltip->SetDelayTime(TTDT_AUTOPOP, 20000);
		tooltip->SetDelayTime(TTDT_INITIAL, thePrefs.GetToolTipDelay()*1000);
	}

	InsertColumn(0,GetResString(IDS_QL_USERNAME),LVCFMT_LEFT,150,0);
	InsertColumn(1,GetResString(IDS_FILE),LVCFMT_LEFT,275,1);
	InsertColumn(2,GetResString(IDS_DL_SPEED),LVCFMT_LEFT,60,2);
	InsertColumn(3,GetResString(IDS_DL_TRANSF),LVCFMT_LEFT,65,3);
	InsertColumn(4,GetResString(IDS_WAITED),LVCFMT_LEFT,60,4);
	InsertColumn(5,GetResString(IDS_UPLOADTIME),LVCFMT_LEFT,60,5);
	InsertColumn(6,GetResString(IDS_STATUS),LVCFMT_LEFT,110,6);
	InsertColumn(7,GetResString(IDS_UPSTATUS),LVCFMT_LEFT,100,7);
	//MORPH START - Added by SiRoB, Client Software
	InsertColumn(8,GetResString(IDS_CD_CSOFT),LVCFMT_LEFT,100,8);
	//MORPH END - Added by SiRoB, Client Software
	InsertColumn(9,GetResString(IDS_UPL_DL),LVCFMT_LEFT,100,9); //Total up down
	InsertColumn(10,GetResString(IDS_CL_DOWNLSTATUS),LVCFMT_LEFT,100,10); //Yun.SF3 Remote Queue Status
	//MORPH START - Added by SiRoB, ZZ Upload System 20030724-0336
	InsertColumn(11,GetResString(IDS_UPSLOTNUMBER),LVCFMT_LEFT,100,11);
	//MORPH END - Added by SiRoB, ZZ Upload System 20030724-0336
	//MORPH START - Added by SiRoB, Show Compression by Tarod
	InsertColumn(12,GetResString(IDS_COMPRESSIONGAIN),LVCFMT_LEFT,50,12);
	//MORPH END - Added by SiRoB, Show Compression by Tarod

	// Mighty Knife: Community affiliation
	InsertColumn(13,GetResString(IDS_COMMUNITY),LVCFMT_LEFT,100,13);
	// [end] Mighty Knife

	// EastShare - Added by Pretender, Friend Tab
	InsertColumn(14,GetResString(IDS_FRIENDLIST),LVCFMT_LEFT,75,14);
	// EastShare - Added by Pretender, Friend Tab

    // Commander - Added: IP2Country column - Start
	InsertColumn(15,GetResString(IDS_COUNTRY),LVCFMT_LEFT,100,15);
    // Commander - Added: IP2Country column - End
	
	//MORPH START - Added by SiRoB, Display current uploading chunk
	InsertColumn(16,GetResString(IDS_CHUNK),LVCFMT_LEFT,100,16);
    //MORPH END   - Added by SiRoB, Display current uploading chunk
		
	SetAllIcons();
	Localize();
	LoadSettings();

	// Barry - Use preferred sort order from preferences
	SetSortArrow();
	SortItems(SortProc, GetSortItem() + (GetSortAscending() ? 0:100));

	// Mighty Knife: Community affiliation
	if (thePrefs.IsCommunityEnabled ()) ;// ShowColumn (13); //Removed by SiRoB, some people may prefere disable it
	else HideColumn (13);
	// [end] Mighty Knife

	// Commander - Added: IP2Country column - Start
	if (thePrefs.GetIP2CountryNameMode() == IP2CountryName_DISABLE)
		HideColumn (15);
	// Commander - Added: IP2Country column - End
}

CUploadListCtrl::~CUploadListCtrl()
{
	delete m_tooltip;
}

void CUploadListCtrl::OnSysColorChange()
{
	CMuleListCtrl::OnSysColorChange();
	SetAllIcons();
}

void CUploadListCtrl::SetAllIcons()
{
	imagelist.DeleteImageList();
	imagelist.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	imagelist.SetBkColor(CLR_NONE);
	imagelist.Add(CTempIconLoader(_T("ClientEDonkey")));
	imagelist.Add(CTempIconLoader(_T("ClientCompatible")));
	//MORPH START - Modified by SiRoB, More client & Credit overlay icon
	//imagelist.Add(CTempIconLoader(_T("ClientEDonkeyPlus")));
	//imagelist.Add(CTempIconLoader(_T("ClientCompatiblePlus")));
	imagelist.Add(CTempIconLoader(_T("Friend")));
	imagelist.Add(CTempIconLoader(_T("ClientMLDonkey")));
	//imagelist.Add(CTempIconLoader(_T("ClientMLDonkeyPlus")));
	imagelist.Add(CTempIconLoader(_T("ClientEDonkeyHybrid")));
	//imagelist.Add(CTempIconLoader(_T("ClientEDonkeyHybridPlus")));
	imagelist.Add(CTempIconLoader(_T("ClientShareaza")));
	//imagelist.Add(CTempIconLoader(_T("ClientShareazaPlus")));
	imagelist.Add(CTempIconLoader(_T("ClientAMule")));
	//imagelist.Add(CTempIconLoader(_T("ClientAMulePlus")));
	imagelist.Add(CTempIconLoader(_T("ClientLPhant")));
	//imagelist.Add(CTempIconLoader(_T("ClientLPhantPlus")));
	imagelist.Add(CTempIconLoader(_T("ClientRightEdonkey")));
	imagelist.Add(CTempIconLoader(_T("Morph")));
	imagelist.Add(CTempIconLoader(_T("SCARANGEL")));
	imagelist.Add(CTempIconLoader(_T("STULLE")));
	imagelist.Add(CTempIconLoader(_T("MAXMOD")));
	imagelist.Add(CTempIconLoader(_T("XTREME")));
	imagelist.Add(CTempIconLoader(_T("EASTSHARE")));
	imagelist.Add(CTempIconLoader(_T("IONIX")));
	imagelist.Add(CTempIconLoader(_T("CYREX")));
	imagelist.Add(CTempIconLoader(_T("NEXTEMF")));
	imagelist.Add(CTempIconLoader(_T("NEO")));
	imagelist.SetOverlayImage(imagelist.Add(CTempIconLoader(_T("ClientSecureOvl"))), 1);
	imagelist.SetOverlayImage(imagelist.Add(CTempIconLoader(_T("ClientCreditOvl"))), 2);
	imagelist.SetOverlayImage(imagelist.Add(CTempIconLoader(_T("ClientCreditSecureOvl"))), 3);//10
	//MORPH END   - Modified by SiRoB, More client & Credit overlay icon

	// Mighty Knife: Community icon
	m_overlayimages.DeleteImageList();
	m_overlayimages.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	m_overlayimages.SetBkColor(CLR_NONE);
	m_overlayimages.Add(CTempIconLoader(_T("Community")));
	// [end] Mighty Knife
	//MORPH START - Addded by SiRoB, Friend Addon
	m_overlayimages.Add(CTempIconLoader(_T("ClientFriendOvl")));
	m_overlayimages.Add(CTempIconLoader(_T("ClientFriendSlotOvl")));
	//MORPH END   - Addded by SiRoB, Friend Addon
}

void CUploadListCtrl::Localize()
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;

	if(pHeaderCtrl->GetItemCount() != 0) {
		CString strRes;
		strRes = GetResString(IDS_QL_USERNAME);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(0, &hdi);

		strRes = GetResString(IDS_FILE);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(1, &hdi);

		strRes = GetResString(IDS_DL_SPEED);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(2, &hdi);
	
		strRes = GetResString(IDS_DL_TRANSF);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(3, &hdi);

		strRes = GetResString(IDS_WAITED);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(4, &hdi);

		strRes = GetResString(IDS_UPLOADTIME);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(5, &hdi);

		strRes = GetResString(IDS_STATUS);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(6, &hdi);

		strRes = GetResString(IDS_UPSTATUS);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(7, &hdi);
		
		//MORPH START - Modified by SiRoB, Client Software
		strRes = GetResString(IDS_CD_CSOFT);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(8, &hdi);
		//MORPH END - Modified by SiRoB, Client Software

		//MORPH START - Modified by IceCream, Total up down
		strRes = GetResString(IDS_UPL_DL);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(9, &hdi);
		//MORPH END - Modified by IceCream, Total up down

		//MORPH START - Modified by IceCream, Remote Status
		strRes = GetResString(IDS_CL_DOWNLSTATUS);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(10, &hdi);
		//MORPH END - Modified by IceCream, Remote Status

		//MORPH START - Added by SiRoB, ZZ Missing
		strRes = GetResString(IDS_UPSLOTNUMBER);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(11, &hdi);
		//MORPH END - Added by SiRoB, ZZ Missing

		//MORPH START - Added by SiRoB, Show Compression by Tarod
		strRes = GetResString(IDS_COMPRESSIONGAIN);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(12, &hdi);
		//MORPH END - Added by SiRoB, Show Compression by Tarod

		// Mighty Knife: Community affiliation
		strRes = GetResString(IDS_COMMUNITY);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(13, &hdi);
		// [end] Mighty Knife

		// EastShare - Added by Pretender, Friend Tab
		strRes = GetResString(IDS_FRIENDLIST);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(14, &hdi);
		// EastShare - Added by Pretender, Friend Tab

		// Commander - Added: IP2Country column - Start
		strRes = GetResString(IDS_COUNTRY);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(15, &hdi);
		// Commander - Added: IP2Country column - End
		
		//MORPH START - Added by SiRoB, Display current uploading chunk
		strRes = GetResString(IDS_CHUNK);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(16, &hdi);
		//MORPH START - Added by SiRoB, Display current uploading chunk
	}
}

void CUploadListCtrl::AddClient(const CUpDownClient* client)
{
	if (!theApp.emuledlg->IsRunning())
		return;

	int iItemCount = GetItemCount();
	int iItem = InsertItem(LVIF_TEXT|LVIF_PARAM,iItemCount,LPSTR_TEXTCALLBACK,0,0,0,(LPARAM)client);
	Update(iItem);
	theApp.emuledlg->transferwnd->UpdateListCount(CTransferWnd::wnd2Uploading, iItemCount+1);
}

void CUploadListCtrl::RemoveClient(const CUpDownClient* client)
{
	if (!theApp.emuledlg->IsRunning())
		return;

	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	int result = FindItem(&find);
	if (result != -1){
		DeleteItem(result);
		theApp.emuledlg->transferwnd->UpdateListCount(CTransferWnd::wnd2Uploading);
	}
}

void CUploadListCtrl::RefreshClient(const CUpDownClient* client)
{
	// There is some type of timing issue here.. If you click on item in the queue or upload and leave
	// the focus on it when you exit the cient, it breaks on line 854 of emuleDlg.cpp
	// I added this IsRunning() check to this function and the DrawItem method and
	// this seems to keep it from crashing. This is not the fix but a patch until
	// someone points out what is going wrong.. Also, it will still assert in debug mode..
	if( !theApp.emuledlg->IsRunning())
		return;
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	int result = FindItem(&find);
	if(result != -1)
		Update(result);
}

#define DLC_DT_TEXT (DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)

void CUploadListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if( !theApp.emuledlg->IsRunning() )
		return;
	if (!lpDrawItemStruct->itemData)
		return;
	//MORPH START - Added by SiRoB, Don't draw hidden Rect
	RECT clientRect;
	GetClientRect(&clientRect);
	CRect cur_rec(lpDrawItemStruct->rcItem);
	if (cur_rec.top >= clientRect.bottom || cur_rec.bottom <= clientRect.top)
		return;
	//MORPH END   - Added by SiRoB, Don't draw hidden Rect
	CDC* odc = CDC::FromHandle(lpDrawItemStruct->hDC);
	BOOL bCtrlFocused = ((GetFocus() == this ) || (GetStyle() & LVS_SHOWSELALWAYS));
	if (lpDrawItemStruct->itemState & ODS_SELECTED) {
		if(bCtrlFocused)
			odc->SetBkColor(m_crHighlight);
		else
			odc->SetBkColor(m_crNoHighlight);
	}
	else
		odc->SetBkColor(GetBkColor());
	const CUpDownClient* client = (CUpDownClient*)lpDrawItemStruct->itemData;
	CMemDC dc(odc, &lpDrawItemStruct->rcItem);
	CFont *pOldFont = dc.SelectObject(GetFont());
	//MORPH - Moved by SiRoB, Don't draw hidden Rect
	/*
	CRect cur_rec(lpDrawItemStruct->rcItem);
	*/
	COLORREF crOldTextColor = dc.SetTextColor((lpDrawItemStruct->itemState & ODS_SELECTED) ? m_crHighlightText : m_crWindowText);

	if(client->IsScheduledForRemoval()) {
		dc.SetTextColor(RGB(255,50,50));
	} else if(client->GetSlotNumber() > theApp.uploadqueue->GetActiveUploadsCount(client->GetClassID())) { //MORPH - Upload Splitting Class
        dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
    }

	int iOldBkMode;
	if (m_crWindowTextBk == CLR_NONE){
		DefWindowProc(WM_ERASEBKGND, (WPARAM)(HDC)dc, 0);
		iOldBkMode = dc.SetBkMode(TRANSPARENT);
	}
	else
		iOldBkMode = OPAQUE;

	//MORPH START - Adde by SiRoB, Optimization requpfile
	/*
	CKnownFile* file = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
	*/
	CKnownFile* file = client->CheckAndGetReqUpFile();
	//MORPH END - Adde by SiRoB, Optimization requpfile
	CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
	int iCount = pHeaderCtrl->GetItemCount();
	cur_rec.right = cur_rec.left - 8;
	cur_rec.left += 4;
	CString Sbuffer;
	for(int iCurrent = 0; iCurrent < iCount; iCurrent++)
	{
		int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);
		if (!IsColumnHidden(iColumn))
		{
			cur_rec.right += GetColumnWidth(iColumn);
			//MORPH START - Added by SiRoB, Don't draw hidden columns
			if (cur_rec.left < clientRect.right && cur_rec.right > clientRect.left)
			{
			//MORPH END   - Added by SiRoB, Don't draw hidden columns
				UINT dcdttext = DLC_DT_TEXT; //MORPH - Added by SiRoB, Justify Text Option
				switch(iColumn)
				{
					case 0:{
						//MORPH START - Modified by SiRoB, More client & Credit overlay icon
						uint8 image;
						//MORPH - Removed by SiRoB, Friend Addon
						/*
						if (client->IsFriend())
							image = 2;
						else*/ if (client->GetClientSoft() == SO_MLDONKEY )
							image = 3;
						else if (client->GetClientSoft() == SO_EDONKEYHYBRID )
							image = 4;
						else if (client->GetClientSoft() == SO_SHAREAZA )
							image = 5;
						else if (client->GetClientSoft() == SO_AMULE)
							image = 6;
						else if (client->GetClientSoft() == SO_LPHANT)
							image = 7;
						else if (client->GetClientSoft() == SO_EDONKEY )
							image = 8;
						else if (client->ExtProtocolAvailable())
						//MORPH START - Added by SiRoB, More client icon
						{
							if(client->GetModClient() == MOD_NONE)
								image = 1;
							else
								image = (uint8)(client->GetModClient() + 8);
						}
						//MORPH END   - Added by SiRoB, More client icon
						else
							image = 0;

						POINT point = {cur_rec.left, cur_rec.top+1};
						//MORPH START - Added by Stulle, fix score display
						/*
						UINT uOvlImg = INDEXTOOVERLAYMASK(((client->Credits() && client->Credits()->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED) ? 1 : 0) | ((client->credits->GetScoreRatio(client->GetIP()) > 1) ? 2 : 0));
						*/
						UINT uOvlImg = INDEXTOOVERLAYMASK(((client->Credits() && client->Credits()->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED) ? 1 : 0) | ((client->credits->GetHasScore(client->GetIP())) ? 2 : 0));
						//MORPH END - Added by Stulle, fix score display
						if (client->IsLeecher())
							imagelist.DrawIndirect(dc,image, point, CSize(16,16), CPoint(0,0), ILD_NORMAL | uOvlImg, 0, RGB(255,64,64));
						else
							imagelist.DrawIndirect(dc,image, point, CSize(16,16), CPoint(0,0), ILD_NORMAL | uOvlImg, 0, odc->GetBkColor());
						//MORPH END   - Modified by SiRoB, More client & Credit overlay icon

						// Mighty Knife: Community visualization
						if (client->IsCommunity())
							m_overlayimages.Draw(dc,0, point, ILD_TRANSPARENT);
						// [end] Mighty Knife

						//MORPH START - Added by SiRoB, Friend Addon
						if (client->IsFriend())
							m_overlayimages.Draw(dc,client->GetFriendSlot()?2:1, point, ILD_TRANSPARENT);
						//MORPH END   - Added by SiRoB, Friend Addon
							
						Sbuffer = client->GetUserName();

						//EastShare Start - added by AndCycle, IP to Country, modified by Commander
						if(theApp.ip2country->ShowCountryFlag() && IsColumnHidden(15)){
							cur_rec.left+=20;
							POINT point2= {cur_rec.left,cur_rec.top+1};
							theApp.ip2country->GetFlagImageList()->DrawIndirect(dc, client->GetCountryFlagIndex(), point2, CSize(18,16), CPoint(0,0), ILD_NORMAL);
						}
						//EastShare End - added by AndCycle, IP to Country

						cur_rec.left +=20;
						dc.DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
						cur_rec.left -=20;

						//EastShare Start - added by AndCycle, IP to Country
						if(theApp.ip2country->ShowCountryFlag() && IsColumnHidden(15)){
							cur_rec.left-=20;
						}
						//EastShare End - added by AndCycle, IP to Country

						break;
					}
					case 1:
						if(file){
							Sbuffer = file->GetFileName();

							//Morph Start - added by AndCycle, Equal Chance For Each File
							//Morph - added by AndCycle, more detail...for debug?
							if(thePrefs.IsEqualChanceEnable()){
								Sbuffer.Format(_T("%s :%s"), file->statistic.GetEqualChanceValueString(false), Sbuffer);
							}
							//Morph - added by AndCycle, more detail...for debug?
							//Morph End - added by AndCycle, Equal Chance For Each File
						}
						else
							Sbuffer = _T("?");
						break;
					case 2:
						Sbuffer = CastItoXBytes(client->GetDatarate(), false, true);
						//MORPH START - Added by SiRoB, Right Justify
						dcdttext |= DT_RIGHT;
						//MORPH END   - Added by SiRoB, Right Justify
						break;
					case 3:
						// this would be the logical correct data item to show here
						//Sbuffer = CastItoXBytes(client->GetTransferredUp(), false, false);
						//Morph - modified by AndCycle, more uploading session info to show full chunk transfer
						if(client->GetSessionUp() == client->GetQueueSessionUp()) {
							Sbuffer.Format(_T("%s (%s)"), CastItoXBytes(client->GetQueueSessionPayloadUp(), false, false), CastItoXBytes(client->GetQueueSessionUp(), false, false));
						} else {
							Sbuffer.Format(_T("%s=%s+%s (%s=%s+%s)"), CastItoXBytes(client->GetQueueSessionPayloadUp()), CastItoXBytes(client->GetSessionPayloadUp()), CastItoXBytes(client->GetQueueSessionPayloadUp()-client->GetSessionPayloadUp()), CastItoXBytes(client->GetQueueSessionUp()), CastItoXBytes(client->GetSessionUp()), CastItoXBytes(client->GetQueueSessionUp()-client->GetSessionUp()));
						}
						//Morph - modified by AndCycle, more uploading session info to show full chunk transfer
						break;
					case 4:
						if (client->HasLowID())
							if(client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick)
								Sbuffer.Format(GetResString(IDS_UP_LOWID_DELAYED),CastSecondsToHM(client->GetWaitTime()/1000), CastSecondsToHM((::GetTickCount()-client->GetUpStartTimeDelay()-client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick)/1000));
							else
								Sbuffer.Format(GetResString(IDS_UP_LOWID),CastSecondsToHM(client->GetWaitTime()/1000));
						else
							Sbuffer = CastSecondsToHM(client->GetWaitTime()/1000);
						break;
					case 5:
						{//Morph - modified by AndCycle, upRemain
							sint32 timeleft = -1;
							uint32 UpDatarate = client->GetDatarate();
							// Mighty Knife: Check for credits!=NULL
							if ((UpDatarate == 0) || (client->Credits()==NULL)) timeleft = -1;
							else if(file)
								if(client->IsMoreUpThanDown(file) && client->GetQueueSessionUp() > SESSIONMAXTRANS)	timeleft = (sint32)((client->Credits()->GetDownloadedTotal() - client->Credits()->GetUploadedTotal())/UpDatarate);
							// [end] Mighty Knife
								else if(client->GetPowerShared(file) && client->GetQueueSessionUp() > SESSIONMAXTRANS) timeleft = -1; //(float)(file->GetFileSize() - client->GetQueueSessionUp())/UpDatarate;
								else if (file->GetFileSize() > (uint64)SESSIONMAXTRANS)	timeleft = (sint32)((SESSIONMAXTRANS - client->GetQueueSessionUp())/UpDatarate);
								else timeleft = (sint32)(((uint64)file->GetFileSize() - client->GetQueueSessionUp())/UpDatarate);
							Sbuffer.Format(_T("%s (+%s)"), CastSecondsToHM((client->GetUpStartTimeDelay())/1000), (timeleft>=0)?CastSecondsToHM(timeleft):_T("?"));
						}//Morph - modified by AndCycle, upRemain
						break;
					case 6:
						Sbuffer = client->GetUploadStateDisplayString();
						break;
					case 7:
							cur_rec.bottom--;
							cur_rec.top++;
							client->DrawUpStatusBar(dc,&cur_rec,false,thePrefs.UseFlatBar());
							cur_rec.bottom++;
							cur_rec.top--;
						break;
					//MORPH START - Added by SiRoB, Client Software
					case 8:			
						Sbuffer = client->GetClientSoftVer();
						break;
					//MORPH END - Added by SiRoB, Client Software
				
					//MORPH START - Added By Yun.SF3, Upload/Download
					case 9: //LSD Total UP/DL
						{
							if (client->Credits()){
								Sbuffer.Format( _T("%s/%s"),
									CastItoXBytes(client->Credits()->GetUploadedTotal(),false,false),
									CastItoXBytes(client->Credits()->GetDownloadedTotal(),false,false));
								//(float)client->Credits()->GetScoreRatio() );

							}
							else{
								Sbuffer.Format( _T("%s/%s"),// R%s",
									_T("?"),_T("?"));//,"?" );
							}
							break;	

						}
					//MORPH END - Added By Yun.SF3, Upload/Download

					//MORPH START - Added By Yun.SF3, Remote Status
					case 10: //Yun.SF3 remote queue status
						{	
							int qr = client->GetRemoteQueueRank();
							if (client->GetDownloadDatarate() > 0){
								Sbuffer.Format(_T("%s"),CastItoXBytes(client->GetDownloadDatarate(),false,true));
							}
							else if (qr)
									Sbuffer.Format(_T("QR: %u"),qr);
							
							else if(client->IsRemoteQueueFull())
								Sbuffer.Format(_T("%s"),GetResString(IDS_QUEUEFULL));
							else
								Sbuffer.Format(_T("%s"),GetResString(IDS_UNKNOWN));
						
						}
						break;	
						//MORPH END - Added By Yun.SF3, Remote Status

					case 11:{
						Sbuffer.Format(_T("%i"), client->GetSlotNumber());
						//MORPH START - Added by SiRoB, Upload Bandwidth Splited by class
						if (client->GetClassID()==0){
							Sbuffer.Append(_T(" FS"));
						} else
						//Morph - modified by AndCycle, take PayBackFirst have same class with PowerShare
						if (file && client->GetClassID()==1){
							if (client->IsMoreUpThanDown(file))
							{
								Sbuffer.Append(_T(",PBF"));
								if (client->Credits() && client->Credits()->GetDownloadedTotal() > client->Credits()->GetUploadedTotal())
									Sbuffer.AppendFormat( _T("(%s)"),
									CastItoXBytes((float)client->Credits()->GetDownloadedTotal()-
												(float)client->Credits()->GetUploadedTotal(),false,false));
							}
							if (client->GetPowerShared(file))
								Sbuffer.Append(_T(",PS"));

							CString tempFilePrio;
							switch (file->GetUpPriority()) {
								case PR_VERYLOW : {
									tempFilePrio = GetResString(IDS_PRIOVERYLOW);
									break; }
								case PR_LOW : {
									if( file->IsAutoUpPriority() )
										tempFilePrio = GetResString(IDS_PRIOAUTOLOW);
									else
										tempFilePrio = GetResString(IDS_PRIOLOW);
									break; }
								case PR_NORMAL : {
									if( file->IsAutoUpPriority() )
										tempFilePrio = GetResString(IDS_PRIOAUTONORMAL);
									else
										tempFilePrio = GetResString(IDS_PRIONORMAL);
									break; }
								case PR_HIGH : {
									if( file->IsAutoUpPriority() )
										tempFilePrio = GetResString(IDS_PRIOAUTOHIGH);
									else
										tempFilePrio = GetResString(IDS_PRIOHIGH);
									break; }
								case PR_VERYHIGH : {
									tempFilePrio = GetResString(IDS_PRIORELEASE);
									break; }
								default:
									tempFilePrio.Empty();
							}
							Sbuffer.Append(_T(",") + tempFilePrio);
						}
						//MORPH END   - Added by SiRoB, Upload Bandwidth Splited by class
					}break;
					//MORPH START - Added by SiRoB, Show Compression by Tarod
					case 12:
						if (client->GetCompression() < 0.1f)
							Sbuffer = _T("-");
						else
							Sbuffer.Format(_T("%.1f%%"), client->GetCompression());
						break;
					//MORPH END - Added by SiRoB, Show Compression by Tarod

					// Mighty Knife: Community affiliation
					case 13:
						Sbuffer = client->IsCommunity () ? GetResString(IDS_YES) : _T("");
						break;
					// [end] Mighty Knife
					// EastShare - Added by Pretender, Friend Tab
					case 14:
						Sbuffer = client->IsFriend () ? GetResString(IDS_YES) : _T("");
						break;
					// EastShare - Added by Pretender, Friend Tab
					// Commander - Added: IP2Country column - Start
					case 15:
						Sbuffer.Format(_T("%s"), client->GetCountryName());
						if(theApp.ip2country->ShowCountryFlag()){
							POINT point2= {cur_rec.left,cur_rec.top+1};
							theApp.ip2country->GetFlagImageList()->DrawIndirect(dc, client->GetCountryFlagIndex(), point2, CSize(18,16), CPoint(0,0), ILD_NORMAL);
							cur_rec.left+=20;
						}
						dc.DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
						if(theApp.ip2country->ShowCountryFlag()){
							cur_rec.left-=20;
						}
						break;
					// Commander - Added: IP2Country column - End
					//MORPH START - Added by SiRoB, Display current uploading chunk
					case 16:
							cur_rec.bottom--;
							cur_rec.top++;
							client->DrawUpStatusBarChunk(dc,&cur_rec,false,thePrefs.UseFlatBar());
							cur_rec.bottom++;
							cur_rec.top--;
						break;
					//MORPH END   - Added by SiRoB, Display current uploading chunk
				}
				if( iColumn != 7 && iColumn != 0 && iColumn != 15 && iColumn != 16)
					dc.DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,dcdttext);
			} //MORPH - Added by SiRoB, Don't draw hidden columns
			cur_rec.left += GetColumnWidth(iColumn);
		}
	}

	//draw rectangle around selected item(s)
	if ((lpDrawItemStruct->itemAction | ODA_SELECT) && (lpDrawItemStruct->itemState & ODS_SELECTED))
	{
		RECT outline_rec = lpDrawItemStruct->rcItem;

		outline_rec.top--;
		outline_rec.bottom++;
		dc.FrameRect(&outline_rec, &CBrush(GetBkColor()));
		outline_rec.top++;
		outline_rec.bottom--;
		outline_rec.left++;
		outline_rec.right--;

		if(bCtrlFocused)
			dc.FrameRect(&outline_rec, &CBrush(m_crFocusLine));
		else
			dc.FrameRect(&outline_rec, &CBrush(m_crNoFocusLine));
	}

	if (m_crWindowTextBk == CLR_NONE)
		dc.SetBkMode(iOldBkMode);
	dc.SelectObject(pOldFont);
	dc.SetTextColor(crOldTextColor);
}

void CUploadListCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	const CUpDownClient* client = (iSel != -1) ? (CUpDownClient*)GetItemData(iSel) : NULL;

	CTitleMenu ClientMenu;
	ClientMenu.CreatePopupMenu();
	ClientMenu.AddMenuTitle(GetResString(IDS_CLIENTS), true);
	ClientMenu.AppendMenu(MF_STRING | (client ? MF_ENABLED : MF_GRAYED), MP_DETAIL, GetResString(IDS_SHOWDETAILS), _T("CLIENTDETAILS"));
	ClientMenu.SetDefaultItem(MP_DETAIL);
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && !client->IsFriend()) ? MF_ENABLED : MF_GRAYED), MP_ADDFRIEND, GetResString(IDS_ADDFRIEND), _T("ADDFRIEND"));
	//MORPH START - Added by SiRoB, Friend Addon
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->IsFriend()) ? MF_ENABLED : MF_GRAYED), MP_REMOVEFRIEND, GetResString(IDS_REMOVEFRIEND), _T("DELETEFRIEND"));
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->IsFriend()) ? MF_ENABLED  | ((!client->HasLowID() && client->IsFriend() && client->GetFriendSlot())?MF_CHECKED : MF_UNCHECKED) : MF_GRAYED), MP_FRIENDSLOT, GetResString(IDS_FRIENDSLOT), _T("FRIENDSLOT"));
	//MORPH END - Added by SiRoB, Friend Addon
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient()) ? MF_ENABLED : MF_GRAYED), MP_MESSAGE, GetResString(IDS_SEND_MSG), _T("SENDMESSAGE"));
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetViewSharedFilesSupport()) ? MF_ENABLED : MF_GRAYED), MP_SHOWLIST, GetResString(IDS_VIEWFILES), _T("VIEWFILES"));
	if (Kademlia::CKademlia::IsRunning() && !Kademlia::CKademlia::IsConnected())
		ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetKadPort()!=0) ? MF_ENABLED : MF_GRAYED), MP_BOOT, GetResString(IDS_BOOTSTRAP));
	ClientMenu.AppendMenu(MF_STRING | (GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED), MP_FIND, GetResString(IDS_FIND), _T("Search"));
	
	//MORPH START - Added by Yun.SF3, List Requested Files
	ClientMenu.AppendMenu(MF_SEPARATOR); // Added by sivka
	ClientMenu.AppendMenu(MF_STRING | (client ? MF_ENABLED : MF_GRAYED),MP_LIST_REQUESTED_FILES, GetResString(IDS_LISTREQUESTED), _T("FILEREQUESTED")); // Added by sivka
	//MORPH END - Added by Yun.SF3, List Requested Files
	GetPopupMenuPos(*this, point);
	ClientMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
}

BOOL CUploadListCtrl::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{
	wParam = LOWORD(wParam);

	switch (wParam)
	{
		case MP_FIND:
			OnFindStart();
			return TRUE;
	}

	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1){
		CUpDownClient* client = (CUpDownClient*)GetItemData(iSel);
		switch (wParam){
			case MP_SHOWLIST:
				client->RequestSharedFileList();
				break;
			case MP_MESSAGE:
				theApp.emuledlg->chatwnd->StartSession(client);
				break;
			case MP_ADDFRIEND:
				if (theApp.friendlist->AddFriend(client))
					Update(iSel);
				break;
			case MP_DETAIL:
			case MPG_ALTENTER:
			case IDA_ENTER:
			{
				CClientDetailDialog dialog(client, this);
				dialog.DoModal();
				break;
			}
			case MP_BOOT:
				if (client->GetKadPort())
					Kademlia::CKademlia::Bootstrap(ntohl(client->GetIP()), client->GetKadPort());
				break;
			//MORPH START - Addded by SiRoB, Friend Addon
			case MP_REMOVEFRIEND:{//LSD
				if (client && client->IsFriend())
				{
					theApp.friendlist->RemoveFriend(client->m_Friend);
					RefreshClient(client);
				}
				break;
			}
			case MP_FRIENDSLOT:{
				//MORPH START - Modified by SIRoB, Added by Yun.SF3, ZZ Upload System
				if (client){
					bool IsAlready;
					IsAlready = client->GetFriendSlot();
					//theApp.friendlist->RemoveAllFriendSlots();
					if( IsAlready ) {
						client->SetFriendSlot(false);
					} else {
						client->SetFriendSlot(true);
					}
					theApp.friendlist->ShowFriends();
					RefreshClient(client);
				}
				//MORPH END - Modified by SIRoB, Added by Yun.SF3, ZZ Upload System
				break;
			}
			//Xman end
			//MORPH END   - Added by SiRoB, Friend Addon
			//MORPH START - Added by Yun.SF3, List Requested Files
			case MP_LIST_REQUESTED_FILES: { // added by sivka
				if (client != NULL)
				{
					client->ShowRequestedFiles(); //Changed by SiRoB
				}
				break;
			}
			//MORPH END - Added by Yun.SF3, List Requested Files
		}
	}
	return true;
}


void CUploadListCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult){

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// if it's a second click on the same column then reverse the sort order,
	// otherwise sort the new column in ascending order.

	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	bool sortAscending = (GetSortItem() != pNMListView->iSubItem) ? true : !GetSortAscending();

	// Sort table
	UpdateSortHistory(pNMListView->iSubItem + (sortAscending ? 0:100));
	SetSortArrow(pNMListView->iSubItem, sortAscending);
	SortItems(SortProc, pNMListView->iSubItem + (sortAscending ? 0:100));

	*pResult = 0;
}

int CUploadListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const CUpDownClient* item1 = (CUpDownClient*)lParam1;
	const CUpDownClient* item2 = (CUpDownClient*)lParam2;

	int iResult=0;
	switch(lParamSort){
		case 0: 
			if(item1->GetUserName() && item2->GetUserName())
				iResult=CompareLocaleStringNoCase(item1->GetUserName(), item2->GetUserName());
			else if(item1->GetUserName())
				iResult=1;
			else
				iResult=-1;
			break;
		case 100:
			if(item1->GetUserName() && item2->GetUserName())
				iResult=CompareLocaleStringNoCase(item2->GetUserName(), item1->GetUserName());
			else if(item2->GetUserName())
				iResult=1;
			else
				iResult=-1;
			break;
		case 1: {
			//MORPH START - Adde by SiRoB, Optimization requpfile
			/*
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			*/
			CKnownFile* file1 = item1->CheckAndGetReqUpFile();
			CKnownFile* file2 = item2->CheckAndGetReqUpFile();
			//MORPH END   - Adde by SiRoB, Optimization requpfile
			if( (file1 != NULL) && (file2 != NULL))
				iResult=CompareLocaleStringNoCase(file1->GetFileName(), file2->GetFileName());
			else if( file1 == NULL )
				iResult=1;
			else
				iResult=-1;
			break;
		}
		case 101:{
			//MORPH START - Adde by SiRoB, Optimization requpfile
			/*
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			*/
			CKnownFile* file1 = item1->CheckAndGetReqUpFile();
			CKnownFile* file2 = item2->CheckAndGetReqUpFile();
			//MORPH END   - Adde by SiRoB, Optimization requpfile
			if( (file1 != NULL) && (file2 != NULL))
				iResult=CompareLocaleStringNoCase(file2->GetFileName(), file1->GetFileName());
			else if( file1 == NULL )
				iResult=1;
			else
				iResult=-1;
			break;
		}
		case 2: 
			iResult=CompareUnsigned(item1->GetDatarate(), item2->GetDatarate());
			break;
		case 102:
			iResult=CompareUnsigned(item2->GetDatarate(), item1->GetDatarate());
			break;

		//Morph - modified by AndCycle, more uploading session info to show full chunk transfer
		case 3: 
			iResult=CompareUnsigned(item1->GetQueueSessionUp(), item2->GetQueueSessionUp());
			break;
		case 103: 
			iResult=CompareUnsigned(item2->GetQueueSessionUp(), item1->GetQueueSessionUp());
			break;
		//Morph - modified by AndCycle, more uploading session info to show full chunk transfer

		case 4: 
			iResult=item1->GetWaitTime() - item2->GetWaitTime();
			break;
		case 104: 
			iResult=item2->GetWaitTime() - item1->GetWaitTime();
			break;

		case 5: 
			iResult=item1->GetUpStartTimeDelay() - item2->GetUpStartTimeDelay();
			break;
		case 105: 
			iResult=item2->GetUpStartTimeDelay() - item1->GetUpStartTimeDelay();
			break;

		case 6: 
			iResult=item1->GetUploadState() - item2->GetUploadState();
			break;
		case 106: 
			iResult=item2->GetUploadState() - item1->GetUploadState();
			break;

		case 7:
			iResult=item1->GetUpPartCount() - item2->GetUpPartCount();
			break;
		case 107: 
			iResult=item2->GetUpPartCount() - item1->GetUpPartCount();
			break;
		//MORPH START - Modified by SiRoB, Client Software	
		case 8:
			iResult=item2->GetClientSoftVer().CompareNoCase(item1->GetClientSoftVer());
			break;
		case 108:
			iResult=item1->GetClientSoftVer().CompareNoCase(item2->GetClientSoftVer());
			break;
		//MORPH END - Modified by SiRoB, Client Software
		
		//MORPH START - Added By Yun.SF3, Upload/Download
		case 9: // UP-DL TOTAL
			iResult=CompareUnsigned64(item2->Credits()->GetUploadedTotal(), item1->Credits()->GetUploadedTotal());
			break;
		case 109: 
			iResult=CompareUnsigned64(item2->Credits()->GetDownloadedTotal(), item1->Credits()->GetDownloadedTotal());
			break;
		//MORPH END - Added By Yun.SF3, Upload/Download
		
		//MORPH START - Added by SiRoB, ZZ Upload System
		case 11:
			iResult=CompareUnsigned(item1->GetSlotNumber(), item2->GetSlotNumber());
			break;
		case 111:
			iResult=CompareUnsigned(item2->GetSlotNumber(), item1->GetSlotNumber());
			break;
		//MORPH END - Added by SiRoB, ZZ Upload System 20030724-0336

		//MORPH START - Added by SiRoB, Show Compression by Tarod
		case 12:
			if (item1->GetCompression() == item2->GetCompression())
				iResult=0;
			else
				iResult=item1->GetCompression() > item2->GetCompression()?1:-1;
			break;
		case 112:
			if (item1->GetCompression() == item2->GetCompression())
				iResult=0;
			else
				iResult=item2->GetCompression() > item1->GetCompression()?1:-1;
			break;
		//MORPH END - Added by SiRoB, Show Compression by Tarod
		
		// Mighty Knife: Community affiliation
		case 13:
			iResult=item1->IsCommunity() - item2->IsCommunity();
			break;
		case 113:
			iResult=item2->IsCommunity() - item1->IsCommunity();
			break;
		// [end] Mighty Knife
		// EastShare - Added by Pretender, Friend Tab
		case 14:
			iResult=item1->IsFriend() - item2->IsFriend();
			break;
		case 114:
			iResult=item2->IsFriend() - item1->IsFriend();
			break;
		// EastShare - Added by Pretender, Friend Tab
        // Commander - Added: IP2Country column - Start
        case 15:
			if(item1->GetCountryName(true) && item2->GetCountryName(true))
				iResult=CompareLocaleStringNoCase(item1->GetCountryName(true), item2->GetCountryName(true));
			else if(item1->GetCountryName(true))
				iResult=1;
			else
				iResult=-1;
			break;
		
		case 115:
			if(item1->GetCountryName(true) && item2->GetCountryName(true))
				iResult=CompareLocaleStringNoCase(item2->GetCountryName(true), item1->GetCountryName(true));
			else if(item2->GetCountryName(true))
				iResult=1;
			else
				iResult=-1;
			break;
       // Commander - Added: IP2Country column - End
		default:
			iResult=0;
			break;
	}

	// SLUGFILLER: multiSort remove - handled in parent class
	/*
	int dwNextSort;
	//call secondary sortorder, if this one results in equal
	//(Note: yes I know this call is evil OO wise, but better than changing a lot more code, while we have only one instance anyway - might be fixed later)
	if (iResult == 0 && (dwNextSort = theApp.emuledlg->transferwnd->uploadlistctrl.GetNextSortOrder(lParamSort)) != (-1)){
		iResult= SortProc(lParam1, lParam2, dwNextSort);
	}
	*/
	return iResult;

}

void CUploadListCtrl::ShowSelectedUserDetails()
{
	POINT point;
	::GetCursorPos(&point);
	CPoint p = point; 
	ScreenToClient(&p); 
	int it = HitTest(p); 
    if (it == -1)
		return;

	SetItemState(-1, 0, LVIS_SELECTED);
	SetItemState(it, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	SetSelectionMark(it);   // display selection mark correctly! 

	CUpDownClient* client = (CUpDownClient*)GetItemData(GetSelectionMark());
	if (client){
		CClientDetailDialog dialog(client, this);
		dialog.DoModal();
	}
}

void CUploadListCtrl::OnNMDblclk(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1){
		CUpDownClient* client = (CUpDownClient*)GetItemData(iSel);
		if (client){
			CClientDetailDialog dialog(client, this);
			dialog.DoModal();
		}
	}
	*pResult = 0;
}

void CUploadListCtrl::OnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
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
			const CUpDownClient* pClient = reinterpret_cast<CUpDownClient*>(pDispInfo->item.lParam);
			if (pClient != NULL){
				switch (pDispInfo->item.iSubItem){
					case 0:
						if (pClient->GetUserName() && pDispInfo->item.cchTextMax > 0){
							_tcsncpy(pDispInfo->item.pszText, pClient->GetUserName(), pDispInfo->item.cchTextMax);
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

void CUploadListCtrl::OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);

	if (pGetInfoTip->iSubItem == 0)
	{
		LVHITTESTINFO hti = {0};
		::GetCursorPos(&hti.pt);
		ScreenToClient(&hti.pt);
		if (SubItemHitTest(&hti) == -1 || hti.iItem != pGetInfoTip->iItem || hti.iSubItem != 0){
			// don' show the default label tip for the main item, if the mouse is not over the main item
			if ((pGetInfoTip->dwFlags & LVGIT_UNFOLDED) == 0 && pGetInfoTip->cchTextMax > 0 && pGetInfoTip->pszText[0] != '\0')
				pGetInfoTip->pszText[0] = '\0';
			return;
		}

		const CUpDownClient* client = (CUpDownClient*)GetItemData(pGetInfoTip->iItem);
		if (client && pGetInfoTip->pszText && pGetInfoTip->cchTextMax > 0)
		{
			CString info;

			//MORPH START - Adde by SiRoB, Optimization requpfile
			/*
			CKnownFile* file = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
			*/
			CKnownFile* file = client->CheckAndGetReqUpFile();
			//MORPH END   - Adde by SiRoB, Optimization requpfile
			// build info text and display it
			info.Format(GetResString(IDS_USERINFO), client->GetUserName());
			//MORPH START - Extra User Infos
			info += GetResString(IDS_CD_CSOFT) + _T(": ") + client->GetClientSoftVer() + _T("\n");
			info += GetResString(IDS_COUNTRY) + _T(": ") + client->GetCountryName(true) + _T("\n");
			//MORPH END   - Extra User Infos
			if (file)
			{
				info += GetResString(IDS_SF_REQUESTED) + _T(" ") + CString(file->GetFileName()) + _T("\n");
				CString stat;
				stat.Format(GetResString(IDS_FILESTATS_SESSION)+GetResString(IDS_FILESTATS_TOTAL),
							file->statistic.GetAccepts(), file->statistic.GetRequests(), CastItoXBytes(file->statistic.GetTransferred(), false, false),
							file->statistic.GetAllTimeAccepts(), file->statistic.GetAllTimeRequests(), CastItoXBytes(file->statistic.GetAllTimeTransferred(), false, false) );
				info += stat;
			}
			else
			{
				info += GetResString(IDS_REQ_UNKNOWNFILE);
			}

			_tcsncpy(pGetInfoTip->pszText, info, pGetInfoTip->cchTextMax);
			pGetInfoTip->pszText[pGetInfoTip->cchTextMax-1] = _T('\0');
		}
	}
	*pResult = 0;
}
