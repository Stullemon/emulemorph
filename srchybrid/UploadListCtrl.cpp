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

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CUploadListCtrl

IMPLEMENT_DYNAMIC(CUploadListCtrl, CMuleListCtrl)
CUploadListCtrl::CUploadListCtrl(){
}

void CUploadListCtrl::Init()
{
	CImageList ilDummyImageList; //dummy list for getting the proper height of listview entries
	ilDummyImageList.Create(1, theApp.GetSmallSytemIconSize().cy,theApp.m_iDfltImageListColorFlags|ILC_MASK, 1, 1); 
	SetImageList(&ilDummyImageList, LVSIL_SMALL);
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	ilDummyImageList.Detach();

	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	CToolTipCtrl* tooltip = GetToolTips();
	if (tooltip){
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
	InsertColumn(8,GetResString(IDS_CLIENTSOFTWARE),LVCFMT_LEFT,100,8);
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

	SetAllIcons();
	Localize();
	LoadSettings(CPreferences::tableUpload);

	// Barry - Use preferred sort order from preferences
	int sortItem = thePrefs.GetColumnSortItem(CPreferences::tableUpload);
	bool sortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableUpload);
	SetSortArrow(sortItem, sortAscending);
	// SLUGFILLER: multiSort - load multiple params
	for (int i = thePrefs.GetColumnSortCount(CPreferences::tableUpload); i > 0; ) {
		i--;
		sortItem = thePrefs.GetColumnSortItem(CPreferences::tableUpload, i);
		sortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableUpload, i);
		SortItems(SortProc, sortItem + (sortAscending ? 0:100));
	}
	// SLUGFILLER: multiSort

	// Mighty Knife: Community affiliation
	if (thePrefs.IsCommunityEnabled ()) ShowColumn (13);
	else HideColumn (13);
	// [end] Mighty Knife

	// Commander - Added: IP2Country column - Start
	if (thePrefs.GetIP2CountryNameMode() == IP2CountryName_DISABLE)
		HideColumn (15);
	// Commander - Added: IP2Country column - End
}

CUploadListCtrl::~CUploadListCtrl(){
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
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(0, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_FILE);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(1, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_DL_SPEED);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(2, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_DL_TRANSF);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(3, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_WAITED);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(4, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_UPLOADTIME);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(5, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_STATUS);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(6, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_UPSTATUS);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(7, &hdi);
		strRes.ReleaseBuffer();
		
		//MORPH START - Modified by SiRoB, Client Software
		strRes = GetResString(IDS_CLIENTSOFTWARE);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(8, &hdi);
		strRes.ReleaseBuffer();
		//MORPH END - Modified by SiRoB, Client Software

		//MORPH START - Modified by IceCream, Total up down
		strRes = GetResString(IDS_UPL_DL);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(9, &hdi);
		strRes.ReleaseBuffer();
		//MORPH END - Modified by IceCream, Total up down

		//MORPH START - Modified by IceCream, Remote Status
		strRes = GetResString(IDS_CL_DOWNLSTATUS);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(10, &hdi);
		strRes.ReleaseBuffer();
		//MORPH END - Modified by IceCream, Remote Status

		//MORPH START - Added by SiRoB, ZZ Missing
		strRes = GetResString(IDS_UPSLOTNUMBER);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(11, &hdi);
		strRes.ReleaseBuffer();
		//MORPH END - Added by SiRoB, ZZ Missing

		//MORPH START - Added by SiRoB, Show Compression by Tarod
		strRes = GetResString(IDS_COMPRESSIONGAIN);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(12, &hdi);
		strRes.ReleaseBuffer();
		//MORPH END - Added by SiRoB, Show Compression by Tarod

		// Mighty Knife: Community affiliation
		strRes = GetResString(IDS_COMMUNITY);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(13, &hdi);
		strRes.ReleaseBuffer();
		// [end] Mighty Knife

		// EastShare - Added by Pretender, Friend Tab
		strRes = GetResString(IDS_FRIENDLIST);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(14, &hdi);
		strRes.ReleaseBuffer();
		// EastShare - Added by Pretender, Friend Tab

		// Commander - Added: IP2Country column - Start
		strRes = GetResString(IDS_COUNTRY);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(15, &hdi);
		strRes.ReleaseBuffer();
		// Commander - Added: IP2Country column - End
	}
}

void CUploadListCtrl::AddClient(const CUpDownClient* client)
{
	if (!theApp.emuledlg->IsRunning())
		return;

	int iItemCount = GetItemCount();
	int iItem = InsertItem(LVIF_TEXT|LVIF_PARAM,iItemCount,LPSTR_TEXTCALLBACK,0,0,0,(LPARAM)client);
	Update(iItem);
	theApp.emuledlg->transferwnd->UpdateListCount(1, iItemCount+1);
}

void CUploadListCtrl::RemoveClient(const CUpDownClient* client)
{
	if (!theApp.emuledlg->IsRunning())
		return;

	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	sint32 result = FindItem(&find);
	if (result != -1){
		DeleteItem(result);
		theApp.emuledlg->transferwnd->UpdateListCount(1);
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
	//MORPH START - SiRoB, Don't Refresh item if not needed
	if( theApp.emuledlg->activewnd != theApp.emuledlg->transferwnd || theApp.emuledlg->transferwnd->uploadlistctrl.IsWindowVisible() == false )
		return;
	//MORPH END   - SiRoB, Don't Refresh item if not needed
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	sint16 result = FindItem(&find);
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
	RECT cur_rec = lpDrawItemStruct->rcItem;
	if (cur_rec.top >= clientRect.bottom || cur_rec.bottom <= clientRect.top)
		return;
	//MORPH END   - Added by SiRoB, Don't draw hidden Rect
	CDC* odc = CDC::FromHandle(lpDrawItemStruct->hDC);
	BOOL bCtrlFocused = ((GetFocus() == this ) || (GetStyle() & LVS_SHOWSELALWAYS));
	if( (lpDrawItemStruct->itemAction | ODA_SELECT) && (lpDrawItemStruct->itemState & ODS_SELECTED )){
		if(bCtrlFocused)
			odc->SetBkColor(m_crHighlight);
		else
			odc->SetBkColor(m_crNoHighlight);
	}
	else
		odc->SetBkColor(GetBkColor());
	const CUpDownClient* client = (CUpDownClient*)lpDrawItemStruct->itemData;
	CMemDC dc(CDC::FromHandle(lpDrawItemStruct->hDC), &lpDrawItemStruct->rcItem);
	CFont *pOldFont = dc.SelectObject(GetFont());
	//MORPH - Moved by SiRoB, Don't draw hidden Rect
	/*
	RECT cur_rec = lpDrawItemStruct->rcItem;
	*/
	COLORREF crOldTextColor = dc.SetTextColor(m_crWindowText);
    if(client->GetSlotNumber() > theApp.uploadqueue->GetActiveUploadsCount()) {
        dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
    }

	int iOldBkMode;
	if (m_crWindowTextBk == CLR_NONE){
		DefWindowProc(WM_ERASEBKGND, (WPARAM)(HDC)dc, 0);
		iOldBkMode = dc.SetBkMode(TRANSPARENT);
	}
	else
		iOldBkMode = OPAQUE;

	CString Sbuffer;
	CKnownFile* file = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
	CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
	int iCount = pHeaderCtrl->GetItemCount();
	cur_rec.right = cur_rec.left - 8;
	cur_rec.left += 4;

	for(int iCurrent = 0; iCurrent < iCount; iCurrent++){
		int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);
		if( !IsColumnHidden(iColumn) ){
			cur_rec.right += GetColumnWidth(iColumn);
			//MORPH START - Added by SiRoB, Don't draw hidden columns
			if (cur_rec.left < clientRect.right && cur_rec.right > clientRect.left)
			{
			//MORPH END   - Added by SiRoB, Don't draw hidden columns
				UINT dcdttext = DLC_DT_TEXT; //MORPH - Added by SiRoB, Justify Text Option
				switch(iColumn){
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
							image = (client->IsMorph())?9:1;
						else
							image = 0;

						POINT point = {cur_rec.left, cur_rec.top+1};
						UINT uOvlImg = INDEXTOOVERLAYMASK(((client->Credits() && client->Credits()->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED) ? 1 : 0) | ((client->credits->GetScoreRatio(client->GetIP()) > 1) ? 2 : 0));
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

						cur_rec.left +=20;
						dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
						cur_rec.left -=20;
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
						Sbuffer.Format(_T("%s"), CastItoXBytes(client->GetDatarate(), false, true));
						//MORPH START - Added by SIRoB, Right Justify and AverageDatarate display
						dcdttext |= DT_RIGHT;
						//MORPH END   - Added by SIRoB, Right Justify and AverageDatarate display
						break;
					case 3:
						//Morph - modified by AndCycle, more uploading session info to show full chunk transfer
						if(client->GetSessionUp() == client->GetQueueSessionUp()) {
							Sbuffer.Format(_T("%s (%s)"), CastItoXBytes(client->GetQueueSessionPayloadUp(), false, false), CastItoXBytes(client->GetQueueSessionUp(), false, false));
						} else {
							Sbuffer.Format(_T("%s (%s=%s+%s)"), CastItoXBytes(client->GetQueueSessionPayloadUp()), CastItoXBytes(client->GetQueueSessionUp()), CastItoXBytes(client->GetSessionUp()), CastItoXBytes(client->GetQueueSessionUp()-client->GetSessionUp()));
						}
						//Morph - modified by AndCycle, more uploading session info to show full chunk transfer
						break;
					case 4:
						if (client->HasLowID())
							if(client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick)
								Sbuffer.Format(_T("%s LowID delayed %s"),CastSecondsToHM((client->GetWaitTime())/1000), CastSecondsToHM((::GetTickCount()-client->GetUpStartTimeDelay()-client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick)/1000));
							else
								Sbuffer.Format(_T("%s LowID"),CastSecondsToHM((client->GetWaitTime())/1000));
						else
							Sbuffer = CastSecondsToHM((client->GetWaitTime())/1000);
						break;
					case 5:
						{//Morph - modified by AndCycle, upRemain
							sint32 timeleft;
							uint32 UpDatarate = client->GetDatarate();
							if(UpDatarate == 0)	timeleft = -1;
							else if(client->IsMoreUpThanDown() && client->GetQueueSessionUp() > SESSIONMAXTRANS)	timeleft = (float)(client->credits->GetDownloadedTotal() - client->credits->GetUploadedTotal())/UpDatarate;
							else if(client->GetPowerShared() && client->GetQueueSessionUp() > SESSIONMAXTRANS) timeleft = -1; //(float)(file->GetFileSize() - client->GetQueueSessionUp())/UpDatarate;
							else if(file)
								if (file->GetFileSize() > SESSIONMAXTRANS)	timeleft = (float)(SESSIONMAXTRANS - client->GetQueueSessionUp())/UpDatarate;
								else timeleft = (float)(file->GetFileSize() - client->GetQueueSessionUp())/UpDatarate;
							Sbuffer.Format(_T("%s (+%s)"), CastSecondsToHM((client->GetUpStartTimeDelay())/1000), CastSecondsToHM(timeleft));
						}//Morph - modified by AndCycle, upRemain
						break;
					case 6:
						Sbuffer = client->GetUploadStateDisplayString();
						break;
					case 7:
						//if( client->GetUpPartCount() )[
							cur_rec.bottom--;
							cur_rec.top++;
							client->DrawUpStatusBar(dc,&cur_rec,false,thePrefs.UseFlatBar());
							cur_rec.bottom++;
							cur_rec.top--;
						//}
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
						if (client->GetFriendSlot() && client->IsFriend()){
							Sbuffer.Append(_T(" FS"));
						}
						//Morph - modified by AndCycle, take PayBackFirst have same class with PowerShare
						else if (client->IsPBForPS()){
							if (client->IsMoreUpThanDown())
							{
								Sbuffer.Append(_T(" PBF"));
								if (client->Credits() && client->Credits()->GetDownloadedTotal() > client->Credits()->GetUploadedTotal())
									Sbuffer.AppendFormat( _T("(%s)"),
									CastItoXBytes((float)client->Credits()->GetDownloadedTotal()-
												(float)client->Credits()->GetUploadedTotal(),false,false));
							}
							if (client->GetPowerShared())
								Sbuffer.Append(_T(" PS"));

							CString tempFilePrio;
							if (file)
							{
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
							}
							Sbuffer.Append(_T(" ") + tempFilePrio);
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
						dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
						if(theApp.ip2country->ShowCountryFlag()){
							cur_rec.left-=20;
						}
						break;
					// Commander - Added: IP2Country column - End	
				}
				if( iColumn != 7 && iColumn != 0 && iColumn != 15)
					dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,dcdttext);
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
		dc->FrameRect(&outline_rec, &CBrush(GetBkColor()));
		outline_rec.top++;
		outline_rec.bottom--;
		outline_rec.left++;
		outline_rec.right--;

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

BEGIN_MESSAGE_MAP(CUploadListCtrl, CMuleListCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
	ON_NOTIFY_REFLECT(LVN_GETINFOTIP, OnLvnGetInfoTip)
END_MESSAGE_MAP()

// CUploadListCtrl message handlers
void CUploadListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
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
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->IsFriend()) ? MF_ENABLED  | ((!client->HasLowID() && client->GetFriendSlot())?MF_CHECKED : MF_UNCHECKED) : MF_GRAYED), MP_FRIENDSLOT, GetResString(IDS_FRIENDSLOT), _T("FRIENDSLOT"));
	//MORPH END - Added by SiRoB, Friend Addon
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient()) ? MF_ENABLED : MF_GRAYED), MP_MESSAGE, GetResString(IDS_SEND_MSG), _T("SENDMESSAGE"));
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetViewSharedFilesSupport()) ? MF_ENABLED : MF_GRAYED), MP_SHOWLIST, GetResString(IDS_VIEWFILES), _T("VIEWFILES"));
	if (Kademlia::CKademlia::isRunning() && !Kademlia::CKademlia::isConnected())
		ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetKadPort()!=0) ? MF_ENABLED : MF_GRAYED), MP_BOOT, GetResString(IDS_BOOTSTRAP));
	
	//MORPH START - Added by Yun.SF3, List Requested Files
	ClientMenu.AppendMenu(MF_SEPARATOR); // Added by sivka
	ClientMenu.AppendMenu(MF_STRING | (client ? MF_ENABLED : MF_GRAYED),MP_LIST_REQUESTED_FILES, GetResString(IDS_LISTREQUESTED), _T("FILEREQUESTED")); // Added by sivka
	//MORPH END - Added by Yun.SF3, List Requested Files
	GetPopupMenuPos(*this, point);
	ClientMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
}

BOOL CUploadListCtrl::OnCommand(WPARAM wParam,LPARAM lParam ){
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
			case MPG_ALTENTER:
			case MP_DETAIL:{
				CClientDetailDialog dialog(client);
				dialog.DoModal();
				break;
			}
			case MP_BOOT:
				if (client->GetKadPort())
					Kademlia::CKademlia::bootstrap(ntohl(client->GetIP()), client->GetKadPort());
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
	int sortItem = thePrefs.GetColumnSortItem(CPreferences::tableUpload);
	bool m_oldSortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableUpload);
	bool sortAscending = (sortItem != pNMListView->iSubItem) ? true : !m_oldSortAscending;
	// Item is column clicked
	sortItem = pNMListView->iSubItem;
	// Save new preferences
	thePrefs.SetColumnSortItem(CPreferences::tableUpload, sortItem);
	thePrefs.SetColumnSortAscending(CPreferences::tableUpload, sortAscending);
	// Sort table
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));

	*pResult = 0;
}

int CUploadListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const CUpDownClient* item1 = (CUpDownClient*)lParam1;
	const CUpDownClient* item2 = (CUpDownClient*)lParam2;
	switch(lParamSort){
		case 0: 
			if(item1->GetUserName() && item2->GetUserName())
				return CompareLocaleStringNoCase(item1->GetUserName(), item2->GetUserName());
			else if(item1->GetUserName())
				return 1;
			else
				return -1;
		case 100:
			if(item1->GetUserName() && item2->GetUserName())
				return CompareLocaleStringNoCase(item2->GetUserName(), item1->GetUserName());
			else if(item2->GetUserName())
				return 1;
			else
				return -1;
		case 1: {
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			if( (file1 != NULL) && (file2 != NULL))
				return CompareLocaleStringNoCase(file1->GetFileName(), file2->GetFileName());
			else if( file1 == NULL )
				return 1;
			else
				return -1;
		}
		case 101:{
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			if( (file1 != NULL) && (file2 != NULL))
				return CompareLocaleStringNoCase(file2->GetFileName(), file1->GetFileName());
			else if( file1 == NULL )
				return 1;
			else
				return -1;
		}
		case 2: 
			return CompareUnsigned(item1->GetDatarate(), item2->GetDatarate());
		case 102:
			return CompareUnsigned(item2->GetDatarate(), item1->GetDatarate());

		//Morph - modified by AndCycle, more uploading session info to show full chunk transfer
		case 3: 
			return CompareUnsigned(item1->GetQueueSessionUp(), item2->GetQueueSessionUp());
		case 103: 
			return CompareUnsigned(item2->GetQueueSessionUp(), item1->GetQueueSessionUp());
		//Morph - modified by AndCycle, more uploading session info to show full chunk transfer

		case 4: 
			return item1->GetWaitTime() - item2->GetWaitTime();
		case 104: 
			return item2->GetWaitTime() - item1->GetWaitTime();
		case 5: 
			return item1->GetUpStartTimeDelay() - item2->GetUpStartTimeDelay();
		case 105: 
			return item2->GetUpStartTimeDelay() - item1->GetUpStartTimeDelay();
		case 6: 
			return item1->GetUploadState() - item2->GetUploadState();
		case 106: 
			return item2->GetUploadState() - item1->GetUploadState();
		case 7:
			return item1->GetUpPartCount() - item2->GetUpPartCount();
		case 107: 
			return item2->GetUpPartCount() - item1->GetUpPartCount();
		//MORPH START - Modified by SiRoB, Client Software	
		case 8:
			return item2->GetClientSoftVer().CompareNoCase(item1->GetClientSoftVer());
		case 108:
			return item1->GetClientSoftVer().CompareNoCase(item2->GetClientSoftVer());
		//MORPH END - Modified by SiRoB, Client Software
		
		//MORPH START - Added By Yun.SF3, Upload/Download
		case 9: // UP-DL TOTAL
			return item2->Credits()->GetUploadedTotal() - item1->Credits()->GetUploadedTotal();
		case 109: 
			return item2->Credits()->GetDownloadedTotal() - item1->Credits()->GetDownloadedTotal();
		//MORPH END - Added By Yun.SF3, Upload/Download
		
		//MORPH START - Added by SiRoB, ZZ Upload System
		case 11:
			return CompareUnsigned(item1->GetSlotNumber(), item2->GetSlotNumber());
		case 111:
			return CompareUnsigned(item2->GetSlotNumber(), item1->GetSlotNumber());
		//MORPH END - Added by SiRoB, ZZ Upload System 20030724-0336

		//MORPH START - Added by SiRoB, Show Compression by Tarod
		case 12:
			if (item1->GetCompression() == item2->GetCompression())
				return 0;
			else
				return item1->GetCompression() > item2->GetCompression()?1:-1;
		case 112:
			if (item1->GetCompression() == item2->GetCompression())
				return 0;
			else
				return item2->GetCompression() > item1->GetCompression()?1:-1;
		//MORPH END - Added by SiRoB, Show Compression by Tarod
		
		// Mighty Knife: Community affiliation
		case 13:
			return item1->IsCommunity() - item2->IsCommunity();
		case 113:
			return item2->IsCommunity() - item1->IsCommunity();
		// [end] Mighty Knife
		// EastShare - Added by Pretender, Friend Tab
		case 14:
			return item1->IsFriend() - item2->IsFriend();
		case 114:
			return item2->IsFriend() - item1->IsFriend();
		// EastShare - Added by Pretender, Friend Tab
        // Commander - Added: IP2Country column - Start
        case 15:
			if(item1->GetCountryName(true) && item2->GetCountryName(true))
				return CompareLocaleStringNoCase(item1->GetCountryName(true), item2->GetCountryName(true));
			else if(item1->GetCountryName(true))
				return 1;
			else
				return -1;
		
		case 115:
			if(item1->GetCountryName(true) && item2->GetCountryName(true))
				return CompareLocaleStringNoCase(item2->GetCountryName(true), item1->GetCountryName(true));
			else if(item2->GetCountryName(true))
				return 1;
			else
				return -1;
       // Commander - Added: IP2Country column - End
		default:
			return 0;
	}
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

	const CUpDownClient* client = (CUpDownClient*)GetItemData(GetSelectionMark());
	if (client){
		CClientDetailDialog dialog(client);
		dialog.DoModal();
	}
}

void CUploadListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1){
		const CUpDownClient* client = (CUpDownClient*)GetItemData(iSel);
		if (client){
			CClientDetailDialog dialog(client);
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

			CKnownFile* file = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
			// build info text and display it
			info.Format(GetResString(IDS_USERINFO), client->GetUserName());
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
