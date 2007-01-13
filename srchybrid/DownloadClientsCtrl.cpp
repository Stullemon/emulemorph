//--- xrmb:downloadclientslist ---

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

//SLAHAM: ADDED/MODIFIED DownloadClientsCtrl =>

#include "stdafx.h"
#include "emule.h"
#include "emuledlg.h"
#include "DownloadClientsCtrl.h"
#include "ClientDetailDialog.h"
#include "MemDC.h"
#include "MenuCmds.h"
#include "FriendList.h"
#include "TransferWnd.h"
#include "ChatWnd.h"
#include "UpDownClient.h"
#include "UploadQueue.h"
#include "ClientCredits.h"
#include "PartFile.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "SharedFileList.h"
#include "IP2Country.h" //EastShare - added by AndCycle, IP to Country

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


IMPLEMENT_DYNAMIC(CDownloadClientsCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CDownloadClientsCtrl, CMuleListCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclkDownloadClientlist)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
END_MESSAGE_MAP()

CDownloadClientsCtrl::CDownloadClientsCtrl()
	: CListCtrlItemWalk(this)
{
	SetGeneralPurposeFind(true, false);
}

void CDownloadClientsCtrl::Init()
{
	SetName(_T("DownloadClientsCtrl"));
	CImageList ilDummyImageList; //dummy list for getting the proper height of listview entries
	ilDummyImageList.Create(1, theApp.GetSmallSytemIconSize().cy,theApp.m_iDfltImageListColorFlags|ILC_MASK, 1, 1); 
	SetImageList(&ilDummyImageList, LVSIL_SMALL);
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	ilDummyImageList.Detach();

	SetExtendedStyle(LVS_EX_FULLROWSELECT);

	InsertColumn(0,	GetResString(IDS_QL_USERNAME), LVCFMT_LEFT, 165);
	InsertColumn(1,	GetResString(IDS_CD_CSOFT), LVCFMT_LEFT, 90); 
	InsertColumn(2,	GetResString(IDS_FILE), LVCFMT_LEFT, 235);
	InsertColumn(3,	GetResString(IDS_DL_SPEED), LVCFMT_LEFT, 65);
	InsertColumn(4, GetResString(IDS_AVAILABLEPARTS), LVCFMT_LEFT, 150);
	InsertColumn(5,	GetResString(IDS_CL_TRANSFDOWN), LVCFMT_LEFT, 115);
	InsertColumn(6,	GetResString(IDS_CL_TRANSFUP), LVCFMT_LEFT, 115);
	InsertColumn(7,	GetResString(IDS_META_SRCTYPE), LVCFMT_LEFT, 60);
	InsertColumn(8,	GetResString(IDS_DL_ULDL), LVCFMT_LEFT, 60);
	InsertColumn(9, GetResString(IDS_LAST_ASKED), LVCFMT_LEFT, 100); //SLAHAM: ADDED Last Asked 
	InsertColumn(10, GetResString(IDS_DOWNL_TIME), LVCFMT_LEFT, 100); //SLAHAM: ADDED Downloading Time
	InsertColumn(11, GetResString(IDS_KNOWN_SINCE),LVCFMT_LEFT, 100); //SLAHAM: ADDED Known Since

	// EastShare - Added by Pretender: IP2Country column
	InsertColumn(12,GetResString(IDS_COUNTRY),LVCFMT_LEFT,50);
	// EastShare - Added by Pretender: IP2Country column

	SetAllIcons();
	Localize();
	LoadSettings();

	// Barry - Use preferred sort order from preferences
	SetSortArrow();
	SortItems(SortProc, GetSortItem()+ (GetSortAscending()? 0:100));
	
	//MORPH START - Added by SiRoB, IP2Country column
	if (thePrefs.GetIP2CountryNameMode() == IP2CountryName_DISABLE)
		HideColumn (12);
	//MORPH END   - Added by SiRoB, IP2Country column
}

CDownloadClientsCtrl::~CDownloadClientsCtrl()
{
}

void CDownloadClientsCtrl::SetAllIcons() 
{
	m_ImageList.DeleteImageList();
	m_ImageList.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	m_ImageList.SetBkColor(CLR_NONE);
	m_ImageList.Add(CTempIconLoader(_T("ClientEDonkey")));
	m_ImageList.Add(CTempIconLoader(_T("ClientCompatible")));
	m_ImageList.Add(CTempIconLoader(_T("ClientEDonkey")));
	m_ImageList.Add(CTempIconLoader(_T("ClientCompatible")));
	m_ImageList.Add(CTempIconLoader(_T("Friend")));
	m_ImageList.Add(CTempIconLoader(_T("ClientMLDonkey")));
	m_ImageList.Add(CTempIconLoader(_T("ClientMLDonkey")));
	m_ImageList.Add(CTempIconLoader(_T("ClientEDonkeyHybrid")));
	m_ImageList.Add(CTempIconLoader(_T("ClientEDonkeyHybrid")));
	m_ImageList.Add(CTempIconLoader(_T("ClientShareaza")));
	m_ImageList.Add(CTempIconLoader(_T("ClientShareaza")));
	m_ImageList.Add(CTempIconLoader(_T("ClientAMule")));
	m_ImageList.Add(CTempIconLoader(_T("ClientAMule")));
	m_ImageList.Add(CTempIconLoader(_T("ClientLPhant")));
	m_ImageList.Add(CTempIconLoader(_T("ClientLPhant")));
	//MORPH START - Added by SiRoB, More client & Credit Overlay Icon
	m_ImageList.Add(CTempIconLoader(_T("ClientRightEdonkey")));
	m_ImageList.Add(CTempIconLoader(_T("WEBCACHE")));
	m_ImageList.Add(CTempIconLoader(_T("Server")));
	m_ImageList.Add(CTempIconLoader(_T("Morph")));
	m_ImageList.Add(CTempIconLoader(_T("SCARANGEL")));
	m_ImageList.Add(CTempIconLoader(_T("STULLE")));
	m_ImageList.Add(CTempIconLoader(_T("MAXMOD")));
	m_ImageList.Add(CTempIconLoader(_T("XTREME")));
	m_ImageList.Add(CTempIconLoader(_T("EASTSHARE")));
	m_ImageList.Add(CTempIconLoader(_T("IONIX")));
	m_ImageList.Add(CTempIconLoader(_T("CYREX")));
	m_ImageList.Add(CTempIconLoader(_T("NEXTEMF")));
	m_ImageList.Add(CTempIconLoader(_T("NEO")));
	//MORPH END   - Added by SiRoB, More client & Credit Overlay Icon
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("ClientSecureOvl"))), 1);
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("OverlayObfu"))), 2);
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("OverlaySecureObfu"))), 3);
	
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

void CDownloadClientsCtrl::Localize()
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

	strRes = GetResString(IDS_QL_USERNAME);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(0, &hdi);

	strRes = GetResString(IDS_CD_CSOFT);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(1, &hdi);

	strRes = GetResString(IDS_FILE);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(2, &hdi);

	strRes = GetResString(IDS_DL_SPEED);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(3, &hdi);

	strRes = GetResString(IDS_AVAILABLEPARTS);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(4, &hdi);

	strRes = GetResString(IDS_CL_TRANSFDOWN);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(5, &hdi);

	strRes = GetResString(IDS_CL_TRANSFUP);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(6, &hdi);

	strRes = GetResString(IDS_META_SRCTYPE);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(7, &hdi);

	strRes = GetResString(IDS_DL_ULDL);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(8, &hdi);

	//SLAHAM: ADDED Last Asked =>
	strRes = GetResString(IDS_LAST_ASKED);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(9, &hdi);
	//SLAHAM: ADDED Last Asked <=

	//SLAHAM: ADDED Downloading Time =>
	strRes = GetResString(IDS_DOWNL_TIME);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(10, &hdi);
	//SLAHAM: ADDED Downloading Time <=

	//SLAHAM: ADDED Known Since =>
	strRes = GetResString(IDS_KNOWN_SINCE);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(11, &hdi);
	//SLAHAM: ADDED Known Since <=

	//MORPH START - Added by SiRoB, IP2Country column
	strRes = GetResString(IDS_COUNTRY);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(12, &hdi);
	//MORPH END   - Added by SiRoB, IP2Country column
}

void CDownloadClientsCtrl::AddClient(CUpDownClient* client)
{
	if (theApp.IsRunningAsService()) return;// MORPH leuk_he:run as ntservice v1..

	if(!theApp.emuledlg->IsRunning())
		return;
       
	//MORPH START - Changed by SiRoB, fix by XMan
	/*
	InsertItem(LVIF_TEXT|LVIF_PARAM, GetItemCount(), client->GetUserName(), 0, 0, 1, (LPARAM)client);
	RefreshClient(client);
	*/
	int item = InsertItem(LVIF_TEXT|LVIF_PARAM, GetItemCount(), client->GetUserName(), 0, 0, 1, (LPARAM)client);
	Update(item);
	//RefreshClient(client);
	//MORPH END  - Changed by SiRoB, fix by XMan
	
	theApp.emuledlg->transferwnd->UpdateListCount(CTransferWnd::wnd2Downloading, GetItemCount()); 
}

void CDownloadClientsCtrl::RemoveClient(CUpDownClient* client)
{
	if(!theApp.emuledlg->IsRunning())
		return;

	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	int result = FindItem(&find);
	if (result != -1)
		DeleteItem(result);
	theApp.emuledlg->transferwnd->UpdateListCount(CTransferWnd::wnd2Downloading, GetItemCount()); 
}

void CDownloadClientsCtrl::RefreshClient(CUpDownClient* client)
{
	if (theApp.IsRunningAsService()) return;// MORPH leuk_he:run as ntservice v1..
	
	if( !theApp.emuledlg->IsRunning() )
		return;

	//MORPH START - SiRoB, Don't Refresh item if not needed 
	if( theApp.emuledlg->activewnd != theApp.emuledlg->transferwnd  || theApp.emuledlg->transferwnd->downloadclientsctrl.IsWindowVisible() == false ) 
		return; 
	//MORPH END   - SiRoB, Don't Refresh item if not needed 

	//MORPH START - UpdateItemThread
	/*
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	int result = FindItem(&find);
	if(result != -1)
		Update(result);
	*/
	m_updatethread->AddItemToUpdate((LPARAM)client);
	//MORPH END - UpdateItemThread
}

#define DLC_DT_TEXT (DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)

void CDownloadClientsCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (!theApp.emuledlg->IsRunning())
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
	BOOL bCtrlFocused = ((GetFocus() == this) || (GetStyle() & LVS_SHOWSELALWAYS));
	if (lpDrawItemStruct->itemState & ODS_SELECTED) {
		if (bCtrlFocused)
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
	if(client->GetSlotNumber() > theApp.uploadqueue->GetActiveUploadsCount(client->GetClassID())) { //MORPH - Upload Splitting Class
        dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
    }

	int iOldBkMode;
	if (m_crWindowTextBk == CLR_NONE){
		DefWindowProc(WM_ERASEBKGND, (WPARAM)(HDC)dc, 0);
		iOldBkMode = dc.SetBkMode(TRANSPARENT);
	}
	else
		iOldBkMode = OPAQUE;

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
					uint8 image;
					
					//MORPH - Removed by SiRoB, Friend Addon
					/*
					/*if (client->IsFriend())
						image = 4;
					else*/ if (client->GetClientSoft() == SO_EDONKEYHYBRID){
						image = 7;
					}
					else if (client->GetClientSoft() == SO_MLDONKEY){
						image = 5;
					}
					else if (client->GetClientSoft() == SO_SHAREAZA){
						image = 9;
					}
					else if (client->GetClientSoft() == SO_AMULE){
						image = 11;
					}
					else if (client->GetClientSoft() == SO_LPHANT){
						image = 13;
					}
					//MORPH START - Added by Stulle, Server Icon
					else if(client->GetClientSoft() == SO_URL){
						image = 17;
					}
					//MORPH END   - Added by Stulle, Server Icon
					//MORPH START - Added by SiRoB, WebCache
					else if (client->GetClientSoft() == SO_WEBCACHE) {
						image = 16;
					}
					//MORPH END   - Added by SiRoB, WebCache
					else if (client->ExtProtocolAvailable()){
						//MORPH START - Added by SiRoB, More client icon
						if(client->GetModClient() == MOD_NONE)
							image = 1;
						else
							image = (uint8)(client->GetModClient() + 17);
						//MORPH END   - Added by SiRoB, More client icon
					}
					else if (client->GetClientSoft() == SO_EDONKEY){
						image = 15;
					}
					else
						image = 0;

					uint32 nOverlayImage = 0;
					if ((client->Credits() && client->Credits()->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED))
						nOverlayImage |= 1;
					if (client->IsObfuscatedConnectionEstablished())
						nOverlayImage |= 2;
					POINT point = {cur_rec.left, cur_rec.top+1};
					m_ImageList.Draw(dc,image, point, ILD_NORMAL | INDEXTOOVERLAYMASK(nOverlayImage));
					
					//MORPH START - Credit Overlay Icon
					if (client->Credits() && client->Credits()->GetMyScoreRatio(client->GetIP()) > 1) {
						if (nOverlayImage & 1)
							m_overlayimages.Draw(dc, 4, point, ILD_TRANSPARENT);
						else
							m_overlayimages.Draw(dc, 3, point, ILD_TRANSPARENT);
					}
					//MORPH END - Credit Overlay Icon
					//MORPH START - Added by SiRoB, Friend Addon
					if (client->IsFriend())
						m_overlayimages.Draw(dc, client->GetFriendSlot()?2:1,point, ILD_TRANSPARENT);
					//MORPH END   - Added by SiRoB, Friend Addon
					// Mighty Knife: Community visualization
					if (client->IsCommunity())
						m_overlayimages.Draw(dc,0, point, ILD_TRANSPARENT);
					// [end] Mighty Knife
					//MORPH END - Modified by SiRoB, More client & ownCredits overlay icon

					Sbuffer = client->GetUserName();

					//EastShare Start - added by AndCycle, IP to Country 
					if(theApp.ip2country->ShowCountryFlag() && IsColumnHidden(12)){
						cur_rec.left+=20;
						POINT point2= {cur_rec.left,cur_rec.top+1};
						int index = client->GetCountryFlagIndex();
						theApp.ip2country->GetFlagImageList()->DrawIndirect(dc, index , point2, CSize(18,16), CPoint(0,0), ILD_NORMAL);
					}
					//EastShare End - added by AndCycle, IP to Country

					cur_rec.left +=20;
					dc.DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
					cur_rec.left -=20;

					//EastShare Start - added by AndCycle, IP to Country
					if(theApp.ip2country->ShowCountryFlag() && IsColumnHidden(12)){
						cur_rec.left-=20;
					}
					//EastShare End - added by AndCycle, IP to Country

					break;
					}
				case 1:
					Sbuffer.Format(_T("%s"), client->GetClientSoftVer());
					break;
				case 2:
					Sbuffer.Format(_T("%s"), client->GetRequestFile()->GetFileName());
					break;
				case 3:
					Sbuffer=CastItoXBytes( (float)client->GetDownloadDatarate() , false, true);
					dc.DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec, DLC_DT_TEXT | DT_RIGHT);
					break;
				case 4:
					{
						cur_rec.bottom--;
						cur_rec.top++;

						client->DrawStatusBar(dc, &cur_rec, client->GetRequestFile(), thePrefs.UseFlatBar());
						CString buffer;

						//SLAHAM: ADDED Client Percentage =>
						if (thePrefs.GetUseClientPercentage() && client->GetPartStatus())
						{
							float percent = (float)client->GetAvailablePartCount() / (float)client->GetPartCount()* 100.0f;
							buffer.Format(_T("%.1f%%"), percent);

							if (percent > 0.05f)
							{
								//Commander - Added: Draw Client Percentage xored, caching before draw - Start
								COLORREF oldclr = dc.SetTextColor(RGB(0,0,0));
								int iOMode = dc.SetBkMode(TRANSPARENT);
								buffer.Format(_T("%.1f%%"), percent);
								CFont *pOldFont = dc.SelectObject(&theApp.emuledlg->transferwnd->downloadlistctrl.m_fontBoldSmaller);
	#define	DrawClientPercentText		dc.DrawText(buffer, buffer.GetLength(),&cur_rec, ((DLC_DT_TEXT | DT_RIGHT) & ~DT_LEFT) | DT_CENTER)
								cur_rec.top-=1;cur_rec.bottom-=1;
								DrawClientPercentText;cur_rec.left+=1;cur_rec.right+=1;
								DrawClientPercentText;cur_rec.left+=1;cur_rec.right+=1;
								DrawClientPercentText;cur_rec.top+=1;cur_rec.bottom+=1;
								DrawClientPercentText;cur_rec.top+=1;cur_rec.bottom+=1;
								DrawClientPercentText;cur_rec.left-=1;cur_rec.right-=1;
								DrawClientPercentText;cur_rec.left-=1;cur_rec.right-=1;
								DrawClientPercentText;cur_rec.top-=1;cur_rec.bottom-=1;
								DrawClientPercentText;cur_rec.left++;cur_rec.right++;
								dc.SetTextColor(RGB(255,255,255));
								DrawClientPercentText;
								dc.SelectObject(pOldFont);
								dc.SetBkMode(iOMode);
								dc.SetTextColor(oldclr);
								//Commander - Added: Draw Client Percentage xored, caching before draw - End	
							}
							//SLAHAM: ADDED Client Percentage <=

							cur_rec.bottom++;
							cur_rec.top--;
						}
						break;
					}	
				case 5:
					if(client->Credits() && client->GetSessionDown() < client->credits->GetDownloadedTotal())
						Sbuffer.Format(_T("%s (%s)"), CastItoXBytes(client->GetSessionDown()), CastItoXBytes(client->credits->GetDownloadedTotal()));
					else
						Sbuffer.Format(_T("%s"), CastItoXBytes(client->GetSessionDown()));
					break;
				case 6:
					if(client->Credits() && client->GetSessionUp() < client->credits->GetUploadedTotal())
						Sbuffer.Format(_T("%s (%s)"), CastItoXBytes(client->GetSessionUp()), CastItoXBytes(client->credits->GetUploadedTotal()));
					else
						Sbuffer.Format(_T("%s"), CastItoXBytes(client->GetSessionUp()));
					break;
				case 7:
						switch(client->GetSourceFrom()){
						case SF_SERVER:
							Sbuffer = _T("eD2K Server");
							break;
						//MORPH - Source cache
						case SF_CACHE_SERVER:
							Sbuffer = _T("eD2K Server (SC)");
							break;
						//MORPH - Source cache
						case SF_KADEMLIA:
							Sbuffer = GetResString(IDS_KADEMLIA);
							break;
						case SF_SOURCE_EXCHANGE:
							Sbuffer = GetResString(IDS_SE);
							break;
						//MORPH - Source cache
						case SF_CACHE_SOURCE_EXCHANGE:
							Sbuffer = GetResString(IDS_KADEMLIA) + _T(" (SC)");
							break;
						//MORPH - Source cache
						case SF_PASSIVE:
							Sbuffer = GetResString(IDS_PASSIVE);
							break;
						case SF_LINK:
							Sbuffer = GetResString(IDS_SW_LINK);
							break;
						//MORPH START - Added by SiRoB, Source Loader Saver [SLS]
						case SF_SLS:
							Sbuffer = GetResString(IDS_SOURCE_LOADER_SAVER); //ADDED IDS by FrankyFive
							break;
						//MORPH END   - Added by SiRoB, Source Loader Saver [SLS]
						default:
							Sbuffer = GetResString(IDS_UNKNOWN);
							break;
						}
					break;
				case 8:
					if(client->Credits())
						Sbuffer.Format(_T("%.1f/%.1f"),client->credits->GetScoreRatio(client->GetIP()), client->credits->GetMyScoreRatio(client->GetIP()));
					else
						Sbuffer.Format(_T(""));
					break;
					//SLAHAM: ADDED Last Asked Counter =>
				case 9:
					if( client->GetLastAskedTime() )
						Sbuffer.Format(_T(" %s [%u]"), CastSecondsToHM((::GetTickCount()-client->GetLastAskedTime())/1000), client->uiDLAskingCounter);
					else
						Sbuffer.Format(_T("Reseted [%u]"), client->uiDLAskingCounter);
					dc.DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec, DLC_DT_TEXT);
					break;
					//SLAHAM: ADDED Last Asked Counter <=
					//SLAHAM: ADDED Show Downloading Time =>
				case 10:	
					Sbuffer.Format(_T(" %s (%s)[%u]"), CastSecondsToHM(client->dwSessionDLTime/1000), CastSecondsToHM(client->dwTotalDLTime/1000), client->uiStartDLCount);
					dc.DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec, DLC_DT_TEXT);
					break;
					//SLAHAM: ADDED Show Downloading Time <=
					//SLAHAM: ADDED Known Since =>
				case 11: 
					if( client->dwThisClientIsKnownSince )
						Sbuffer.Format(_T(" %s"), CastSecondsToHM((::GetTickCount()-client->dwThisClientIsKnownSince)/1000));
					else
						Sbuffer.Format(_T("WHO IS THAT???"));
					dc.DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec, DLC_DT_TEXT);
					break;
					//SLAHAM: ADDED Known Since <=
				// EastShare - Added by Pretender: IP2Country column
				case 12:
						if(theApp.ip2country->ShowCountryFlag()){
							POINT point2= {cur_rec.left,cur_rec.top+1};
							int index = client->GetCountryFlagIndex();
							theApp.ip2country->GetFlagImageList()->DrawIndirect(dc, index , point2, CSize(18,16), CPoint(0,0), ILD_NORMAL);
							cur_rec.left+=20;
						}
						Sbuffer.Format(_T("%s"), client->GetCountryName());
						dc.DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
						if(theApp.ip2country->ShowCountryFlag()){
							cur_rec.left-=20;
						}
						break;
				// EastShare - Added by Pretender: IP2Country column
				}
				if( iColumn != 4 && iColumn != 0 && iColumn != 3 && iColumn != 12)
				dc.DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
			}//MORPH - Added by SiRoB, Don't draw hidden columns
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

		if (bCtrlFocused)
			dc.FrameRect(&outline_rec, &CBrush(m_crFocusLine));
		else
			dc.FrameRect(&outline_rec, &CBrush(m_crNoFocusLine));
	}

	if (m_crWindowTextBk == CLR_NONE)
		dc.SetBkMode(iOldBkMode);

	dc.SelectObject(pOldFont);
	dc.SetTextColor(crOldTextColor);
}

void CDownloadClientsCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult){

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	bool sortAscending = (GetSortItem()!= pNMListView->iSubItem) ? (pNMListView->iSubItem == 0) : !GetSortAscending();	

	// Sort table
	UpdateSortHistory(pNMListView->iSubItem + (sortAscending ? 0:100), 100);
	SetSortArrow(pNMListView->iSubItem, sortAscending);
	SortItems(SortProc, pNMListView->iSubItem + (sortAscending ? 0:100));

	*pResult = 0;
}

BOOL CDownloadClientsCtrl::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
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
					Kademlia::CKademlia::Bootstrap(ntohl(client->GetIP()), client->GetKadPort(), (client->GetKadVersion() > 1));
				break;
			//MORPH START - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System
			case MP_REMOVEFRIEND:{
				if (client && client->IsFriend()){					
					theApp.friendlist->RemoveFriend(client->m_Friend);
					theApp.emuledlg->transferwnd->downloadlistctrl.UpdateItem(client);
				}
				break;
			 }

			case MP_FRIENDSLOT:{
				if (client){
					bool IsAlready;
					IsAlready = client->GetFriendSlot();
					//theApp.friendlist->RemoveAllFriendSlots();
					if( IsAlready ) {
						client->SetFriendSlot(false);
					}else 
					{
						client->SetFriendSlot(true);
					}
					theApp.friendlist->ShowFriends();
					theApp.emuledlg->transferwnd->downloadlistctrl.UpdateItem(client);
				}
				break;
			}
			//MORPH END - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System
			//MORPH START - Added by Yun.SF3, List Requested Files
			case MP_LIST_REQUESTED_FILES:
				if (client != NULL)
					client->ShowRequestedFiles(); //Changed by SiRoB
				break;
			//MORPH END - Added by Yun.SF3, List Requested Files
		}
	}
	return true;
}

int CDownloadClientsCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort){
	const CUpDownClient* item1 = (CUpDownClient*)lParam1;
	const CUpDownClient* item2 = (CUpDownClient*)lParam2;
	int iResult=0;
	switch(lParamSort){
		case 0: 
		case 100:
			if(item1->GetUserName() && item2->GetUserName())
				iResult=CompareLocaleStringNoCase(item1->GetUserName(), item2->GetUserName());
			else if(item1->GetUserName())
				iResult=1;
			else
				iResult=-1;
			break;
		case 1:
	    case 101:
			if (item1->GetClientSoft() == item2->GetClientSoft())
				if (item2->GetVersion() == item1->GetVersion() && item1->GetClientSoft() == SO_EMULE)
					iResult=CompareOptLocaleStringNoCase(item2->GetClientSoftVer(), item1->GetClientSoftVer());
				else
					iResult=item2->GetVersion() - item1->GetVersion();
			else
				iResult=item1->GetClientSoft() - item2->GetClientSoft();
			break;
		case 2:
		case 102:
		{
			CKnownFile* file1 = item1->GetRequestFile();
			CKnownFile* file2 = item2->GetRequestFile();
			if( (file1 != NULL) && (file2 != NULL))
				iResult=CompareLocaleStringNoCase(file1->GetFileName(), file2->GetFileName());
			else if( file1 == NULL )
				iResult=1;
			else
				iResult=-1;
			break;
		}
		case 3:
		case 103:
			iResult=CompareUnsigned(item1->GetDownloadDatarate(), item2->GetDownloadDatarate());
			break;
		case 4:
		case 104: 
			iResult=CompareUnsigned(item1->GetPartCount(), item2->GetPartCount());
			break;
		case 5:
		case 105:
			iResult=CompareUnsigned(item1->GetSessionDown(), item2->GetSessionDown());
			break;
		case 6:
		case 106:
			iResult=CompareUnsigned(item1->GetSessionUp(), item2->GetSessionUp());
			break;
		case 7: 
		case 107: 
			iResult=CompareUnsigned(item1->GetSourceFrom(), item2->GetSourceFrom());
			break;
		case 8:
		case 108:
			{   
				if (!item1->Credits()) 
					iResult=1; 
				else if (!item2->Credits())   
					iResult=-1;
				float r1=item2->credits->GetScoreRatio(item2->GetIP());
				float r2=item1->credits->GetScoreRatio(item1->GetIP());
				if (r1==r2)
					iResult=0;
				else if (r1<r2)
					iResult=-1;
				else
					iResult=1;
				break;
			}
		//SLAHAM: ADDED Last Asked =>
		case 9: 
		case 109:
			{
				uint32 lastAskedTime1 = item2->GetLastAskedTime();
				uint32 lastAskedTime2 = item1->GetLastAskedTime();
				iResult=lastAskedTime1==lastAskedTime2? 0 : lastAskedTime1<lastAskedTime2? -1 : 1;
				break;
			}
		//SLAHAM: ADDED Last Asked <=
		//SLAHAM: ADDED Show Downloading Time =>
		case 10:
		case 110:
			iResult=CompareUnsigned(item2->dwSessionDLTime, item1->dwSessionDLTime);
			break;
		//SLAHAM: ADDED Show Downloading Time <=
		//SLAHAM: ADDED Known Since =>
		case 11:
		case 111:
			{
				uint32 known1 = item2->dwThisClientIsKnownSince;
				uint32 known2 = item1->dwThisClientIsKnownSince;
				iResult=known1==known2? 0 : known1<known2? -1 : 1;
				break;
			}
			//SLAHAM: ADDED Known Since <=

		// EastShare - Added by Pretender: IP2Country column
        case 12:
		case 112:
			if(item1->GetCountryName(true) && item2->GetCountryName(true))
				iResult=CompareLocaleStringNoCase(item1->GetCountryName(true), item2->GetCountryName(true));
			else if(item1->GetCountryName(true))
				iResult=1;
			else
				iResult=-1;
			break;
	// EastShare - Added by Pretender: IP2Country column		

		default:
			iResult=0;
			break;
	}

	if (lParamSort>=100)
    iResult*=-1;

	// SLUGFILLER: multiSort remove - handled in parent class
	/*
	int dwNextSort;
	//call secondary sortorder, if this one results in equal
	//(Note: yes I know this call is evil OO wise, but better than changing a lot more code, while we have only one instance anyway - might be fixed later)
	if (iResult == 0 && (dwNextSort = theApp.emuledlg->transferwnd->downloadclientsctrl.GetNextSortOrder(lParamSort)) != (-1)){
		iResult= SortProc(lParam1, lParam2, dwNextSort);
	}
	*/
	return iResult;
}

//SLAHAM: ADDED [TPT] - New Menu Styles =>
void CDownloadClientsCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
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
	if (Kademlia::CKademlia::IsRunning() && !Kademlia::CKademlia::IsConnected())
		ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetKadPort()!=0) ? MF_ENABLED : MF_GRAYED), MP_BOOT, GetResString(IDS_BOOTSTRAP));
	//MORPH START - Added by Yun.SF3, List Requested Files
	ClientMenu.AppendMenu(MF_SEPARATOR); // Added by sivka
	ClientMenu.AppendMenu(MF_STRING | (client ? MF_ENABLED : MF_GRAYED),MP_LIST_REQUESTED_FILES, GetResString(IDS_LISTREQUESTED), _T("FILEREQUESTED")); // Added by sivka
	//MORPH END - Added by Yun.SF3, List Requested Files
	GetPopupMenuPos(*this, point);
	ClientMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
}

void CDownloadClientsCtrl::ShowSelectedUserDetails(){
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

void CDownloadClientsCtrl::OnNMDblclkDownloadClientlist(NMHDR* /*pNMHDR*/, LRESULT* pResult)
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

void CDownloadClientsCtrl::OnSysColorChange()
{
	CMuleListCtrl::OnSysColorChange();
	SetAllIcons();
}

void CDownloadClientsCtrl::OnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
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