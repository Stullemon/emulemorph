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
CDownloadClientsCtrl::CDownloadClientsCtrl()
	: CListCtrlItemWalk(this)
{
}

BEGIN_MESSAGE_MAP(CDownloadClientsCtrl, CMuleListCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclkDownloadClientlist)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
END_MESSAGE_MAP()

void CDownloadClientsCtrl::Init()
{
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
	LoadSettings(CPreferences::tableDownloadClients);

	// Barry - Use preferred sort order from preferences
	int sortItem = thePrefs.GetColumnSortItem(CPreferences::tableDownloadClients);
	bool sortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableDownloadClients);
	SetSortArrow(sortItem, sortAscending);
	// SLUGFILLER: multiSort - load multiple params
	for (int i = thePrefs.GetColumnSortCount(CPreferences::tableDownloadClients); i > 0; ) {
		i--;
		sortItem = thePrefs.GetColumnSortItem(CPreferences::tableDownloadClients, i);
		sortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableDownloadClients, i);
		SortItems(SortProc, sortItem + (sortAscending ? 0:100));
	}
	// SLUGFILLER: multiSort
	
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
	m_ImageList.Add(CTempIconLoader(_T("ClientEDonkeyPlus")));
	m_ImageList.Add(CTempIconLoader(_T("ClientCompatiblePlus")));
	m_ImageList.Add(CTempIconLoader(_T("Friend")));
	m_ImageList.Add(CTempIconLoader(_T("ClientMLDonkey")));
	m_ImageList.Add(CTempIconLoader(_T("ClientMLDonkeyPlus")));
	m_ImageList.Add(CTempIconLoader(_T("ClientEDonkeyHybrid")));
	m_ImageList.Add(CTempIconLoader(_T("ClientEDonkeyHybridPlus")));
	m_ImageList.Add(CTempIconLoader(_T("ClientShareaza")));
	m_ImageList.Add(CTempIconLoader(_T("ClientShareazaPlus")));
	m_ImageList.Add(CTempIconLoader(_T("ClientAMule")));
	m_ImageList.Add(CTempIconLoader(_T("ClientAMulePlus")));
	m_ImageList.Add(CTempIconLoader(_T("ClientLPhant")));
	m_ImageList.Add(CTempIconLoader(_T("ClientLPhantPlus")));
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("ClientSecureOvl"))), 1);
	//MORPH START - Added by SiRoB, More client & Credit Overlay Icon
	m_ImageList.Add(CTempIconLoader(_T("ClientRightEdonkey")));
	m_ImageList.Add(CTempIconLoader(_T("Morph")));
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("ClientCreditOvl"))), 2);
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("ClientCreditSecureOvl"))), 3);
	//MORPH END   - Added by SiRoB, More client & Credit Overlay Icon

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
}

void CDownloadClientsCtrl::Localize()
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

	strRes = GetResString(IDS_QL_USERNAME);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(0, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_CD_CSOFT);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(1, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_FILE);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(2, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_DL_SPEED);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(3, &hdi);
	strRes.ReleaseBuffer();
	strRes = GetResString(IDS_AVAILABLEPARTS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(4, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_CL_TRANSFDOWN);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(5, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_CL_TRANSFUP);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(6, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_META_SRCTYPE);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(7, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_DL_ULDL);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(8, &hdi);
	strRes.ReleaseBuffer();

	//SLAHAM: ADDED Last Asked =>
	strRes = GetResString(IDS_LAST_ASKED);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(9, &hdi);
	strRes.ReleaseBuffer();
	//SLAHAM: ADDED Last Asked <=

	//SLAHAM: ADDED Downloading Time =>
	strRes = GetResString(IDS_DOWNL_TIME);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(10, &hdi);
	strRes.ReleaseBuffer();
	//SLAHAM: ADDED Downloading Time <=

	//SLAHAM: ADDED Known Since =>
	strRes = GetResString(IDS_KNOWN_SINCE);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(11, &hdi);
	strRes.ReleaseBuffer();
	//SLAHAM: ADDED Known Since <=

	//MORPH START - Added by SiRoB, IP2Country column
	strRes = GetResString(IDS_COUNTRY);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(12, &hdi);
	strRes.ReleaseBuffer();
	//MORPH END   - Added by SiRoB, IP2Country column
}

void CDownloadClientsCtrl::AddClient(CUpDownClient* client)
{
	if(!theApp.emuledlg->IsRunning())
		return;
       
	InsertItem(LVIF_TEXT|LVIF_PARAM, GetItemCount(), client->GetUserName(), 0, 0, 1, (LPARAM)client);
	RefreshClient(client);
	theApp.emuledlg->transferwnd->UpdateListCount(CTransferWnd::wnd2Downloading, GetItemCount()); 
}

void CDownloadClientsCtrl::RemoveClient(CUpDownClient* client)
{
	if(!theApp.emuledlg->IsRunning())
		return;

	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	sint32 result = FindItem(&find);
	if (result != (-1) )
		DeleteItem(result);
	theApp.emuledlg->transferwnd->UpdateListCount(CTransferWnd::wnd2Downloading, GetItemCount()); 
}

void CDownloadClientsCtrl::RefreshClient(CUpDownClient* client)
{
	if( !theApp.emuledlg->IsRunning() )
		return;
	
	//MORPH START - SiRoB, Don't Refresh item if not needed 
	if( theApp.emuledlg->activewnd != theApp.emuledlg->transferwnd  || theApp.emuledlg->transferwnd->downloadclientsctrl.IsWindowVisible() == false ) 
		return; 
	//MORPH END   - SiRoB, Don't Refresh item if not needed 
	
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	sint16 result = FindItem(&find);
	if(result != -1)
		Update(result);
	return;
}

#define DLC_DT_TEXT (DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)

void CDownloadClientsCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
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
	CMemDC dc(CDC::FromHandle(lpDrawItemStruct->hDC),&lpDrawItemStruct->rcItem);
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
				dc->SetTextColor(m_crWindowText);
				switch(iColumn){
				case 0:	{
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
					else if (client->ExtProtocolAvailable()){
						image = client->IsMorph()?17:1; //MORPH START - Added by SiRoB, More client icon
					}
					else if (client->GetClientSoft() == SO_EDONKEY){
						image = 15;
					}
					else{
						image = 0;
					}

					UINT uOvlImg = INDEXTOOVERLAYMASK(((client->Credits() && client->Credits()->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED) ? 1 : 0) | ((client->Credits() && client->Credits()->GetMyScoreRatio(client->GetIP()) > 1) ? 2 : 0));
					POINT point = {cur_rec.left, cur_rec.top+1};
					/*
					m_ImageList.Draw(dc,image, point, ILD_NORMAL | ((client->Credits() && client->Credits()->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED) ? INDEXTOOVERLAYMASK(1) : 0));
					*/
					m_ImageList.Draw(dc,image, point, ILD_NORMAL | uOvlImg);
					// Mighty Knife: Community visualization
					if (client->IsCommunity())
						m_overlayimages.Draw(dc,0, point, ILD_TRANSPARENT);
					// [end] Mighty Knife
					//MORPH START - Added by SiRoB, Friend Addon
					if (client->IsFriend())
						m_overlayimages.Draw(dc, client->GetFriendSlot()?2:1,point, ILD_TRANSPARENT);
					//MORPH END   - Added by SiRoB, Friend Addon
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
					dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
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
					{
						Sbuffer.Format(_T("%.2f KB/s"), (float)client->GetDownloadDatarate()/1024);
						dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec, DLC_DT_TEXT | DT_RIGHT);
						break;
					}
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
								COLORREF oldclr = dc->SetTextColor(RGB(0,0,0));
								int iOMode = dc.SetBkMode(TRANSPARENT);
								buffer.Format(_T("%.1f%%"), percent);
								CFont *pOldFont = dc->SelectObject(&theApp.emuledlg->transferwnd->downloadlistctrl.m_fontBoldSmaller);
	#define	DrawClientPercentText		dc->DrawText(buffer, buffer.GetLength(),&cur_rec, ((DLC_DT_TEXT | DT_RIGHT) & ~DT_LEFT) | DT_CENTER)
								cur_rec.top-=1;cur_rec.bottom-=1;
								DrawClientPercentText;cur_rec.left+=1;cur_rec.right+=1;
								DrawClientPercentText;cur_rec.left+=1;cur_rec.right+=1;
								DrawClientPercentText;cur_rec.top+=1;cur_rec.bottom+=1;
								DrawClientPercentText;cur_rec.top+=1;cur_rec.bottom+=1;
								DrawClientPercentText;cur_rec.left-=1;cur_rec.right-=1;
								DrawClientPercentText;cur_rec.left-=1;cur_rec.right-=1;
								DrawClientPercentText;cur_rec.top-=1;cur_rec.bottom-=1;
								DrawClientPercentText;cur_rec.left++;cur_rec.right++;
								dc->SetTextColor(RGB(255,255,255));
								DrawClientPercentText;
								dc->SelectObject(pOldFont);
								dc->SetBkMode(iOMode);
								dc->SetTextColor(oldclr);
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
							Sbuffer = "eD2K Server";
							break;
						case SF_KADEMLIA:
							Sbuffer = GetResString(IDS_KADEMLIA);
							break;
						case SF_SOURCE_EXCHANGE:
							Sbuffer = GetResString(IDS_SE);
							break;
						case SF_PASSIVE:
							Sbuffer = GetResString(IDS_PASSIVE);
							break;
						case SF_LINK:
							Sbuffer = GetResString(IDS_SW_LINK);
							break;
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
					dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec, DLC_DT_TEXT);
					break;
					//SLAHAM: ADDED Last Asked Counter <=
					//SLAHAM: ADDED Show Downloading Time =>
				case 10:	
					Sbuffer.Format(_T(" %s (%s)[%u]"), CastSecondsToHM(client->dwSessionDLTime/1000), CastSecondsToHM(client->dwTotalDLTime/1000), client->uiStartDLCount);
					dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec, DLC_DT_TEXT);
					break;
					//SLAHAM: ADDED Show Downloading Time <=
					//SLAHAM: ADDED Known Since =>
				case 11: 
					if( client->dwThisClientIsKnownSince )
						Sbuffer.Format(_T(" %s"), CastSecondsToHM((::GetTickCount()-client->dwThisClientIsKnownSince)/1000));
					else
						Sbuffer.Format(_T("WHO IS THAT???"));
					dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec, DLC_DT_TEXT);
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
						dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
						if(theApp.ip2country->ShowCountryFlag()){
							cur_rec.left-=20;
						}
						break;
				// EastShare - Added by Pretender: IP2Country column
				}

				if( iColumn != 4 && iColumn != 0 && iColumn != 3 && iColumn != 12)
				dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
			}//MORPH - Added by SiRoB, Don't draw hidden columns
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

void CDownloadClientsCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult){

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int oldSortItem = thePrefs.GetColumnSortItem(CPreferences::tableDownloadClients); 

	bool m_oldSortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableDownloadClients);
	bool sortAscending = (oldSortItem != pNMListView->iSubItem) ? (pNMListView->iSubItem == 0) : !m_oldSortAscending;	

	// Item is column clicked
	int sortItem = pNMListView->iSubItem; 

	// Save new preferences
	thePrefs.SetColumnSortItem(CPreferences::tableDownloadClients, sortItem);
	thePrefs.SetColumnSortAscending(CPreferences::tableDownloadClients, sortAscending);
	// Sort table
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));

	*pResult = 0;
}

BOOL CDownloadClientsCtrl::OnCommand(WPARAM wParam,LPARAM lParam ){
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
				CClientDetailDialog dialog(client, this);
				dialog.DoModal();
				break;
			}
			case MP_BOOT:
				if (client->GetKadPort())
					Kademlia::CKademlia::bootstrap(ntohl(client->GetIP()), client->GetKadPort());
				break;
			case MP_REMOVEFRIEND:{ //LSD
				if (client && client->IsFriend()){					
					theApp.friendlist->RemoveFriend(client->m_Friend);
					theApp.emuledlg->transferwnd->downloadlistctrl.UpdateItem(client);
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
					}else 
					{
						client->SetFriendSlot(true);
					}
					theApp.friendlist->ShowFriends();
					theApp.emuledlg->transferwnd->downloadlistctrl.UpdateItem(client);
				}
				//MORPH END - Modified by SIRoB, Added by Yun.SF3, ZZ Upload System
				break;
			}

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
		case 1:
			if(item1->GetClientSoft() == item2->GetClientSoft())
				if(item2->GetVersion() == item1->GetVersion() && item1->GetClientSoft() == SO_EMULE){
					return CompareOptLocaleStringNoCase(item2->GetClientSoftVer(), item1->GetClientSoftVer());
				}
				else {
					return item2->GetVersion() - item1->GetVersion();
				}
			else
				return item1->GetClientSoft() - item2->GetClientSoft();
		case 101:
			if(item1->GetClientSoft() == item2->GetClientSoft())
				if(item2->GetVersion() == item1->GetVersion() && item1->GetClientSoft() == SO_EMULE){
					return CompareOptLocaleStringNoCase(item1->GetClientSoftVer(), item2->GetClientSoftVer());
				}
				else {
					return item1->GetVersion() - item2->GetVersion();
				}
			else
				return item2->GetClientSoft() - item1->GetClientSoft();
		case 2: {
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			if( (file1 != NULL) && (file2 != NULL))
				return CompareLocaleStringNoCase(file1->GetFileName(), file2->GetFileName());
			else if( file1 == NULL )
				return 1;
			else
				return -1;
		}
		case 102:{
			CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
			CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
			if( (file1 != NULL) && (file2 != NULL))
				return CompareLocaleStringNoCase(file2->GetFileName(), file1->GetFileName());
			else if( file1 == NULL )
				return 1;
			else
				return -1;
		}
		case 3:
			return CompareUnsigned(item2->GetDownloadDatarate(), item1->GetDownloadDatarate());
		case 103:
			return CompareUnsigned(item1->GetDownloadDatarate(), item2->GetDownloadDatarate());
		case 4:
			return CompareUnsigned(item2->GetPartCount(), item1->GetPartCount());
		case 104: 
			return CompareUnsigned(item1->GetPartCount(), item2->GetPartCount());
		case 5:
			return CompareUnsigned(item2->GetSessionDown(), item1->GetSessionDown());
		case 105:
			return CompareUnsigned(item1->GetSessionDown(), item2->GetSessionDown());
		case 6:
			return CompareUnsigned(item2->GetSessionUp(), item1->GetSessionUp());
		case 106:
			return CompareUnsigned(item1->GetSessionUp(), item2->GetSessionUp());
		case 8:
			{   
				if (!item1->Credits()) 
					return 1; 
				else if (!item2->Credits())   
					return -1; 
				float r1=item2->credits->GetScoreRatio(item2->GetIP());
				float r2=item1->credits->GetScoreRatio(item1->GetIP());
				return r1==r2? 0 : r1<r2? -1 : 1;
			}
		case 108:
			{   
				if (!item2->Credits()) 
					return 1; 
				else if (!item1->Credits())   
					return -1;  
	
				float r1=item1->credits->GetScoreRatio(item1->GetIP());   
				float r2=item2->credits->GetScoreRatio(item2->GetIP()); 
				return r1==r2? 0 : r1<r2? -1 : 1;
			}
		//SLAHAM: ADDED Last Asked =>
		case 9: 
			{
				uint32 lastAskedTime1 = item2->GetLastAskedTime();
				uint32 lastAskedTime2 = item1->GetLastAskedTime();
				return lastAskedTime1==lastAskedTime2? 0 : lastAskedTime1<lastAskedTime2? -1 : 1;
			}
		case 109:
			{
				uint32 lastAskedTime1 = item1->GetLastAskedTime();
				uint32 lastAskedTime2 = item2->GetLastAskedTime();
				return lastAskedTime1==lastAskedTime2? 0 : lastAskedTime1<lastAskedTime2? -1 : 1;
			}
		//SLAHAM: ADDED Last Asked <=
		//SLAHAM: ADDED Show Downloading Time =>
		case 10:
			return CompareUnsigned(item2->dwSessionDLTime, item1->dwSessionDLTime);
		case 110:
			return CompareUnsigned(item1->dwTotalDLTime, item2->dwTotalDLTime);
		//SLAHAM: ADDED Show Downloading Time <=
		//SLAHAM: ADDED Known Since =>
		case 11:
			{
				uint32 known1 = item2->dwThisClientIsKnownSince;
				uint32 known2 = item1->dwThisClientIsKnownSince;
				return known1==known2? 0 : known1<known2? -1 : 1;
			}
		case 111:
			{
				uint32 known1 = item1->dwThisClientIsKnownSince;
				uint32 known2 = item2->dwThisClientIsKnownSince;
				return known1==known2? 0 : known1<known2? -1 : 1;
			}
			//SLAHAM: ADDED Known Since <=

		// EastShare - Added by Pretender: IP2Country column
        case 12:
			if(item1->GetCountryName(true) && item2->GetCountryName(true))
				return CompareLocaleStringNoCase(item1->GetCountryName(true), item2->GetCountryName(true));
			else if(item1->GetCountryName(true))
				return 1;
			else
				return -1;

		case 112:
			if(item1->GetCountryName(true) && item2->GetCountryName(true))
				return CompareLocaleStringNoCase(item2->GetCountryName(true), item1->GetCountryName(true));
			else if(item2->GetCountryName(true))
				return 1;
			else
				return -1;
	// EastShare - Added by Pretender: IP2Country column		

	default:
		return 0;
	}
}

//SLAHAM: ADDED [TPT] - New Menu Styles =>
void CDownloadClientsCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
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
//SLAHAM: ADDED [TPT] - New Menu Styles <=

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

void CDownloadClientsCtrl::OnNMDblclkDownloadClientlist(NMHDR *pNMHDR, LRESULT *pResult) {
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1){
		CUpDownClient* client = (CUpDownClient*)GetItemData(iSel);
		if (client){
			CClientDetailDialog dialog(client,this);
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