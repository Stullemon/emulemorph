//this file is part of eMule
//Copyright (C)2002-2007 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "QueueListCtrl.h"
#include "OtherFunctions.h"
#include "MenuCmds.h"
#include "ClientDetailDialog.h"
#include "Exceptions.h"
#include "KademliaWnd.h"
#include "emuledlg.h"
#include "FriendList.h"
#include "UploadQueue.h"
#include "UpDownClient.h"
#include "TransferWnd.h"
#include "MemDC.h"
#include "SharedFileList.h"
#include "ClientCredits.h"
#include "PartFile.h"
#include "ChatWnd.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "Kademlia/Kademlia/Prefs.h"
#include "kademlia/net/KademliaUDPListener.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


IMPLEMENT_DYNAMIC(CQueueListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CQueueListCtrl, CMuleListCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
END_MESSAGE_MAP()

CQueueListCtrl::CQueueListCtrl()
	: CListCtrlItemWalk(this)
{
	SetGeneralPurposeFind(true, false);

	// Barry - Refresh the queue every 10 secs
	VERIFY( (m_hTimer = ::SetTimer(NULL, NULL, 10000, QueueUpdateTimer)) != NULL );
	if (thePrefs.GetVerbose() && !m_hTimer)
		AddDebugLogLine(true,_T("Failed to create 'queue list control' timer - %s"),GetErrorMessage(GetLastError()));
}

void CQueueListCtrl::Init()
{
	SetName(_T("QueueListCtrl"));

	CImageList ilDummyImageList; //dummy list for getting the proper height of listview entries
	ilDummyImageList.Create(1, theApp.GetSmallSytemIconSize().cy,theApp.m_iDfltImageListColorFlags|ILC_MASK, 1, 1); 
	SetImageList(&ilDummyImageList, LVSIL_SMALL);
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	ilDummyImageList.Detach();

	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

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
	InsertColumn(10,GetResString(IDS_CD_CSOFT),LVCFMT_LEFT,100,10);
	//MORPH END - Added by SiRoB, Client Software

	// Mighty Knife: Community affiliation
	InsertColumn(11,GetResString(IDS_COMMUNITY),LVCFMT_LEFT,100,11);
	// [end] Mighty Knife

	// EastShare - Added by Pretender, Friend Tab
	InsertColumn(12,GetResString(IDS_FRIENDLIST),LVCFMT_LEFT,75,12);
	// EastShare - Added by Pretender, Friend Tab

    // Commander - Added: IP2Country column - Start
	InsertColumn(13,GetResString(IDS_COUNTRY),LVCFMT_LEFT,100,13);
	// Commander - Added: IP2Country column - End

	SetAllIcons();
	Localize();
	LoadSettings();
	// Barry - Use preferred sort order from preferences
	SetSortArrow();
	SortItems(SortProc, GetSortItem() + (GetSortAscending() ? 0:100));

	// Mighty Knife: Community affiliation
	if (thePrefs.IsCommunityEnabled ()) ;// ShowColumn (11); //Removed by SiRoB, some people may prefere disable it
	else HideColumn (11);
	// [end] Mighty Knife

	// Commander - Added: IP2Country column - Start
	if (thePrefs.GetIP2CountryNameMode() == IP2CountryName_DISABLE)
		HideColumn (13);
	// Commander - Added: IP2Country column - End
}

CQueueListCtrl::~CQueueListCtrl()
{
	if (m_hTimer)
		VERIFY( ::KillTimer(NULL, m_hTimer) );
}

void CQueueListCtrl::OnSysColorChange()
{
	CMuleListCtrl::OnSysColorChange();
	SetAllIcons();
}

void CQueueListCtrl::SetAllIcons()
{
	imagelist.DeleteImageList();
	imagelist.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	imagelist.SetBkColor(CLR_NONE);
	imagelist.Add(CTempIconLoader(_T("ClientEDonkey")));
	//MORPH START - Changed by SiRoB, More client
	imagelist.Add(CTempIconLoader(_T("ClientCompatible")));
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
	imagelist.Add(CTempIconLoader(_T("XTREME")));
	imagelist.Add(CTempIconLoader(_T("EASTSHARE")));
	imagelist.Add(CTempIconLoader(_T("EMF")));
	imagelist.Add(CTempIconLoader(_T("NEO")));
	imagelist.Add(CTempIconLoader(_T("MEPHISTO")));
	imagelist.Add(CTempIconLoader(_T("XRAY")));
	imagelist.Add(CTempIconLoader(_T("MAGIC")));
	imagelist.SetOverlayImage(imagelist.Add(CTempIconLoader(_T("ClientSecureOvl"))), 1);
	imagelist.SetOverlayImage(imagelist.Add(CTempIconLoader(_T("OverlayObfu"))), 2);
	imagelist.SetOverlayImage(imagelist.Add(CTempIconLoader(_T("OverlaySecureObfu"))), 3);
	//MORPH END   - Added by SiRoB, More client icone

	// Mighty Knife: Community icon
	m_overlayimages.DeleteImageList ();
	m_overlayimages.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	m_overlayimages.SetBkColor(CLR_NONE);
	m_overlayimages.Add(CTempIconLoader(_T("Community")));
	// [end] Mighty Knife
	//MORPH START - Addded by SiRoB, Friend Addon
	m_overlayimages.Add(CTempIconLoader(_T("ClientFriendOvl")));
	m_overlayimages.Add(CTempIconLoader(_T("ClientFriendSlotOvl")));
	//MORPH END   - Addded by SiRoB, Friend Addon
	//MORPH START - Credit Overlay Icon
	m_overlayimages.Add(CTempIconLoader(_T("ClientCreditOvl")));
	m_overlayimages.Add(CTempIconLoader(_T("ClientCreditSecureOvl")));
	//MORPH END   - Credit Overlay Icon
	
}

void CQueueListCtrl::Localize()
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

		strRes = GetResString(IDS_FILEPRIO);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(2, &hdi);

		strRes = GetResString(IDS_QL_RATING);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(3, &hdi);

		strRes = GetResString(IDS_SCORE);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(4, &hdi);

		strRes = GetResString(IDS_ASKED);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(5, &hdi);

		strRes = GetResString(IDS_LASTSEEN);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(6, &hdi);

		strRes = GetResString(IDS_ENTERQUEUE);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(7, &hdi);

		strRes = GetResString(IDS_BANNED);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(8, &hdi);
		
		strRes = GetResString(IDS_UPSTATUS);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(9, &hdi);
		
		//MORPH START - Added by SiRoB, Client Software
		strRes = GetResString(IDS_CD_CSOFT);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(10, &hdi);
		//MORPH END - Added by SiRoB, Client Software

		// Mighty Knife: Community affiliation
		strRes = GetResString(IDS_COMMUNITY);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(11, &hdi);
		// [end] Mighty Knife

		// EastShare - Added by Pretender, Friend Tab
		strRes = GetResString(IDS_FRIENDLIST);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(12, &hdi);
		// EastShare - Added by Pretender, Friend Tab

		// Commander - Added: IP2Country column - Start
		strRes = GetResString(IDS_COUNTRY);
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(13, &hdi);
		// Commander - Added: IP2Country column - End
	}
}

void CQueueListCtrl::AddClient(/*const*/CUpDownClient* client, bool resetclient)
{
	if( resetclient && client){
		// EastShare START - Marked by TAHO, modified SUQWT
		/*
		client->SetWaitStartTime();
		*/
		// EastShare END - Marked by TAHO, modified SUQWT
		client->SetAskedCount(1);
	//MORPH START - Added by SiRoB, ZZ Upload System
	} else if( client ) {
		// Clients that have been put back "first" on queue (that is, they
		// get to keep its waiting time since before they started upload), are
		// recognized by having an ask count of 0.
		client->SetAskedCount(0);
	//MORPH END - Added by SiRoB, ZZ Upload System
	}

	if (theApp.IsRunningAsService()) return;// MORPH leuk_he:run as ntservice v1..

	if (!theApp.emuledlg->IsRunning())
		return;
	if(thePrefs.IsQueueListDisabled())
		return;

	int iItemCount = GetItemCount();
	int iItem = InsertItem(LVIF_TEXT|LVIF_PARAM,iItemCount,LPSTR_TEXTCALLBACK,0,0,0,(LPARAM)client);
	Update(iItem);
	theApp.emuledlg->transferwnd->UpdateListCount(CTransferWnd::wnd2OnQueue, iItemCount+1);
}

void CQueueListCtrl::RemoveClient(const CUpDownClient* client)
{
	if (!theApp.emuledlg->IsRunning())
		return;

	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	int result = FindItem(&find);
	if (result != -1){
		DeleteItem(result);
		theApp.emuledlg->transferwnd->UpdateListCount(CTransferWnd::wnd2OnQueue);
	}
}

void CQueueListCtrl::RefreshClient(const CUpDownClient* client)
{
	if (theApp.IsRunningAsService(SVC_LIST_OPT)) return;// MORPH leuk_he:run as ntservice v1..
	
	// There is some type of timing issue here.. If you click on item in the queue or upload and leave
	// the focus on it when you exit the cient, it breaks on line 854 of emuleDlg.cpp.. 
	// I added this IsRunning() check to this function and the DrawItem method and
	// this seems to keep it from crashing. This is not the fix but a patch until
	// someone points out what is going wrong.. Also, it will still assert in debug mode..
	if( !theApp.emuledlg->IsRunning())
		return;

	//MORPH START - SiRoB, Don't Refresh item if not needed
	if( theApp.emuledlg->activewnd != theApp.emuledlg->transferwnd || IsWindowVisible() == FALSE )
		return;
	//MORPH END   - SiRoB, Don't Refresh item if not needed
	
	//MORPH START- UpdateItemThread
	/*
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	int result = FindItem(&find);
	if(result != -1)
		Update(result);
	*/
	m_updatethread->AddItemToUpdate((LPARAM)client);
	//MORPH END- UpdateItemThread
}

#define DLC_DT_TEXT (DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)

void CQueueListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
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
	CFont* pOldFont = dc.SelectObject(GetFont());
	//MORPH - Moved by SiRoB, Don't draw hidden Rect
	/*
	CRect cur_rec(lpDrawItemStruct->rcItem);
	*/
	COLORREF crOldTextColor = dc.SetTextColor((lpDrawItemStruct->itemState & ODS_SELECTED) ? m_crHighlightText : m_crWindowText);

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
	//MORPH END   - Adde by SiRoB, Optimization requpfile
	CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
	int iCount = pHeaderCtrl->GetItemCount();
	cur_rec.right = cur_rec.left - 8;
	cur_rec.left += 4;
	CString Sbuffer;
	for(int iCurrent = 0; iCurrent < iCount; iCurrent++){
		int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);
		if( !IsColumnHidden(iColumn) ){
			cur_rec.right += GetColumnWidth(iColumn);
			//MORPH START - Added by SiRoB, Don't draw hidden columns
			if (cur_rec.left < clientRect.right && cur_rec.right > clientRect.left)
			{
			//MORPH END   - Added by SiRoB, Don't draw hidden columns
				switch(iColumn){
					case 0:{
						//MORPH START - Modified by SiRoB, More client & Credit overlay icon
						uint8 image;
						//MORPH - Removed by SiRoB, Friend Addon
						/*
						if (client->IsFriend())
							image = 2;
						else*/ if (client->GetClientSoft() == SO_EDONKEYHYBRID)
							image = 4;
						else if (client->GetClientSoft() == SO_MLDONKEY)
							image = 3;
						else if (client->GetClientSoft() == SO_SHAREAZA)
							image = 5;
						else if (client->GetClientSoft() == SO_AMULE)
							image = 6;
						else if (client->GetClientSoft() == SO_LPHANT)
							image = 7;
						else if (client->GetClientSoft() == SO_EDONKEY)
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
						//MORPH END   - Modified by SiRoB, More Icons
						uint32 nOverlayImage = 0;
						if ((client->Credits() && client->Credits()->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED))
							nOverlayImage |= 1;
						if (client->IsObfuscatedConnectionEstablished())
							nOverlayImage |= 2;
						POINT point = {cur_rec.left, cur_rec.top+1};
						imagelist.Draw(dc,image, point, ILD_NORMAL | INDEXTOOVERLAYMASK(nOverlayImage));

						//MORPH START - Credit Overlay Icon
						if (client->Credits() && client->Credits()->GetHasScore(client->GetIP())) {
							if (nOverlayImage & 1)
								m_overlayimages.Draw(dc, 4, point, ILD_TRANSPARENT);
							else 
								m_overlayimages.Draw(dc, 3, point, ILD_TRANSPARENT);
						}
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
						if(theApp.ip2country->ShowCountryFlag() && IsColumnHidden(13)){
							cur_rec.left+=20;
							POINT point2= {cur_rec.left,cur_rec.top+1};
							theApp.ip2country->GetFlagImageList()->DrawIndirect(dc, client->GetCountryFlagIndex(), point2, CSize(18,16), CPoint(0,0), ILD_NORMAL);
						}
						//EastShare End - added by AndCycle, IP to Country

						cur_rec.left +=20;
						dc.DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
						cur_rec.left -=20;

						//EastShare Start - added by AndCycle, IP to Country
						if(theApp.ip2country->ShowCountryFlag() && IsColumnHidden(13)){
							cur_rec.left-=20;
						}
						//EastShare End - added by AndCycle, IP to Country

						break;
					}
					case 1:
						if(file)
							Sbuffer = file->GetFileName();
						else
						Sbuffer = _T("?");
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
							if(thePrefs.IsEqualChanceEnable()){
								if(file->GetPowerShared()){
									Sbuffer.Append(_T(" "));
									Sbuffer.Append(file->statistic.GetEqualChanceValueString());
								}
								else{
									Sbuffer = file->statistic.GetEqualChanceValueString();
								}
							}
							//Morph End - added by AndCycle, Equal Chance For Each File

							//EastShare	Start - FairPlay by AndCycle
							if (file->statistic.GetFairPlay()) {
								Sbuffer.Append(_T(",FairPlay"));
							}
							//EastShare	End   - FairPlay by AndCycle

							//MORPH START - Added by SiRoB, ZZ Upload System
							if(file->GetPowerShared()) {
								CString tempString = GetResString(IDS_POWERSHARE_PREFIX);
								tempString.Append(_T(","));
								tempString.Append(Sbuffer);
								Sbuffer.Empty(); //MORPH - HotFix by SiRoB, ZZ Upload System
								Sbuffer = tempString;
							}
							//MORPH END - Added by SiRoB, ZZ Upload System
						}
						else
							Sbuffer = _T("?");
						break;
					case 3:
						Sbuffer.Format(_T("%i"),client->GetScore(false,false,true));
						break;
					case 4:{ // add {
							if (client->HasLowID()){
            /* replaced by zz coide
						if (client->m_bAddNextConnect)
							Sbuffer.Format(_T("%i ****"),client->GetScore(false));
						else
							Sbuffer.Format(_T("%i (%s)"),client->GetScore(false), GetResString(IDS_IDLOW));
              end replace by zz code*/
             //ZZ
								if (client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick)
									Sbuffer.Format(GetResString(IDS_UP_LOWID_AWAITED),client->GetScore(false), CastSecondsToHM((::GetTickCount()-client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick)/1000));
								else
									Sbuffer.Format(GetResString(IDS_UP_LOWID2),client->GetScore(false));
              // END
								}
							else
								Sbuffer.Format(_T("%i"),client->GetScore(false));
							//EastShare START - Added by TAHO, Pay Back First
							if(file && client->IsMoreUpThanDown(file)) {
								CString tempStr;
								tempStr.Format(_T("%s %s"), _T("PBF"), Sbuffer);
								Sbuffer = tempStr;
							}
							//EastShare END - Added by TAHO, Pay Back First

							//Morph Start - added by AndCycle, show out keep full chunk transfer
							if(client->GetQueueSessionUp() > 0){
								Sbuffer.Append(_T(" F"));
							}
							//Morph End - added by AndCycle, show out keep full chunk transfer
						} //MOrph
						break;
					case 5:
						Sbuffer.Format(_T("%i"),client->GetAskedCount());
						break;
					case 6:
						Sbuffer = CastSecondsToHM((::GetTickCount() - client->GetLastUpRequest())/1000);
						break;
					case 7:
						Sbuffer = CastSecondsToHM((::GetTickCount() - (uint32)client->GetWaitStartTime())/1000);
						break;
					case 8:
						//MORPH START - Changed by SiRoB, Code Optimization
						/*
						if(client->IsBanned())
						*/
						if(client->GetUploadState() == US_BANNED)
							Sbuffer = GetResString(IDS_YES);
						else
							Sbuffer = GetResString(IDS_NO);
						break;
					case 9:
						if( client->GetUpPartCount()){
							cur_rec.bottom--;
							cur_rec.top++;
							client->DrawUpStatusBar(dc,&cur_rec,false,thePrefs.UseFlatBar());
							client->DrawCompletedPercent(dc,&cur_rec); //Fafner: client percentage - 080325
							cur_rec.bottom++;
							cur_rec.top--;
						}
						break;
					//MORPH START - Added by SiRoB, Client Software
					case 10:
						Sbuffer = client->GetClientSoftVer();
						break;
					//MORPH END - Added by SiRoB, Client Software

					// Mighty Knife: Community affiliation
					case 11:
						Sbuffer = client->IsCommunity () ? GetResString(IDS_YES) : _T("");
						break;
					// [end] Mighty Knife
					// EastShare - Added by Pretender, Friend Tab
					case 12:
						Sbuffer = client->IsFriend () ? GetResString(IDS_YES) : _T("");
						break;
					// EastShare - Added by Pretender, Friend Tab
					// Commander - Added: IP2Country column - Start
					case 13:
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
				if( iColumn != 9 && iColumn != 0 && iColumn != 13 && iColumn != 14) //JP Webcache added Column 14
					dc.DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
			}//MORPH - Added by SiRoB, Don't draw hidden colums
			cur_rec.left += GetColumnWidth(iColumn);
		}
	}

	//draw rectangle around selected item(s)
	if (lpDrawItemStruct->itemState & ODS_SELECTED)
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
	if (!theApp.IsRunningAsService(SVC_LIST_OPT)) // MORPH leuk_he:run as ntservice v1..
		m_updatethread->AddItemUpdated((LPARAM)client); //MORPH - UpdateItemThread
}

void CQueueListCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	const CUpDownClient* client = (iSel != -1) ? (CUpDownClient*)GetItemData(iSel) : NULL;

	CTitleMenu ClientMenu;
	ClientMenu.CreatePopupMenu();
	ClientMenu.AddMenuTitle(GetResString(IDS_CLIENTS),true);
	ClientMenu.AppendMenu(MF_STRING | (client ? MF_ENABLED : MF_GRAYED), MP_DETAIL, GetResString(IDS_SHOWDETAILS), _T("CLIENTDETAILS"));
	ClientMenu.SetDefaultItem(MP_DETAIL);
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && !client->IsFriend()) ? MF_ENABLED : MF_GRAYED), MP_ADDFRIEND, GetResString(IDS_ADDFRIEND), _T("ADDFRIEND"));
	//MORPH START - Added by SiRoB, Friend Addon
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->IsFriend()) ? MF_ENABLED : MF_GRAYED), MP_REMOVEFRIEND, GetResString(IDS_REMOVEFRIEND), _T("DELETEFRIEND"));
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->IsFriend()) ? MF_ENABLED  | ((!client->HasLowID() && client->IsFriend() && client->GetFriendSlot())?MF_CHECKED : MF_UNCHECKED) : MF_GRAYED), MP_FRIENDSLOT, GetResString(IDS_FRIENDSLOT), _T("FRIENDSLOT"));
	//MORPH END - Added by SiRoB, Friend Addon
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient()) ? MF_ENABLED : MF_GRAYED), MP_MESSAGE, GetResString(IDS_SEND_MSG), _T("SENDMESSAGE"));
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetViewSharedFilesSupport()) ? MF_ENABLED : MF_GRAYED), MP_SHOWLIST, GetResString(IDS_VIEWFILES), _T("VIEWFILES"));
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->IsBanned()) ? MF_ENABLED : MF_GRAYED), MP_UNBAN, GetResString(IDS_UNBAN));
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

BOOL CQueueListCtrl::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
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
			case MP_UNBAN:
				if (client->IsBanned()){
					client->UnBan();
					Update(iSel);
				}
				break;
  			//MORPH START - Added by SiRoB, Friend Addon
			case MP_REMOVEFRIEND:{//LSD
				if (client && client->IsFriend())
				{
					theApp.friendlist->RemoveFriend(client->m_Friend);
					Update(iSel);
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
					Update(iSel);
				}
				//MORPH END - Modified by SIRoB, Added by Yun.SF3, ZZ Upload System
				break;
			}
			//Xman end
			//MORPH END  - Added by SiRoB, Friend Addon
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
					Kademlia::CKademlia::Bootstrap(ntohl(client->GetIP()), client->GetKadPort(), (client->GetKadVersion() > 1));
				break;
			//MORPH START - Added by Yun.SF3, List Requested Files
			case MP_LIST_REQUESTED_FILES: { // added by sivka
				if (client != NULL)
				{
					client->ShowRequestedFiles(); //Changed by SiRoB
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
	bool sortAscending = (GetSortItem()!= pNMListView->iSubItem) ? true : !GetSortAscending();

	// Sort table
	UpdateSortHistory(pNMListView->iSubItem + (sortAscending ? 0:100), 100);
	SetSortArrow(pNMListView->iSubItem, sortAscending);
	SortItems(SortProc, pNMListView->iSubItem + (sortAscending ? 0:100));

	*pResult = 0;
}

int CQueueListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
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
			if(item2->GetUserName() && item1->GetUserName())
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
		case 101: {
			//MORPH START - Adde by SiRoB, Optimization requpfile
			/*
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			*/
			CKnownFile* file1 = item1->CheckAndGetReqUpFile();
			CKnownFile* file2 = item2->CheckAndGetReqUpFile();
			//MORPH END  - Adde by SiRoB, Optimization requpfile
			if( (file1 != NULL) && (file2 != NULL))
				iResult=CompareLocaleStringNoCase(file2->GetFileName(), file1->GetFileName());
			else if( file1 == NULL )
				iResult=1;
			else
				iResult=-1;
			break;
		}
		
/*:replace case 102 & 2
		case 2: {
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			if( (file1 != NULL) && (file2 != NULL))
				iResult=((file1->GetUpPriority()==PR_VERYLOW) ? -1 : file1->GetUpPriority()) - ((file2->GetUpPriority()==PR_VERYLOW) ? -1 : file2->GetUpPriority());
			else if( file1 == NULL )
				iResult=1;
			else
				iResult=-1;
			break;
		}
		case 102:{
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			if( (file1 != NULL) && (file2 != NULL))
				iResult=((file2->GetUpPriority()==PR_VERYLOW) ? -1 : file2->GetUpPriority()) - ((file1->GetUpPriority()==PR_VERYLOW) ? -1 : file1->GetUpPriority());
			else if( file1 == NULL )
				iResult=1;
			else
				iResult=-1;
			break;
		}
   end replace*/
		//MORPH START - Changed by SiRoB, ZZ Upload System
		case 2: 
		case 102: {
			//MORPH START - Adde by SiRoB, Optimization requpfile
			/*
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			*/
			CKnownFile* file1 = item1->CheckAndGetReqUpFile();
			CKnownFile* file2 = item2->CheckAndGetReqUpFile();
			//MORPH END   - Adde by SiRoB, Optimization requpfile
			if( (file1 != NULL) && (file2 != NULL)){
				//only file priority
				if (file1->GetPowerShared() || file1->statistic.GetFairPlay()) ++iResult;
 				if (file2->GetPowerShared() || file1->statistic.GetFairPlay()) --iResult;
				//Morph Start - added by AndCycle, Equal Chance For Each File
				if(iResult == 0 && (!thePrefs.IsEqualChanceEnable() || ((file1->GetPowerShared() || file1->statistic.GetFairPlay()) && (file2->GetPowerShared() || file1->statistic.GetFairPlay()))))
					iResult = ((file1->GetUpPriority()==PR_VERYLOW) ? -1 : file1->GetUpPriority()) - ((file2->GetUpPriority()==PR_VERYLOW) ? -1 : file2->GetUpPriority());
				if (iResult == 0 && file1 != file2 && thePrefs.IsEqualChanceEnable()){
					iResult =
						file1->statistic.GetEqualChanceValue() < file2->statistic.GetEqualChanceValue() ? 1 :
						file1->statistic.GetEqualChanceValue() > file2->statistic.GetEqualChanceValue() ? -1 :
						0;
				}
				//Morph End - added by AndCycle, Equal Chance For Each File
			}
			else if( file1 == NULL )
				iResult = 1;
			else
				iResult = -1;
			if(lParamSort != 2)
				iResult = -iResult;
			break;
		//MORPH END - Changed by SiRoB, ZZ Upload System
		}
		case 3: 
			iResult=CompareUnsigned(item1->GetScore(false,false,true), item2->GetScore(false,false,true));
			break;
		case 103: 
			iResult=CompareUnsigned(item2->GetScore(false,false,true), item1->GetScore(false,false,true));
			break;

		//MORPH START - Changed by SiRoB, ZZ Upload System
		case 4: 
			//iResult=CompareUnsigned(item1->GetScore(false), item2->GetScore(false));
      // break;
		case 104: { 
			//iResult=CompareUnsigned(item2->GetScore(false), item1->GetScore(false));
      //break;
			//MORPH START - Adde by SiRoB, Optimization requpfile
			/*
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			*/
			CKnownFile* file1 = item1->CheckAndGetReqUpFile();
			CKnownFile* file2 = item2->CheckAndGetReqUpFile();
			//MORPH START - Adde by SiRoB, Optimization requpfile
			
			if( (file1 != NULL) && (file2 != NULL)){

				//Morph - modified by AndCycle, definitely a correct compare to show queue
				CUpDownClient *lClient = (CUpDownClient*)item2, *rClient = (CUpDownClient*)item1;

				uint32 lScore = lClient->GetScore(false), rScore = rClient->GetScore(false);
				iResult = 
					theApp.uploadqueue->RightClientIsBetter(lClient, lScore, rClient, rScore) ? 1 :
					theApp.uploadqueue->RightClientIsBetter(rClient, rScore, lClient, lScore) ? -1 :
					0;

			}
			else if( file1 == NULL )
				iResult = 1;
			else
				iResult = -1;
			if(lParamSort != 4)
				iResult = -iResult;
			break;
		}
		//MORPH END - Changed by SiRoB, ZZ Upload System
		case 5: 
			iResult=item1->GetAskedCount() - item2->GetAskedCount();
			break;
		case 105: 
			iResult=item2->GetAskedCount() - item1->GetAskedCount();
			break;
		
		case 6: 
			iResult=item1->GetLastUpRequest() - item2->GetLastUpRequest();
			break;
		case 106: 
			iResult=item2->GetLastUpRequest() - item1->GetLastUpRequest();
			break;
		
		case 7: 
         /* replace:
        iResult=item1->GetWaitStartTime() - item2->GetWaitStartTime(); 
        break;
         */
			//EastShare START - Modified by TAHO, modified SUQWT
			{
				sint64 time1 = item1->GetWaitStartTime();
				sint64 time2 = item2->GetWaitStartTime();
				if ( time1 == time2 ) {
					iResult = 0;
				} else if ( time1 > time2 ) {
					iResult = 1;
				} else {
					iResult = -1;
				}
				break;
			}
			//EastShare END - Modified by TAHO, modified SUQWT
		case 107: 
       /* replace:
			iResult=item2->GetWaitStartTime() - item1->GetWaitStartTime();
			break;
      */ 
			//EastShare START - Modified by TAHO, modified SUQWT
			//return item2->GetWaitStartTime() - item1->GetWaitStartTime();
			{
				sint64 time1 = item1->GetWaitStartTime();
				sint64 time2 = item2->GetWaitStartTime();
				if ( time1 == time2 ) {
					iResult = 0;
				} else if ( time1 < time2 ) {
					iResult = 1;
				} else {
					iResult = -1;
				}
				break;
			}
			//EastShare END - Modified by TAHO, modified SUQWT
		case 8: 
			//MORPH - Changed by SiRoB, Code Optimization
			/*		
			iResult=item1->IsBanned() - item2->IsBanned();
			*/
			iResult=(item1->GetUploadState() == US_BANNED) - (item2->GetUploadState() == US_BANNED);
			break;
		case 108: 
			//MORPH - Changed by SiRoB, Code Optimization
			/*		
			iResult=item2->IsBanned() - item1->IsBanned();
			*/
			iResult=(item2->GetUploadState() == US_BANNED) - (item1->GetUploadState() == US_BANNED);
			break;
		case 9: 
			if (thePrefs.GetUseClientPercentage())
				iResult=(int)(item1->GetCompletedPercent()*10.f) - (int)(item2->GetCompletedPercent()*10.f); //Fafner: client percentage - 080325
			else
				iResult=item1->GetUpPartCount()- item2->GetUpPartCount();
			break;
		case 109: 
			if (thePrefs.GetUseClientPercentage())
				iResult=(int)(item2->GetCompletedPercent()*10.f) - (int)(item1->GetCompletedPercent()*10.f); //Fafner: client percentage - 080325
			else
				iResult=item2->GetUpPartCount() - item1->GetUpPartCount();
			break;
		//MORPH START - Modified by SiRoB, Client Software
		case 10:
			iResult=item2->GetClientSoftVer().CompareNoCase(item1->GetClientSoftVer());
			break;
		case 110:
			iResult=item1->GetClientSoftVer().CompareNoCase(item2->GetClientSoftVer());
			break;
		//MORPH END - Modified by SiRoB, Client Software

		// Mighty Knife: Community affiliation
		case 11:
			iResult=item1->IsCommunity() - item2->IsCommunity();
			break;
		case 111:
			iResult=item2->IsCommunity() - item1->IsCommunity();
			break;
		// [end] Mighty Knife
		// EastShare - Added by Pretender, Friend Tab
		case 12:
			iResult=item1->IsFriend() - item2->IsFriend();
			break;
		case 112:
			iResult=item2->IsFriend() - item1->IsFriend();
			break;
		// EastShare - Added by Pretender, Friend Tab
               // Commander - Added: IP2Country column - Start
		case 13:
			if(item1->GetCountryName(true) && item2->GetCountryName(true))
				iResult=CompareLocaleStringNoCase(item1->GetCountryName(true), item2->GetCountryName(true));
			else if(item1->GetCountryName(true))
				iResult=1;
			else
				iResult=-1;
			break;

		case 113:
			if(item1->GetCountryName(true) && item2->GetCountryName(true))
				iResult=CompareLocaleStringNoCase(item2->GetCountryName(true), item1->GetCountryName(true));
			else if(item2->GetCountryName(true))
				iResult=1;
			else
				iResult=-1;
                // Commander - Added: IP2Country column - End
			break;
		default:
			iResult=0;
			break;
	}
	// SLUGFILLER: multiSort remove - handled in parent class
	/*
	int dwNextSort;
	//call secondary sortorder, if this one results in equal
	//(Note: yes I know this call is evil OO wise, but better than changing a lot more code, while we have only one instance anyway - might be fixed later)
	if (iResult == 0 && (dwNextSort = theApp.emuledlg->transferwnd->queuelistctrl.GetNextSortOrder(lParamSort)) != (-1)){
		iResult= SortProc(lParam1, lParam2, dwNextSort);
	}
	*/

	return iResult;

}

// Barry - Refresh the queue every 10 secs
void CALLBACK CQueueListCtrl::QueueUpdateTimer(HWND /*hwnd*/, UINT /*uiMsg*/, UINT /*idEvent*/, DWORD /*dwTime*/)
{
	// NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
	try
	{
		if (   !theApp.emuledlg->IsRunning() // Don't do anything if the app is shutting down - can cause unhandled exceptions
			|| !thePrefs.GetUpdateQueueList()
			|| theApp.emuledlg->activewnd != theApp.emuledlg->transferwnd
			|| !theApp.emuledlg->transferwnd->queuelistctrl.IsWindowVisible() )
			return;

		const CUpDownClient* update = theApp.uploadqueue->GetNextClient(NULL);
		while( update )
		{
			theApp.emuledlg->transferwnd->queuelistctrl.RefreshClient(update);
			update = theApp.uploadqueue->GetNextClient(update);
		}
	}
	CATCH_DFLT_EXCEPTIONS(_T("CQueueListCtrl::QueueUpdateTimer"))
}

void CQueueListCtrl::ShowQueueClients()
{
	DeleteAllItems(); 
	CUpDownClient* update = theApp.uploadqueue->GetNextClient(NULL);
	while( update )
	{
		AddClient(update,false);
		update = theApp.uploadqueue->GetNextClient(update);
	}
}

void CQueueListCtrl::ShowSelectedUserDetails()
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

void CQueueListCtrl::OnNMDblclk(NMHDR* /*pNMHDR*/, LRESULT *pResult)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1) {
		CUpDownClient* client = (CUpDownClient*)GetItemData(iSel);
		if (client){
			CClientDetailDialog dialog(client, this);
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
			const CUpDownClient* pClient = reinterpret_cast<CUpDownClient*>(pDispInfo->item.lParam);
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
