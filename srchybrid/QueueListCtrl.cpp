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

// QueueListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "QueueListCtrl.h"
#include "otherfunctions.h"
#include "opcodes.h"
#include "ClientDetailDialog.h"
#include "Exceptions.h"
#include "KademliaWnd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CQueueListCtrl

IMPLEMENT_DYNAMIC(CQueueListCtrl, CMuleListCtrl)
CQueueListCtrl::CQueueListCtrl(){

	// Barry - Refresh the queue every 10 secs
	VERIFY( (m_hTimer = ::SetTimer(NULL, NULL, 10000, QueueUpdateTimer)) );
	if (!m_hTimer)
		AddDebugLogLine(true,_T("Failed to create 'queue list control' timer - %s"),GetErrorMessage(GetLastError()));
}

void CQueueListCtrl::Init()
{
	CImageList ilDummyImageList; //dummy list for getting the proper height of listview entries
	ilDummyImageList.Create(1, theApp.GetSmallSytemIconSize().cy,theApp.m_iDfltImageListColorFlags|ILC_MASK, 1, 1); 
	SetImageList(&ilDummyImageList, LVSIL_SMALL);
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	ilDummyImageList.Detach();

	SetExtendedStyle(LVS_EX_FULLROWSELECT);
	InsertColumn(0,GetResString(IDS_QL_USERNAME),LVCFMT_LEFT,150,0);
	InsertColumn(1,GetResString(IDS_FILE),LVCFMT_LEFT,275,1);
	InsertColumn(2,GetResString(IDS_FILEPRIO),LVCFMT_LEFT,110,2);
	InsertColumn(3,GetResString(IDS_QL_RATING),LVCFMT_LEFT,60,3);
	InsertColumn(4,GetResString(IDS_SCORE),LVCFMT_LEFT,60,4);
	InsertColumn(5,GetResString(IDS_ASKED),LVCFMT_LEFT,60,5);
	InsertColumn(6,GetResString(IDS_LASTSEEN),LVCFMT_LEFT,110,6);
	InsertColumn(7,GetResString(IDS_ENTERQUEUE),LVCFMT_LEFT,110,7);
	InsertColumn(8,GetResString(IDS_BANNED),LVCFMT_LEFT,60,8);
	InsertColumn(9,GetResString(IDS_UPSTATUS),LVCFMT_LEFT,100,9);
	//MORPH START - Added by SiRoB, Client Software
	InsertColumn(10,GetResString(IDS_CLIENTSOFTWARE),LVCFMT_LEFT,100,10);
	//MORPH END - Added by SiRoB, Client Software
	
	Localize();
	LoadSettings(CPreferences::tableQueue);
	// Barry - Use preferred sort order from preferences
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableQueue);
	bool sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableQueue);
	SetSortArrow(sortItem, sortAscending);
	// SLUGFILLER: multiSort - load multiple params
	for (int i = theApp.glob_prefs->GetColumnSortCount(CPreferences::tableQueue); i > 0; ) {
		i--;
		sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableQueue, i);
		sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableQueue, i);
		SortItems(SortProc, sortItem + (sortAscending ? 0:100));
	}
	// SLUGFILLER: multiSort
}

CQueueListCtrl::~CQueueListCtrl()
{
	// Barry - Kill the timer that was created
	try
	{
		if (m_hTimer)
			::KillTimer(NULL, m_hTimer);
	} catch (...) {}
}

void CQueueListCtrl::Localize()
{
	imagelist.DeleteImageList();
	imagelist.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	imagelist.SetBkColor(CLR_NONE);
	imagelist.Add(CTempIconLoader("ClientEDonkey"));
	//MORPH START - Changed by SiRoB, More client & Credit overlay icon
	imagelist.Add(CTempIconLoader("ClientCompatible"));
	//imagelist.Add(CTempIconLoader("ClientEDonkeyPlus"));
	//imagelist.Add(CTempIconLoader("ClientCompatiblePlus"));
	imagelist.Add(CTempIconLoader("Friend"));
	imagelist.Add(CTempIconLoader("ClientMLDonkey"));
	//imagelist.Add(CTempIconLoader("ClientMLDonkeyPlus"));
	imagelist.Add(CTempIconLoader("ClientEDonkeyHybrid"));
	//imagelist.Add(CTempIconLoader("ClientEDonkeyHybridPlus"));
	imagelist.Add(CTempIconLoader("ClientShareaza"));
	//imagelist.Add(CTempIconLoader("ClientShareazaPlus"));
	imagelist.Add(CTempIconLoader("ClientRightEdonkey"));
	imagelist.Add(CTempIconLoader("ClientMorph"));
	imagelist.SetOverlayImage(imagelist.Add(CTempIconLoader("ClientSecureOvl")), 1);
	imagelist.SetOverlayImage(imagelist.Add(CTempIconLoader("ClientCreditOvl")), 2);
	imagelist.SetOverlayImage(imagelist.Add(CTempIconLoader("ClientCreditSecureOvl")), 3);
	//MORPH END   - Added by SiRoB, More client icone & Credit overlay icon
	
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

		strRes = GetResString(IDS_FILEPRIO);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(2, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_QL_RATING);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(3, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_SCORE);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(4, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_ASKED);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(5, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_LASTSEEN);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(6, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_ENTERQUEUE);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(7, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_BANNED);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(8, &hdi);
		strRes.ReleaseBuffer();
		
		strRes = GetResString(IDS_UPSTATUS);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(9, &hdi);
		strRes.ReleaseBuffer();
		
		//MORPH START - Modified by SiRoB, Client Software
		strRes = GetResString(IDS_CLIENTSOFTWARE);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(10, &hdi);
		strRes.ReleaseBuffer();
		//MORPH END - Modified by SiRoB, Client Software
	}
}

void CQueueListCtrl::AddClient(CUpDownClient* client, bool resetclient){
	if( resetclient && client){
			client->SetWaitStartTime();
			client->SetAskedCount(1);
	//MORPH START - Added by SiRoB, ZZ Upload System
	} else if( client ) {
		// Clients that have been put back "first" on queue (that is, they
		// get to keep its waiting time since before they started upload), are
		// recognized by having an ask count of 0.
		client->SetAskedCount(0);
	//MORPH END - Added by SiRoB, ZZ Upload System
	}
	if(theApp.glob_prefs->IsQueueListDisabled())
		return;
	uint32 itemnr = GetItemCount();
	itemnr = InsertItem(LVIF_TEXT|LVIF_PARAM,itemnr,LPSTR_TEXTCALLBACK,0,0,1,(LPARAM)client);
	RefreshClient(client);
	theApp.emuledlg->transferwnd.UpdateListCount(2);
}

void CQueueListCtrl::RemoveClient(CUpDownClient* client){
	if (!theApp.emuledlg->IsRunning()) return;
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	sint32 result = FindItem(&find);
	if (result != (-1) )
		DeleteItem(result);
	theApp.emuledlg->transferwnd.UpdateListCount(2);
}

void CQueueListCtrl::RefreshClient(CUpDownClient* client){
	// There is some type of timing issue here.. If you click on item in the queue or upload and leave
	// the focus on it when you exit the cient, it breaks on line 854 of emuleDlg.cpp.. 
	// I added this IsRunning() check to this function and the DrawItem method and
	// this seems to keep it from crashing. This is not the fix but a patch until
	// someone points out what is going wrong.. Also, it will still assert in debug mode..
	if( !theApp.emuledlg->IsRunning() )
		return;
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	sint16 result = FindItem(&find);
	if(result != -1)
		Update(result);
	return;
}

#define DLC_DT_TEXT (DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)

void CQueueListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct){
	if( !theApp.emuledlg->IsRunning() )
		return;
	if (!lpDrawItemStruct->itemData)
		return;
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
	CUpDownClient* client = (CUpDownClient*)lpDrawItemStruct->itemData;
	CMemDC dc(CDC::FromHandle(lpDrawItemStruct->hDC),&CRect(lpDrawItemStruct->rcItem));
	CFont* pOldFont = dc.SelectObject(GetFont());
	RECT cur_rec;
	MEMCOPY(&cur_rec,&lpDrawItemStruct->rcItem,sizeof(RECT));
	COLORREF crOldTextColor = dc.SetTextColor(m_crWindowText);
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
			//MORPH START - Added by SiRoB, Don't draw hidden colums
			CRect Rect;
			this->GetClientRect(Rect);
			if ((Rect.left<=cur_rec.right && cur_rec.right<=Rect.right)
				||(Rect.left<=cur_rec.left && cur_rec.left<=Rect.right)
				||(Rect.left>=cur_rec.left && cur_rec.right>=Rect.right)){
			//MORPH END   - Added by SiRoB, Don't draw hidden colums
				switch(iColumn){
					case 0:{
						//MORPH START - Modified by SiRoB, More client & Credit overlay icon
						uint8 image;
						if (client->IsFriend())
							image = 2;
						else{
							if (client->GetClientSoft() == SO_MLDONKEY )
								image = 3;
							else if (client->GetClientSoft() == SO_EDONKEYHYBRID )
								image = 4;
							else if (client->GetClientSoft() == SO_SHAREAZA )
								image = 5;
							else if (client->GetClientSoft() == SO_EDONKEY )
								image = 6;
							else if (client->ExtProtocolAvailable())
								image = (client->IsMorph())?7:1;
							else
								image = 0;
						}
						//MORPH END   - Modified by SiRoB, More Icons
						POINT point = {cur_rec.left, cur_rec.top+1};
						//MORPH START - Modified by SiRoB, leecher icon
						//imagelist.Draw(dc,image, point, ILD_NORMAL | ((client->Credits() && client->Credits()->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED) ? INDEXTOOVERLAYMASK(1) : 0));
						UINT uOvlImg = INDEXTOOVERLAYMASK(((client->Credits() && client->Credits()->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED) ? 1 : 0) | ((client->credits->GetScoreRatio(client->GetIP()) > 1) ? 2 : 0));
						if (client->IsLeecher())
							imagelist.DrawIndirect(dc,image, point, CSize(16,16), CPoint(0,0), ILD_NORMAL | uOvlImg, 0, RGB(255,64,64));//RGB(255-GetRValue(odc->GetBkColor()),255-GetGValue(odc->GetBkColor()),255-GetBValue(odc->GetBkColor())));
						else
							imagelist.DrawIndirect(dc,image, point, CSize(16,16), CPoint(0,0), ILD_NORMAL | uOvlImg, 0, odc->GetBkColor());
						//MORPH END   - Modified by SiRoB, leecher icon
					
						Sbuffer = client->GetUserName();
						cur_rec.left +=20;
						dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
						cur_rec.left -=20;
						break;
					}
					case 1:
						if(file)
							Sbuffer = file->GetFileName();
						else
							Sbuffer = "?";
						break;
					case 2:
						if(file){
							switch (file->GetUpPriority()) {
								case PR_VERYLOW : {
									Sbuffer = GetResString(IDS_PRIOVERYLOW);
									break; }
								case PR_LOW : {
									if( file->IsAutoUpPriority() )
										Sbuffer = GetResString(IDS_PRIOAUTOLOW);
									else
										Sbuffer = GetResString(IDS_PRIOLOW);
									break; }
								case PR_NORMAL : {
									if( file->IsAutoUpPriority() )
										Sbuffer = GetResString(IDS_PRIOAUTONORMAL);
									else
										Sbuffer = GetResString(IDS_PRIONORMAL);
									break; }
								case PR_HIGH : {
									if( file->IsAutoUpPriority() )
										Sbuffer = GetResString(IDS_PRIOAUTOHIGH);
									else
										Sbuffer = GetResString(IDS_PRIOHIGH);
									break; }
								case PR_VERYHIGH : {
									Sbuffer = GetResString(IDS_PRIORELEASE);
									break; }
								default:
									Sbuffer.Empty();
							}

							//Morph Start - added by AndCycle, Equal Chance For Each File
							if(theApp.glob_prefs->GetEqualChanceForEachFileMode() != ECFEF_DISABLE){

								CString ecfef;

								switch(theApp.glob_prefs->GetEqualChanceForEachFileMode()){

									case ECFEF_ACCEPTED:{
										if(theApp.glob_prefs->IsECFEFallTime()){
											ecfef.Format("%u", file->statistic.GetAllTimeAccepts());
										}
										else{
											ecfef.Format("%u", file->statistic.GetAccepts());
										}
									}break;

									case ECFEF_ACCEPTED_COMPLETE:{
										if(theApp.glob_prefs->IsECFEFallTime()){
											ecfef.Format("%.2f: %u/%u", (float)file->statistic.GetAllTimeAccepts()/file->GetPartCount(), file->statistic.GetAllTimeAccepts(), file->GetPartCount());
										}
										else{
											ecfef.Format("%.2f: %u/%u", (float)file->statistic.GetAccepts()/file->GetPartCount(), file->statistic.GetAccepts(), file->GetPartCount());
										}
									}break;

									case ECFEF_TRANSFERRED:{
										if(theApp.glob_prefs->IsECFEFallTime()){
											ecfef.Format("%s", CastItoXBytes(file->statistic.GetAllTimeTransferred()));
										}
										else{
											ecfef.Format("%s", CastItoXBytes(file->statistic.GetTransferred()));
										}
									}break;

									case ECFEF_TRANSFERRED_COMPLETE:{
										if(theApp.glob_prefs->IsECFEFallTime()){
											ecfef.Format("%.2f: %s/%s", (float)file->statistic.GetAllTimeTransferred()/file->GetFileSize(), CastItoXBytes(file->statistic.GetAllTimeTransferred()), CastItoXBytes(file->GetFileSize()));
										}
										else{
											ecfef.Format("%.2f: %s/%s", (float)file->statistic.GetTransferred()/file->GetFileSize(), CastItoXBytes(file->statistic.GetTransferred()), CastItoXBytes(file->GetFileSize()));
										}
									}break;

									default:{
										ecfef.Empty();
									}break;
								}
								if(file->GetPowerShared()){//keep PS file prio
									Sbuffer.Append(" ");
									Sbuffer.Append(ecfef);
								}
								else{
									Sbuffer = ecfef;
								}

							}
							//Morph End - added by AndCycle, Equal Chance For Each File

							//MORPH START - Added by SiRoB, ZZ Upload System
							if(file->GetPowerShared()) {
								CString tempString = GetResString(IDS_POWERSHARE_PREFIX);
								tempString.Append(" ");
								tempString.Append(Sbuffer);
								Sbuffer.Empty(); //MORPH - HotFix by SiRoB, ZZ Upload System
								Sbuffer = tempString;
							}
							//MORPH END - Added by SiRoB, ZZ Upload System

							//EastShare START - Added by TAHO, Pay Back First
							if(client->MoreUpThanDown()) {
								CString tempString2 = "PBF ";
								tempString2.Append(Sbuffer);
								Sbuffer.Empty();
								Sbuffer = tempString2;
							}
							//EastShare END - Added by TAHO, Pay Back First

						}
						else
							Sbuffer = "?";
						break;
					case 3:
						Sbuffer.Format("%i",client->GetScore(false,false,true));
						break;
					case 4:
						if (client->HasLowID()){
							if (client->m_bAddNextConnect)
								Sbuffer.Format("%i ****",client->GetScore(false));
							else
								Sbuffer.Format("%i LowID",client->GetScore(false));
						}
						else
							Sbuffer.Format("%i",client->GetScore(false));
						break;
					case 5:
						Sbuffer.Format("%i",client->GetAskedCount());
						break;
					case 6:
						Sbuffer = CastSecondsToHM((::GetTickCount() - client->GetLastUpRequest())/1000);
						break;
					case 7:
						Sbuffer = CastSecondsToHM((::GetTickCount() - client->GetWaitStartTime())/1000);
						break;
					case 8:
						if(client->IsBanned())
							Sbuffer = GetResString(IDS_YES);
						else
							Sbuffer = GetResString(IDS_NO);
						break;
					case 9:
						if( client->GetUpPartCount()){
							cur_rec.bottom--;
							cur_rec.top++;
							client->DrawUpStatusBar(dc,&cur_rec,false,theApp.glob_prefs->UseFlatBar());
							cur_rec.bottom++;
							cur_rec.top--;
						}
						break;
					//MORPH START - Modified by SiRoB, Client Software
					case 10:
						Sbuffer = client->GetClientVerString();
						break;
					//MORPH END - Modified by SiRoB, Client Software
				}
				if( iColumn != 9 && iColumn != 0)
					dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
			//MORPH START - Added by SiRoB, Don't draw hidden colums
			}
			//MORPH END   - Added by SiRoB, Don't draw hidden colums
			cur_rec.left += GetColumnWidth(iColumn);
		}
	}
//draw rectangle around selected item(s)
	if ((lpDrawItemStruct->itemAction | ODA_SELECT) && (lpDrawItemStruct->itemState & ODS_SELECTED))
	{
		RECT outline_rec;
		MEMCOPY(&outline_rec,&lpDrawItemStruct->rcItem,sizeof(RECT));

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
	dc.SelectObject(pOldFont);
	dc.SetTextColor(crOldTextColor);
}

BEGIN_MESSAGE_MAP(CQueueListCtrl, CMuleListCtrl)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
END_MESSAGE_MAP()

// CQueueListCtrl message handlers
void CQueueListCtrl::OnContextMenu(CWnd* pWnd, CPoint point){
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	UINT uFlags = (iSel != -1) ? MF_ENABLED : MF_GRAYED;
	CUpDownClient* client = (iSel != -1) ? (CUpDownClient*)GetItemData(iSel) : NULL;

	CTitleMenu ClientMenu;
	ClientMenu.CreatePopupMenu();
	ClientMenu.AddMenuTitle(GetResString(IDS_CLIENTS));
	ClientMenu.AppendMenu(MF_STRING | uFlags,MP_DETAIL, GetResString(IDS_SHOWDETAILS));
	ClientMenu.AppendMenu(MF_STRING | uFlags,MP_ADDFRIEND, GetResString(IDS_ADDFRIEND));
	ClientMenu.AppendMenu(MF_STRING | uFlags,MP_MESSAGE, GetResString(IDS_SEND_MSG));
	ClientMenu.AppendMenu(MF_STRING | uFlags,MP_SHOWLIST, GetResString(IDS_VIEWFILES));
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsBanned()) ? MF_ENABLED : MF_GRAYED),MP_UNBAN, GetResString(IDS_UNBAN));
	if(theApp.kademlia->GetThreadID() && !theApp.kademlia->isConnected() )
		ClientMenu.AppendMenu(MF_STRING | uFlags,MP_BOOT, "BootStrap");
	//MORPH START - Added by Yun.SF3, List Requested Files
	ClientMenu.AppendMenu(MF_SEPARATOR); // Added by sivka
	ClientMenu.AppendMenu(MF_STRING | uFlags,MP_LIST_REQUESTED_FILES, _T(GetResString(IDS_LISTREQUESTED))); // Added by sivka
	//MORPH END - Added by Yun.SF3, List Requested Files
	
	ClientMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
}

BOOL CQueueListCtrl::OnCommand(WPARAM wParam,LPARAM lParam ){
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1){
		CUpDownClient* client = (CUpDownClient*)GetItemData(iSel);
		switch (wParam){
			case MP_SHOWLIST:
				client->RequestSharedFileList();
				break;
			case MP_MESSAGE:{
				theApp.emuledlg->chatwnd.StartSession(client);
				break;
			}
			case MP_ADDFRIEND:{
				theApp.friendlist->AddFriend(client);
				break;
			}
			case MP_UNBAN:{
				if( client->IsBanned() )
					client->UnBan();
				break;
			}
			case MPG_ALTENTER:
			case MP_DETAIL:{
				CClientDetailDialog dialog(client);
				dialog.DoModal();
				break;
			}
			case MP_BOOT:{
				if(	theApp.kademlia->GetThreadID() && client->GetKadPort())
				{
					theApp.kademlia->Bootstrap(ntohl(client->GetIP()), client->GetKadPort());
				}
				break;
			}
			//MORPH START - Added by Yun.SF3, List Requested Files
			case MP_LIST_REQUESTED_FILES: { // added by sivka
				if (client != NULL)
				{
					CString fileList;
					fileList += GetResString(IDS_LISTREQDL);
					fileList += "\n--------------------------\n" ; 
					if (theApp.downloadqueue->IsPartFile(client->reqfile))
					{
						fileList += client->reqfile->GetFileName(); 
						for(POSITION pos = client->m_OtherRequests_list.GetHeadPosition();pos!=0;client->m_OtherRequests_list.GetNext(pos))
						{
							fileList += "\n" ; 
							fileList += client->m_OtherRequests_list.GetAt(pos)->GetFileName(); 
						}
					}
					else
						fileList += GetResString(IDS_LISTREQNODL);
					fileList += "\n\n\n";
					fileList += GetResString(IDS_LISTREQUL);
					fileList += "\n------------------------\n" ; 
					CKnownFile* uploadfile = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
					if(uploadfile)
						fileList += uploadfile->GetFileName();
					else
						fileList += GetResString(IDS_LISTREQNOUL);
					AfxMessageBox(fileList,MB_OK);
				}
				break;
			}
			//MORPH START - Added by Yun.SF3, List Requested Files
		}
	}
	return true;
} 

void CQueueListCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult){

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableQueue);
	bool m_oldSortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableQueue);
	bool sortAscending = (sortItem != pNMListView->iSubItem) ? true : !m_oldSortAscending;
	// Item is column clicked
	sortItem = pNMListView->iSubItem;
	// Save new preferences
	theApp.glob_prefs->SetColumnSortItem(CPreferences::tableQueue, sortItem);
	theApp.glob_prefs->SetColumnSortAscending(CPreferences::tableQueue, sortAscending);
	// Sort table
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));

	*pResult = 0;
}

int CQueueListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort){
	CUpDownClient* item1 = (CUpDownClient*)lParam1;
	CUpDownClient* item2 = (CUpDownClient*)lParam2;
	switch(lParamSort){
		case 0: 
			if(item1->GetUserName() && item2->GetUserName())
				return _tcsicmp(item1->GetUserName(), item2->GetUserName());
			else if(item1->GetUserName())
				return 1;
			else
				return -1;
		case 100:
			if(item2->GetUserName() && item1->GetUserName())
				return _tcsicmp(item2->GetUserName(), item1->GetUserName());
			else if(item2->GetUserName())
				return 1;
			else
				return -1;
		
		case 1: {
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			if( (file1 != NULL) && (file2 != NULL))
				return _tcsicmp(file1->GetFileName(), file2->GetFileName());
			else if( file1 == NULL )
				return 1;
			else
				return -1;
		}
		case 101: {
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			if( (file1 != NULL) && (file2 != NULL))
				return _tcsicmp(file2->GetFileName(), file1->GetFileName());
			else if( file1 == NULL )
				return 1;
			else
				return -1;
		}
		
		//MORPH START - Changed by SiRoB, ZZ Upload System
		case 2: 
		case 102: {
			int result = 0;
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			if( (file1 != NULL) && (file2 != NULL)){
 				//MORPH START - Added by SiRoB, Pay Back First
				if (item1->MoreUpThanDown()) result += 2;
				if (item2->MoreUpThanDown()) result -= 2;
				//MORPH END   - Added by SiRoB, Pay Back First
				if (item1->GetPowerShared()) result ++;
 				if (item2->GetPowerShared()) result --;
				//Morph Start - added by AndCycle, Equal Chance For Each File
				if(item1->GetPowerShared() && item2->GetPowerShared()){//Equal chance keep the file prio under PowerShare
					result = ((file1->GetUpPriority()==PR_VERYLOW) ? -1 : file1->GetUpPriority()) - ((file2->GetUpPriority()==PR_VERYLOW) ? -1 : file2->GetUpPriority());
				}
				if (result == 0){
					switch(theApp.glob_prefs->GetEqualChanceForEachFileMode()){
							case ECFEF_ACCEPTED:
								if(theApp.glob_prefs->IsECFEFallTime()){
									result = file2->statistic.GetAllTimeAccepts() - file1->statistic.GetAllTimeAccepts();
								}
								else{
									result = file2->statistic.GetAccepts() - file1->statistic.GetAccepts();
								}
								break;
							case ECFEF_ACCEPTED_COMPLETE:
								if(theApp.glob_prefs->IsECFEFallTime()){
									result = //result is INT
										(float)file2->statistic.GetAllTimeAccepts()/file2->GetPartCount() == (float)file1->statistic.GetAllTimeAccepts()/file1->GetPartCount() ? 0 :
										(float)file2->statistic.GetAllTimeAccepts()/file2->GetPartCount() > (float)file1->statistic.GetAllTimeAccepts()/file1->GetPartCount() ? 1 : -1;
								}
								else{
									result = //result is INT
										(float)file2->statistic.GetAccepts()/file2->GetPartCount() == (float)file1->statistic.GetAccepts()/file1->GetPartCount() ? 0:
										(float)file2->statistic.GetAccepts()/file2->GetPartCount() > (float)file1->statistic.GetAccepts()/file1->GetPartCount() ? 1 : -1;
								}
								break;
							case ECFEF_TRANSFERRED:
								if(theApp.glob_prefs->IsECFEFallTime()){
									result = file2->statistic.GetAllTimeTransferred() - file1->statistic.GetAllTimeTransferred();
								}
								else{
									result = file2->statistic.GetTransferred() - file1->statistic.GetTransferred();
								}
								break;
							case ECFEF_TRANSFERRED_COMPLETE:
								if(theApp.glob_prefs->IsECFEFallTime()){
									result = //result is INT
										(float)file2->statistic.GetAllTimeTransferred()/file2->GetFileSize() == (float)file1->statistic.GetAllTimeTransferred()/file1->GetFileSize() ? 0 :
										(float)file2->statistic.GetAllTimeTransferred()/file2->GetFileSize() > (float)file1->statistic.GetAllTimeTransferred()/file1->GetFileSize() ? 1 : -1;
								}
								else{
									result = //result is INT
										(float)file2->statistic.GetTransferred()/file2->GetFileSize() == (float)file1->statistic.GetTransferred()/file1->GetFileSize() ? 0 :
										(float)file2->statistic.GetTransferred()/file2->GetFileSize() > (float)file1->statistic.GetTransferred()/file1->GetFileSize() ? 1 : -1;
								}
								break;
							default:
								result = ((file1->GetUpPriority()==PR_VERYLOW) ? -1 : file1->GetUpPriority()) - ((file2->GetUpPriority()==PR_VERYLOW) ? -1 : file2->GetUpPriority());
					}
				}
				//Morph End - added by AndCycle, Equal Chance For Each File
			}
			else if( file1 == NULL )
				result = 1;
			else
				result = -1;

			if(lParamSort == 2)
				return result;
			else
				return -result;
		//MORPH END - Changed by SiRoB, ZZ Upload System
		}
		case 3: 
			return CompareUnsigned(item1->GetScore(false,false,true), item2->GetScore(false,false,true));
		case 103: 
			return CompareUnsigned(item2->GetScore(false,false,true), item1->GetScore(false,false,true));

		//MORPH START - Changed by SiRoB, ZZ Upload System
		case 4: 
			//return CompareUnsigned(item1->GetScore(false), item2->GetScore(false));
		case 104: { 
			//return CompareUnsigned(item2->GetScore(false), item1->GetScore(false));
       		int result = 0;
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			if( (file1 != NULL) && (file2 != NULL)){
				//MORPH START - Added by SiRoB, Pay Back First
				if(item1->MoreUpThanDown())	result += 2;
				if(item2->MoreUpThanDown())	result -= 2;
                //MORPH END   - Added by SiRoB, Pay Back First
				if(item1->GetPowerShared())	result ++;
                if(item2->GetPowerShared())	result --;
				//Morph Start - added by AndCycle, Equal Chance For Each File
				if(item1->GetPowerShared() && item2->GetPowerShared()){//Equal chance keep the file prio under PowerShare
					result = ((file1->GetUpPriority()==PR_VERYLOW) ? -1 : file1->GetUpPriority()) - ((file2->GetUpPriority()==PR_VERYLOW) ? -1 : file2->GetUpPriority());
				}
				if(result == 0){
					switch(theApp.glob_prefs->GetEqualChanceForEachFileMode()){
						case ECFEF_ACCEPTED:
							if(theApp.glob_prefs->IsECFEFallTime()){
								result = file2->statistic.GetAllTimeAccepts() - file1->statistic.GetAllTimeAccepts();
							}
							else{
								result = file2->statistic.GetAccepts() - file1->statistic.GetAccepts();
							}
							break;
						case ECFEF_ACCEPTED_COMPLETE:
							if(theApp.glob_prefs->IsECFEFallTime()){
								result = //result is INT
									(float)file2->statistic.GetAllTimeAccepts()/file2->GetPartCount() == (float)file1->statistic.GetAllTimeAccepts()/file1->GetPartCount() ? 0 :
									(float)file2->statistic.GetAllTimeAccepts()/file2->GetPartCount() > (float)file1->statistic.GetAllTimeAccepts()/file1->GetPartCount() ? 1 : -1;
							}
							else{
								result = //result is INT
									(float)file2->statistic.GetAccepts()/file2->GetPartCount() == (float)file1->statistic.GetAccepts()/file1->GetPartCount() ? 0:
									(float)file2->statistic.GetAccepts()/file2->GetPartCount() > (float)file1->statistic.GetAccepts()/file1->GetPartCount() ? 1 : -1;
							}
							break;
						case ECFEF_TRANSFERRED:
							if(theApp.glob_prefs->IsECFEFallTime()){
								result = file2->statistic.GetAllTimeTransferred() - file1->statistic.GetAllTimeTransferred();
							}
							else{
								result = file2->statistic.GetTransferred() - file1->statistic.GetTransferred();
							}
							break;
						case ECFEF_TRANSFERRED_COMPLETE:
							if(theApp.glob_prefs->IsECFEFallTime()){
								result = //result is INT
									(float)file2->statistic.GetAllTimeTransferred()/file2->GetFileSize() == (float)file1->statistic.GetAllTimeTransferred()/file1->GetFileSize() ? 0 :
									(float)file2->statistic.GetAllTimeTransferred()/file2->GetFileSize() > (float)file1->statistic.GetAllTimeTransferred()/file1->GetFileSize() ? 1 : -1;
							}
							else{
								result = //result is INT
									(float)file2->statistic.GetTransferred()/file2->GetFileSize() == (float)file1->statistic.GetTransferred()/file1->GetFileSize() ? 0 :
									(float)file2->statistic.GetTransferred()/file2->GetFileSize() > (float)file1->statistic.GetTransferred()/file1->GetFileSize() ? 1 : -1;
							}
							break;
					}
				}
				//Morph End - added by AndCycle, Equal Chance For Each File
				if(result == 0){
					result = CompareUnsigned(item1->GetScore(false), item2->GetScore(false));
				}
			}
			else if( file1 == NULL )
				result = 1;
			else
				result = -1;
			if(lParamSort == 4)
				return result;
			else
				return -result;
		}
		//MORPH END - Changed by SiRoB, ZZ Upload System
		case 5: 
			return item1->GetAskedCount() - item2->GetAskedCount();
		case 105: 
			return item2->GetAskedCount() - item1->GetAskedCount();
		
		case 6: 
			return item1->GetLastUpRequest() - item2->GetLastUpRequest();
		case 106: 
			return item2->GetLastUpRequest() - item1->GetLastUpRequest();
		
		case 7: 
			return item1->GetWaitStartTime() - item2->GetWaitStartTime();
		case 107: 
			return item2->GetWaitStartTime() - item1->GetWaitStartTime();
		
		case 8: 
			return item1->IsBanned() - item2->IsBanned();
		case 108: 
			return item2->IsBanned() - item1->IsBanned();
		
		case 9: 
			return item1->GetUpPartCount()- item2->GetUpPartCount();
		case 109: 
			return item2->GetUpPartCount() - item1->GetUpPartCount();
		//MORPH START - Modified by SiRoB, Client Software
		case 10:
			return item2->GetClientVerString().CompareNoCase(item1->GetClientVerString());
		case 110:
			return item1->GetClientVerString().CompareNoCase(item2->GetClientVerString());
		//MORPH END - Modified by SiRoB, Client Software

		default:
			return 0;
	}
}

// Barry - Refresh the queue every 10 secs
void CALLBACK CQueueListCtrl::QueueUpdateTimer(HWND hwnd, UINT uiMsg, UINT idEvent, DWORD dwTime)
{
	// NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
	try
	{
		if (   !theApp.emuledlg->IsRunning() // Don't do anything if the app is shutting down - can cause unhandled exceptions
			|| !theApp.glob_prefs->GetUpdateQueueList()
			|| theApp.emuledlg->activewnd != &(theApp.emuledlg->transferwnd)
			|| !theApp.emuledlg->transferwnd.queuelistctrl.IsWindowVisible() )
			return;

		CUpDownClient* update = theApp.uploadqueue->GetNextClient(NULL);
		while( update )
		{
			theApp.emuledlg->transferwnd.queuelistctrl.RefreshClient(update);
			update = theApp.uploadqueue->GetNextClient(update);
		}
	}
	CATCH_DFLT_EXCEPTIONS("CQueueListCtrl::QueueUpdateTimer")
}

void CQueueListCtrl::ShowQueueClients(){
	DeleteAllItems(); 
	CUpDownClient* update = theApp.uploadqueue->GetNextClient(NULL);
	while( update )
	{
		AddClient(update,false);
		update = theApp.uploadqueue->GetNextClient(update);
	}
}


void CQueueListCtrl::ShowSelectedUserDetails() {
	POINT point;
	::GetCursorPos(&point);
	CPoint p = point; 
    ScreenToClient(&p); 
    int it = HitTest(p); 
    if (it == -1) return;
	SetSelectionMark(it);   // display selection mark correctly! 

	CUpDownClient* client = (CUpDownClient*)GetItemData(GetSelectionMark());

	if (client){
		CClientDetailDialog dialog(client);
		dialog.DoModal();
	}
}

void CQueueListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult) {
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1) {
		CUpDownClient* client = (CUpDownClient*)GetItemData(iSel);
		if (client){
			CClientDetailDialog dialog(client);
			dialog.DoModal();
		}
	}
	*pResult = 0;
}

void CQueueListCtrl::OnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
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
			CUpDownClient* pClient = reinterpret_cast<CUpDownClient*>(pDispInfo->item.lParam);
			if (pClient != NULL){
				switch (pDispInfo->item.iSubItem){
					case 0:
						if (pClient->GetUserName() != NULL && pDispInfo->item.cchTextMax > 0){
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
