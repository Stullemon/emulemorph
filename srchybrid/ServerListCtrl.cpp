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
#include <share.h>
#include "emule.h"
#include "ServerListCtrl.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "DownloadQueue.h"
#include "ServerList.h"
#include "Server.h"
#include "Sockets.h"
#include "MenuCmds.h"
#include "ServerWnd.h"
#include "IrcWnd.h"
#include "Opcodes.h"
#include "IP2Country.h" //EastShare - added by AndCycle, IP to Country
#include "MemDC.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CServerListCtrl, CMuleListCtrl)
// CServerListCtrl
BEGIN_MESSAGE_MAP(CServerListCtrl, CMuleListCtrl)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMLdblclk)
	ON_NOTIFY_REFLECT(LVN_GETINFOTIP, OnLvnGetInfoTip)
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

CServerListCtrl::CServerListCtrl()
{
	server_list = NULL;
	SetGeneralPurposeFind(true);
}

bool CServerListCtrl::Init(CServerList* in_list)
{
	server_list = in_list;
	ModifyStyle(0,LVS_SINGLESEL|LVS_REPORT);
	ModifyStyle(LVS_SINGLESEL|LVS_LIST|LVS_ICON|LVS_SMALLICON,LVS_REPORT); //here the CListCtrl is set to report-style
	SetExtendedStyle(GetExtendedStyle() | LVS_EX_INFOTIP);

	InsertColumn(0, GetResString(IDS_SL_SERVERNAME),LVCFMT_LEFT, 150);
	InsertColumn(1, GetResString(IDS_IP),			LVCFMT_LEFT, 140);
	InsertColumn(2, GetResString(IDS_DESCRIPTION),	LVCFMT_LEFT, 150);
	InsertColumn(3, GetResString(IDS_PING),			LVCFMT_RIGHT, 50);
	InsertColumn(4, GetResString(IDS_UUSERS),		LVCFMT_RIGHT, 50);
	InsertColumn(5, GetResString(IDS_MAXCLIENT),	LVCFMT_RIGHT, 50);
	InsertColumn(6, GetResString(IDS_PW_FILES) ,	LVCFMT_RIGHT, 50);
	InsertColumn(7, GetResString(IDS_PREFERENCE),	LVCFMT_LEFT,  60);
	InsertColumn(8, GetResString(IDS_UFAILED),		LVCFMT_RIGHT, 50);
	InsertColumn(9, GetResString(IDS_STATICSERVER),	LVCFMT_LEFT,  50);
	InsertColumn(10,GetResString(IDS_SOFTFILES),	LVCFMT_RIGHT, 50);
	InsertColumn(11,GetResString(IDS_HARDFILES),	LVCFMT_RIGHT, 50);
	InsertColumn(12,GetResString(IDS_VERSION),		LVCFMT_LEFT,  50);
	InsertColumn(13,GetResString(IDS_IDLOW),		LVCFMT_RIGHT, 50);
	InsertColumn(14,GetResString(IDS_AUXPORTS),		LVCFMT_LEFT,  50);//Morph - added by AndCycle, aux Ports, by lugdunummaster
    InsertColumn(15,GetResString(IDS_COUNTRY),      LVCFMT_LEFT, 100);// Commander - Added: IP2Country column

	SetAllIcons();
	Localize();
	LoadSettings(CPreferences::tableServer);

	// Barry - Use preferred sort order from preferences
	int iSortItem = thePrefs.GetColumnSortItem(CPreferences::tableServer);
	bool bSortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableServer);
	SetSortArrow(iSortItem, bSortAscending);
	// SLUGFILLER: multiSort - load multiple params
	for (int i = thePrefs.GetColumnSortCount(CPreferences::tableServer); i > 0; ) {
		i--;
		iSortItem = thePrefs.GetColumnSortItem(CPreferences::tableServer, i);
		bSortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableServer, i);
		SortItems(SortProc, MAKELONG(iSortItem, (bSortAscending ? 0 : 0x0001)));
	}
	// SLUGFILLER: multiSort
	ShowServerCount();

	return true;
} 

CServerListCtrl::~CServerListCtrl() {
}

void CServerListCtrl::OnSysColorChange()
{
	CMuleListCtrl::OnSysColorChange();
	SetAllIcons();
}

void CServerListCtrl::SetAllIcons()
{
	CImageList iml;
	iml.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	iml.SetBkColor(CLR_NONE);
	iml.Add(CTempIconLoader(_T("Server")));
	HIMAGELIST himl = ApplyImageList(iml.Detach());
	if (himl)
		ImageList_Destroy(himl);
	//MORPH START - Changed b SiRoB, CountryFlag Addon
	imagelist.DeleteImageList();
	imagelist.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	imagelist.SetBkColor(CLR_NONE);
	imagelist.Add(CTempIconLoader(_T("Server")));
	//MORPH END  - Changed b SiRoB, CountryFlag Addon
}

void CServerListCtrl::Localize()
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

	strRes = GetResString(IDS_SL_SERVERNAME);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(0, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_IP);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(1, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_DESCRIPTION);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(2, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_PING);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(3, &hdi);
	strRes.ReleaseBuffer();

 
	strRes = GetResString(IDS_UUSERS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(4, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_MAXCLIENT);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(5, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_PW_FILES);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(6, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_PREFERENCE);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(7, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_UFAILED);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(8, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_STATICSERVER);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(9, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_SOFTFILES);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(10, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_HARDFILES);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(11, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_VERSION);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(12, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_IDLOW);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(13, &hdi);
	strRes.ReleaseBuffer();

	//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
	strRes = GetResString(IDS_AUXPORTS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(14, &hdi);
	strRes.ReleaseBuffer();
	//Morph End - added by AndCycle, aux Ports, by lugdunummaster

	// Commander - Added: IP2Country column - Start
	strRes = GetResString(IDS_COUNTRY);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(15, &hdi);
	strRes.ReleaseBuffer();
    // Commander - Added: IP2Country column - End

	int iItems = GetItemCount();
	for (int i = 0; i < iItems; i++)
		RefreshServer((CServer*)GetItemData(i));
}

void CServerListCtrl::RemoveServer(CServer* todel)
{
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)todel;
	sint32 result = FindItem(&find);
	if (result != (-1) ){
		server_list->RemoveServer((CServer*)GetItemData(result));
		DeleteItem(result); 
		ShowServerCount();
	}
	return;
}

void CServerListCtrl::RemoveAllDeadServers()
{
	ShowWindow(SW_HIDE); 
	for(POSITION pos = server_list->list.GetHeadPosition(); pos != NULL;server_list->list.GetNext(pos)) { 
		CServer* cur_server = server_list->list.GetAt(pos); 
		if (cur_server->GetFailedCount() >= thePrefs.GetDeadserverRetries()){
			RemoveServer(cur_server);
			pos = server_list->list.GetHeadPosition();
		}
	}
   ShowWindow(SW_SHOW); 
}

bool CServerListCtrl::AddServer(CServer* toadd, bool bAddToList)
{
   if (!server_list->AddServer(toadd)) 
      return false; 
   if (bAddToList) 
   {
	   uint32 itemnr = GetItemCount();
	   InsertItem(LVIF_TEXT|LVIF_PARAM,itemnr,toadd->GetListName(),0,0,1,(LPARAM)toadd);
	   RefreshServer( toadd );
   }
	ShowServerCount();
   return true; 
}

void CServerListCtrl::RefreshServer(const CServer* server)
{
	if (!server || !theApp.emuledlg->IsRunning())
		return;

	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)server;
	int itemnr = FindItem(&find);
	if (itemnr == -1)
		return;
	//MORPH START - Added by SiRoB,  CountryFlag Addon
	Update(itemnr);
	return;
	//MORPH START - Added by SiRoB,  CountryFlag Addon
	const CServer* cur_srv;
	if (theApp.serverconnect->IsConnected()
		&& (cur_srv = theApp.serverconnect->GetCurrentServer()) != NULL
		&& cur_srv->GetPort() == server->GetPort()
		&& cur_srv->GetConnPort() == server->GetConnPort()//Morph - added by AndCycle, aux Ports, by lugdunummaster
		&& _tcsicmp(cur_srv->GetAddress(), server->GetAddress()) == 0)
		SetItemState(itemnr,LVIS_GLOW,LVIS_GLOW);
	else
		SetItemState(itemnr, 0, LVIS_GLOW);

	CString temp;
	//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
	/*
	temp.Format(_T("%s : %i"), server->GetAddress(), server->GetPort());
	*/
	if (server->GetConnPort() != server->GetPort())
			temp.Format(_T("%s : %i/%i"), server->GetAddress(), server->GetPort(), server->GetConnPort());
	else temp.Format(_T("%s : %i"), server->GetAddress(), server->GetPort());
	//Morph End - added by AndCycle, aux Ports, by lugdunummaster
	SetItemText(itemnr, 1, temp);

	//Servername
	SetItemText(itemnr,0,server->GetListName());
	
    //Description
	SetItemText(itemnr,2,server->GetDescription());

	// Ping
	if(server->GetPing()){
		temp.Format(_T("%i"), server->GetPing());
		SetItemText(itemnr, 3, temp);
	}
	else
		SetItemText(itemnr,3,_T(""));

	// Users
	if (server->GetUsers())
		SetItemText(itemnr, 4, CastItoIShort(server->GetUsers()));
	else
		SetItemText(itemnr,4,_T(""));

	// Max Users
	if (server->GetMaxUsers())
		SetItemText(itemnr, 5, CastItoIShort(server->GetMaxUsers()));
	else
		SetItemText(itemnr,5,_T(""));

	// Files
	if (server->GetFiles())
		SetItemText(itemnr, 6, CastItoIShort(server->GetFiles()));
	else
		SetItemText(itemnr,6,_T(""));

	switch(server->GetPreferences()){
	case SRV_PR_LOW:
		SetItemText(itemnr, 7, GetResString(IDS_PRIOLOW));
		break;
	case SRV_PR_NORMAL:
		SetItemText(itemnr, 7, GetResString(IDS_PRIONORMAL));
		break;
	case SRV_PR_HIGH:
		SetItemText(itemnr, 7, GetResString(IDS_PRIOHIGH));
		break;
	default:
		SetItemText(itemnr, 7, GetResString(IDS_PRIONOPREF));
	}
	
	// Failed Count
	temp.Format(_T("%i"), server->GetFailedCount());
	SetItemText(itemnr, 8, temp);

	// Static server
	if (server->IsStaticMember())
		SetItemText(itemnr,9,GetResString(IDS_YES)); 
	else
		SetItemText(itemnr,9,GetResString(IDS_NO));

	// Soft Files
	if (server->GetSoftFiles())
		SetItemText(itemnr, 10, CastItoIShort(server->GetSoftFiles()));
	else
		SetItemText(itemnr,10,_T(""));

	// Hard Files
	if (server->GetHardFiles())
		SetItemText(itemnr, 11, CastItoIShort(server->GetHardFiles()));
	else
		SetItemText(itemnr,11,_T(""));

	temp = server->GetVersion();
	if (thePrefs.GetDebugServerUDPLevel() > 0){
		if (server->GetUDPFlags() != 0){
			if (!temp.IsEmpty())
				temp += _T("; ");
			temp.AppendFormat(_T("ExtUDP=%x"), server->GetUDPFlags());
		}
	}
	if (thePrefs.GetDebugServerTCPLevel() > 0){
		if (server->GetTCPFlags() != 0){
			if (!temp.IsEmpty())
				temp += _T("; ");
			temp.AppendFormat(_T("ExtTCP=%x"), server->GetTCPFlags());
		}
	}
	SetItemText(itemnr,12,temp);

	// LowID Users
	if (server->GetLowIDUsers())
		SetItemText(itemnr, 13, CastItoIShort(server->GetLowIDUsers()));
	else
		SetItemText(itemnr, 13,_T(""));

	//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
	// aux Port
	if(server->GetConnPort() != server->GetPort()){
		temp.Format(_T("%i"), server->GetConnPort());
		SetItemText(itemnr, 14, temp);
	}
	else{
		SetItemText(itemnr,14,_T(""));
	}
	//Morph End - added by AndCycle, aux Ports, by lugdunummaster
   
	// Commander - Added: IP2Country column - Start
	//Countryname
	if(server->GetCountryName()){
	   CString tempCountryName;
	   tempCountryName = server->GetCountryName();
	   SetItemText(itemnr,15,tempCountryName);
	}
	else{
       SetItemText(itemnr,15,_T(""));
	}
	// Commander - Added: IP2Country column - End
}

//EastShare Start - added by AndCycle, IP to Country
void CServerListCtrl::RefreshAllServer(){

	for(POSITION pos = server_list->list.GetHeadPosition(); pos != NULL;){
		RefreshServer(server_list->list.GetAt(pos));
		server_list->list.GetNext(pos);
	}

}
//EastShare End - added by AndCycle, IP to Country

// CServerListCtrl message handlers

void CServerListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{ 
	// get merged settings
	bool bFirstItem = true;
	int iSelectedItems = GetSelectedCount();
	int iStaticServers = 0;
	UINT uPrioMenuItem = 0;
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		const CServer* pServer = (CServer*)GetItemData(GetNextSelectedItem(pos));

		iStaticServers += pServer->IsStaticMember() ? 1 : 0;

		UINT uCurPrioMenuItem = 0;
		if (pServer->GetPreferences() == SRV_PR_LOW)
			uCurPrioMenuItem = MP_PRIOLOW;
		else if (pServer->GetPreferences() == SRV_PR_NORMAL)
			uCurPrioMenuItem = MP_PRIONORMAL;
		else if (pServer->GetPreferences() == SRV_PR_HIGH)
			uCurPrioMenuItem = MP_PRIOHIGH;
		else
			ASSERT(0);

		if (bFirstItem)
			uPrioMenuItem = uCurPrioMenuItem;
		else if (uPrioMenuItem != uCurPrioMenuItem)
			uPrioMenuItem = 0;

		bFirstItem = false;
	}

	CTitleMenu ServerMenu;
	ServerMenu.CreatePopupMenu();
	ServerMenu.AddMenuTitle(GetResString(IDS_EM_SERVER));

	ServerMenu.AppendMenu(MF_STRING | (iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED), MP_CONNECTTO, GetResString(IDS_CONNECTTHIS));
	ServerMenu.SetDefaultItem(iSelectedItems > 0 ? MP_CONNECTTO : -1);

	CMenu ServerPrioMenu;
	ServerPrioMenu.CreateMenu();
	if (iSelectedItems > 0){
		ServerPrioMenu.AppendMenu(MF_STRING, MP_PRIOLOW, GetResString(IDS_PRIOLOW));
		ServerPrioMenu.AppendMenu(MF_STRING, MP_PRIONORMAL, GetResString(IDS_PRIONORMAL));
		ServerPrioMenu.AppendMenu(MF_STRING, MP_PRIOHIGH, GetResString(IDS_PRIOHIGH));
		ServerPrioMenu.CheckMenuRadioItem(MP_PRIOLOW, MP_PRIOHIGH, uPrioMenuItem, 0);
	}
	ServerMenu.AppendMenu(MF_POPUP  | (iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED), (UINT_PTR)ServerPrioMenu.m_hMenu, GetResString(IDS_PRIORITY));

	// enable add/remove from static server list, if there is at least one selected server which can be used for the action
	ServerMenu.AppendMenu(MF_STRING | (iStaticServers < iSelectedItems ? MF_ENABLED : MF_GRAYED), MP_ADDTOSTATIC, GetResString(IDS_ADDTOSTATIC));
	ServerMenu.AppendMenu(MF_STRING | (iStaticServers > 0 ? MF_ENABLED : MF_GRAYED), MP_REMOVEFROMSTATIC, GetResString(IDS_REMOVEFROMSTATIC));
	ServerMenu.AppendMenu(MF_SEPARATOR);

	ServerMenu.AppendMenu(MF_STRING | (iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED), MP_REMOVE, GetResString(IDS_REMOVETHIS));
	ServerMenu.AppendMenu(MF_STRING | (GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED), MP_REMOVEALL, GetResString(IDS_REMOVEALL));
	ServerMenu.AppendMenu(MF_STRING | (iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED), MP_GETED2KLINK, GetResString(IDS_DL_LINK1));
	ServerMenu.AppendMenu(MF_STRING | (theApp.IsEd2kServerLinkInClipboard() ? MF_ENABLED : MF_GRAYED), MP_PASTE, GetResString(IDS_PASTE));

	ServerMenu.AppendMenu(MF_SEPARATOR);
	ServerMenu.AppendMenu(MF_ENABLED | (GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED), MP_FIND, GetResString(IDS_FIND));

	GetPopupMenuPos(*this, point);
	ServerMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);

	VERIFY( ServerPrioMenu.DestroyMenu() );
	VERIFY( ServerMenu.DestroyMenu() );
}

BOOL CServerListCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
   if (wParam==MP_REMOVEALL)
   { 
		if (AfxMessageBox(_T("Do you really want to remove all servers?"), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) != IDYES)
			return TRUE;
	   if( theApp.serverconnect->IsConnecting() ){
	       theApp.downloadqueue->StopUDPRequests(); 
		   theApp.serverconnect->StopConnectionTry();
  		   theApp.serverconnect->Disconnect();
		   theApp.emuledlg->ShowConnectionState();
	   }
       ShowWindow(SW_HIDE); 
       server_list->RemoveAllServers(); 
       DeleteAllItems(); 
       ShowWindow(SW_SHOW);
		ShowServerCount();
		return TRUE;
   } 
	else if (wParam == MP_FIND)
	{
		OnFindStart();
		return TRUE;
	}
	else if (wParam == MP_PASTE)
	{
		if (theApp.IsEd2kServerLinkInClipboard())
			theApp.emuledlg->serverwnd->PasteServerFromClipboard();
		return TRUE;
	}

	int item = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (item != -1)
	{
		if (((CServer*)GetItemData(item)) != NULL)
		{
		  switch (wParam){ 
            case MP_CONNECTTO: 
			{
					if (GetSelectedCount() > 1)
					{
					CServer* aServer;

					theApp.serverconnect->Disconnect();
					POSITION pos=GetFirstSelectedItemPosition();
					while (pos!=NULL )
					{ 
							item = GetNextSelectedItem(pos);
						if (item>-1) {
								aServer=(CServer*)GetItemData(item);
							theApp.serverlist->MoveServerDown(aServer);
						}
					}
					theApp.serverconnect->ConnectToAnyServer( theApp.serverlist->GetServerCount()-  this->GetSelectedCount(),false, false );
					}
					else{
						theApp.serverconnect->ConnectToServer((CServer*)GetItemData(item));
				}
				theApp.emuledlg->ShowConnectionState();
					return TRUE;
			}			
			case MPG_DELETE:
            case MP_REMOVE: 
            { 
				ShowWindow(SW_HIDE); 
				POSITION pos;
					while (GetFirstSelectedItemPosition() != NULL)
				{ 
					pos=GetFirstSelectedItemPosition();
						item = GetNextSelectedItem(pos);
						server_list->RemoveServer((CServer*)GetItemData(item));
					DeleteItem(item);
				}
					ShowServerCount();
				ShowWindow(SW_SHOW); 
					SetFocus();
					return TRUE;
            }
			case MP_ADDTOSTATIC:
				{
				POSITION pos=GetFirstSelectedItemPosition();
				while( pos != NULL ){
						CServer* change = (CServer*)GetItemData(GetNextSelectedItem(pos));
					if (!StaticServerFileAppend(change))
							return FALSE;
					change->SetIsStaticMember(true);
					theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(change);
				}
					return TRUE;
			}
			case MP_REMOVEFROMSTATIC:
				{
				POSITION pos=GetFirstSelectedItemPosition();
				while( pos != NULL ){
						CServer* change = (CServer*)GetItemData(GetNextSelectedItem(pos));
					if (!StaticServerFileRemove(change))
							return FALSE;
					change->SetIsStaticMember(false);
					theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(change);
				}
					return TRUE;
			}
			case MP_PRIOLOW:
			{
					POSITION pos = GetFirstSelectedItemPosition();
				while( pos != NULL ){
						CServer* change = (CServer*)GetItemData(GetNextSelectedItem(pos));
					change->SetPreference( SRV_PR_LOW);
					theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(change);
				}
					return TRUE;
			}
			case MP_PRIONORMAL:
			{
					POSITION pos = GetFirstSelectedItemPosition();
				while( pos != NULL ){
						CServer* change = (CServer*)GetItemData(GetNextSelectedItem(pos));
					change->SetPreference( SRV_PR_NORMAL );
					theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(change);
				}
					return TRUE;
			}
			case MP_PRIOHIGH:
			{
					POSITION pos = GetFirstSelectedItemPosition();
				while( pos != NULL ){
						CServer* change = (CServer*)GetItemData(GetNextSelectedItem(pos));
					change->SetPreference( SRV_PR_HIGH );
					theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(change);
				}
					return TRUE;
			}
			case MP_COPYSELECTED:
			case MP_GETED2KLINK: 
			{ 
					POSITION pos = GetFirstSelectedItemPosition();
				CString buffer, link; 
				while( pos != NULL ){ 
						const CServer* change = (CServer*)GetItemData(GetNextSelectedItem(pos));
						buffer.Format(_T("ed2k://|server|%s|%d|/"), change->GetFullIP(), change->GetPort());
						if (link.GetLength() > 0)
							buffer = _T("\r\n") + buffer;
					link += buffer; 
				} 
				theApp.CopyTextToClipboard(link); 
					return TRUE;
			}
			case Irc_SetSendLink:
			{
					POSITION pos = GetFirstSelectedItemPosition();
				CString buffer, link; 
				while( pos != NULL ){ 
						const CServer* change = (CServer*)GetItemData(GetNextSelectedItem(pos));
						buffer.Format(_T("ed2k://|server|%s|%d|/"), change->GetFullIP(), change->GetPort());
						if (link.GetLength() > 0)
							buffer = _T("\r\n") + buffer;
						link += buffer;
					}
					theApp.emuledlg->ircwnd->SetSendFileString(link);
					return TRUE;
				}
			}
		}
  	}
	return FALSE;
}

void CServerListCtrl::OnNMLdblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1){
		theApp.serverconnect->ConnectToServer((CServer*)GetItemData(iSel));
	   theApp.emuledlg->ShowConnectionState();
	}
}

bool CServerListCtrl::AddServermetToList(const CString& strFile)
{ 
	SetRedraw(false);
  	bool flag=server_list->AddServermetToList(strFile);
	RemoveAllDeadServers();
	ShowServerCount();
	SetRedraw(true);
	return flag;
}

void CServerListCtrl::OnColumnClick(NMHDR *pNMHDR, LRESULT *pResult) 
{ 
   NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR; 

	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int iSortItem = thePrefs.GetColumnSortItem(CPreferences::tableServer);
	bool bOldSortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableServer);
	bool bSortAscending = (iSortItem != pNMListView->iSubItem) ? true : !bOldSortAscending;

	// Item is column clicked
	iSortItem = pNMListView->iSubItem;

	// Save new preferences
	thePrefs.SetColumnSortItem(CPreferences::tableServer, iSortItem);
	thePrefs.SetColumnSortAscending(CPreferences::tableServer, bSortAscending);

	// Sort table
	SetSortArrow(iSortItem, bSortAscending);
	SortItems(SortProc, MAKELONG(iSortItem, (bSortAscending ? 0 : 0x0001)));

	Invalidate();
   *pResult = 0; 
} 

int CServerListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const CServer* item1 = (CServer*)lParam1;
	const CServer* item2 = (CServer*)lParam2;
	if (item1 == NULL || item2 == NULL)
		return 0;

#define UNDEFINED_STR_AT_BOTTOM(s1, s2) \
	if ((s1).IsEmpty() && (s2).IsEmpty()) \
		return 0;						\
	if ((s1).IsEmpty())					\
		return 1;						\
	if ((s2).IsEmpty())					\
		return -1;						\

#define UNDEFINED_INT_AT_BOTTOM(i1, i2) \
	if ((i1) == (i2))					\
		return 0;						\
	if ((i1) == 0)						\
		return 1;						\
	if ((i2) == 0)						\
		return -1;						\

	int iResult;
	switch (LOWORD(lParamSort)){
	  case 0:
		  UNDEFINED_STR_AT_BOTTOM(item1->GetListName(), item2->GetListName());
		  iResult = item1->GetListName().CompareNoCase(item2->GetListName());
		  break;

	  case 1:
		  if (item1->HasDynIP() && item2->HasDynIP())
			  iResult = item1->GetDynIP().CompareNoCase(item2->GetDynIP());
		  else if (item1->HasDynIP())
			  iResult = -1;
		  else if (item2->HasDynIP())
			  iResult = 1;
		  else{
			  uint32 uIP1 = htonl(item1->GetIP());
			  uint32 uIP2 = htonl(item2->GetIP());
			  if (uIP1 < uIP2)
				  iResult = -1;
			  else if (uIP1 > uIP2)
				  iResult = 1;
			  else
				  iResult = CompareUnsigned(item1->GetPort(), item2->GetPort());
		  }
		  break;

	  case 2:
		  UNDEFINED_STR_AT_BOTTOM(item1->GetDescription(), item2->GetDescription());
		  iResult = item1->GetDescription().CompareNoCase(item2->GetDescription());
		  break;

	  case 3:
		  UNDEFINED_INT_AT_BOTTOM(item1->GetPing(), item2->GetPing());
		  iResult = CompareUnsigned(item1->GetPing(), item2->GetPing());
		  break;

	  case 4:
		  UNDEFINED_INT_AT_BOTTOM(item1->GetUsers(), item2->GetUsers());
		  iResult = CompareUnsigned(item1->GetUsers(), item2->GetUsers());
		  break;

	  case 5:
		  UNDEFINED_INT_AT_BOTTOM(item1->GetMaxUsers(), item2->GetMaxUsers());
		  iResult = CompareUnsigned(item1->GetMaxUsers(), item2->GetMaxUsers());
		  break;

	  case 6:
		  UNDEFINED_INT_AT_BOTTOM(item1->GetFiles(), item2->GetFiles());
		  iResult = CompareUnsigned(item1->GetFiles(), item2->GetFiles());
		  break;

	  case 7:
		  if (item2->GetPreferences() == item1->GetPreferences())
			  iResult = 0;
		  else if (item2->GetPreferences() == SRV_PR_LOW)
			  iResult = 1;
		  else if (item1->GetPreferences() == SRV_PR_LOW)
			  iResult = -1;
		  else if (item2->GetPreferences() == SRV_PR_HIGH)
			  iResult = -1;
		  else if (item1->GetPreferences() == SRV_PR_HIGH)
			  iResult = 1;
		  else
			  iResult = 0;
		  break;

	  case 8:
		  iResult = CompareUnsigned(item1->GetFailedCount(), item2->GetFailedCount());
		  break;

	  case 9:
		  iResult = (int)item1->IsStaticMember() - (int)item2->IsStaticMember();
		  break;

	  case 10:  
		  UNDEFINED_INT_AT_BOTTOM(item1->GetSoftFiles(), item2->GetSoftFiles());
		  iResult = CompareUnsigned(item1->GetSoftFiles(), item2->GetSoftFiles());
		  break;

	  case 11: 
		  UNDEFINED_INT_AT_BOTTOM(item1->GetHardFiles(), item2->GetHardFiles());
		  iResult = CompareUnsigned(item1->GetHardFiles(), item2->GetHardFiles());
		  break;

	  case 12:
		  UNDEFINED_STR_AT_BOTTOM(item1->GetVersion(), item2->GetVersion());
		  iResult = item1->GetVersion().CompareNoCase(item2->GetVersion());
		  break;

	  case 13:
		  UNDEFINED_INT_AT_BOTTOM(item1->GetLowIDUsers(), item2->GetLowIDUsers());
		  iResult = CompareUnsigned(item1->GetLowIDUsers(), item2->GetLowIDUsers());
		  break;
           // Commander - Added: AuxPort Sort - Start
	  case 14:
		  UNDEFINED_INT_AT_BOTTOM(item1->GetConnPort(), item2->GetConnPort());
		  iResult = CompareUnsigned(item1->GetConnPort(), item2->GetConnPort());
		  break;
		  // Commander - Added: AuxPort Sort - End
          // Commander - Added: IP2Country column - Start
      case 15:
          UNDEFINED_STR_AT_BOTTOM(item1->GetCountryName(), item2->GetCountryName());
          iResult = item1->GetCountryName().CompareNoCase(item2->GetCountryName());
		  break;
         // Commander - Added: IP2Country column - End

	  default: 
		  iResult = 0;
	} 
	if (HIWORD(lParamSort))
		iResult = -iResult;
	return iResult;
}

bool CServerListCtrl::StaticServerFileAppend(CServer *server)
{
	try
	{
		// Remove any entry before writing to avoid duplicates
		StaticServerFileRemove(server);

		FILE* staticservers = _tfsopen(thePrefs.GetConfigDir() + _T("staticservers.dat"), _T("a"), _SH_DENYWR);
		if (staticservers==NULL) 
		{
			AddLogLine( false, GetResString(IDS_ERROR_SSF));
			return false;
		}
		
		if (_ftprintf(staticservers,
					_T("%s:%i,%i,%s\n"),
					server->GetAddress(),
					server->GetPort(), 
					server->GetPreferences(),
					server->GetListName()) != EOF) 
		{
			AddLogLine(false, _T("'%s:%i,%s' %s"), server->GetAddress(), server->GetPort(), server->GetListName(), GetResString(IDS_ADDED2SSF));
			server->SetIsStaticMember(true);
			theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(server);
		}

		fclose(staticservers);
	}
	catch (...)
	{
		ASSERT(0);
		return false;
	}
	return true;
}

bool CServerListCtrl::StaticServerFileRemove(const CServer *server)
{
	try
	{
		if (!server->IsStaticMember())
			return true;

		CString strLine;
		CString strTest;
		TCHAR buffer[1024];
		int lenBuf = 1024;
		int pos;
		CString StaticFilePath = thePrefs.GetConfigDir() + _T("staticservers.dat");
		CString StaticTempPath = thePrefs.GetConfigDir() + _T("statictemp.dat");
		FILE* staticservers = _tfsopen(StaticFilePath , _T("r"), _SH_DENYWR);
		FILE* statictemp = _tfsopen(StaticTempPath , _T("w"), _SH_DENYWR);

		if ((staticservers == NULL) || (statictemp == NULL))
		{
			if (staticservers)
				fclose(staticservers);
			if (statictemp)
				fclose(statictemp);
			AddLogLine( false, GetResString(IDS_ERROR_SSF));
			return false;
		}

		while (!feof(staticservers))
		{
			if (_fgetts(buffer, lenBuf, staticservers) == 0)
				break;

			strLine = buffer;

			// ignore comments or invalid lines
			if (strLine.GetAt(0) == _T('#') || strLine.GetAt(0) == _T('/'))
				continue;
			if (strLine.GetLength() < 5)
				continue;

			// Only interested in "host:port"
			pos = strLine.Find(_T(','));
			if (pos == -1)
				continue;
			strLine = strLine.Left(pos);

			// Get host and port from given server
			strTest.Format(_T("%s:%i"), server->GetAddress(), server->GetPort());

			// Compare, if not the same server write original line to temp file
			if (strLine.Compare(strTest) != 0)
				_ftprintf(statictemp, buffer);
		}

		fclose(staticservers);
		fclose(statictemp);

		// All ok, remove the existing file and replace with the new one
		CFile::Remove( StaticFilePath );
		CFile::Rename( StaticTempPath, StaticFilePath );
	}
	catch (...)
	{
		ASSERT(0);
		return false;
	}
	return true;
}

void CServerListCtrl::ShowServerCount()
{
	CString counter;

	counter.Format(_T(" (%i)"), GetItemCount());
	theApp.emuledlg->serverwnd->GetDlgItem(IDC_SERVLIST_TEXT)->SetWindowText(GetResString(IDS_SV_SERVERLIST)+counter  );
}

void CServerListCtrl::OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	if (pGetInfoTip->iSubItem == 0)
	{
		LVHITTESTINFO hti = {0};
		::GetCursorPos(&hti.pt);
		ScreenToClient(&hti.pt);
		bool bOverMainItem = (SubItemHitTest(&hti) != -1 && hti.iItem == pGetInfoTip->iItem && hti.iSubItem == 0);

		// those tooltips are very nice for debugging/testing but pretty annoying for general usage
		// enable tooltips only if Shift+Ctrl is currently pressed
		bool bShowInfoTip = GetSelectedCount() > 1 || ((GetKeyState(VK_SHIFT) & 0x8000) && (GetKeyState(VK_CONTROL) & 0x8000));

		if (!bShowInfoTip){
			if (!bOverMainItem){
				// don' show the default label tip for the main item, if the mouse is not over the main item
				if ((pGetInfoTip->dwFlags & LVGIT_UNFOLDED) == 0 && pGetInfoTip->cchTextMax > 0 && pGetInfoTip->pszText[0] != '\0')
					pGetInfoTip->pszText[0] = '\0';
			}
			return;
		}

		if (GetSelectedCount() == 1)
		{
			;
		}
		else
		{
			int iSelected = 0;
			ULONGLONG ulTotalUsers = 0;
			ULONGLONG ulTotalLowIdUsers = 0;
			ULONGLONG ulTotalFiles = 0;
			POSITION pos = GetFirstSelectedItemPosition();
			while (pos)
			{
				const CServer* pServer = (CServer*)GetItemData(GetNextSelectedItem(pos));
				if (pServer)
				{
					iSelected++;
					ulTotalUsers += pServer->GetUsers();
					ulTotalFiles += pServer->GetFiles();
					ulTotalLowIdUsers += pServer->GetLowIDUsers();
				}
			}

			if (iSelected > 0)
			{
				CString strInfo;
				strInfo.Format(_T("%s: %u\r\n%s: %s\r\n%s: %s\r\n%s: %s"), 
					GetResString(IDS_FSTAT_SERVERS), iSelected, 
					GetResString(IDS_UUSERS), CastItoIShort(ulTotalUsers),
					GetResString(IDS_IDLOW), CastItoIShort(ulTotalLowIdUsers),
					GetResString(IDS_PW_FILES), CastItoIShort(ulTotalFiles));

				_tcsncpy(pGetInfoTip->pszText, strInfo, pGetInfoTip->cchTextMax);
				pGetInfoTip->pszText[pGetInfoTip->cchTextMax-1] = _T('\0');
			}
		}
	}

	*pResult = 0;
}

//Commander - Added: CountryFlag - Start
#define DLC_DT_TEXT (DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)

void CServerListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if( !theApp.emuledlg->IsRunning() )
		return;
	if (!lpDrawItemStruct->itemData)
		return;
	CDC* odc = CDC::FromHandle(lpDrawItemStruct->hDC);
	BOOL bCtrlFocused = ((GetFocus() == this ) || (GetStyle() & LVS_SHOWSELALWAYS));
	const CServer* server = (CServer*)lpDrawItemStruct->itemData;
	const CServer* cur_srv;
	if( (lpDrawItemStruct->itemAction | ODA_SELECT) && (lpDrawItemStruct->itemState & ODS_SELECTED )
		||
		(
			theApp.serverconnect->IsConnected()
			&& (cur_srv = theApp.serverconnect->GetCurrentServer()) != NULL
			&& cur_srv->GetPort() == server->GetPort()
			&& cur_srv->GetConnPort() == server->GetConnPort()//Morph - added by AndCycle, aux Ports, by lugdunummaster
			&& _tcsicmp(cur_srv->GetAddress(), server->GetAddress()) == 0)
		){
		if(bCtrlFocused)
			odc->SetBkColor(m_crHighlight);
		else
			odc->SetBkColor(m_crNoHighlight);
	}
	else
		odc->SetBkColor(GetBkColor());
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

	CString Sbuffer;
	CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
	int iCount = pHeaderCtrl->GetItemCount();
	cur_rec.right = cur_rec.left - 8;
	cur_rec.left += 4;

	for(int iCurrent = 0; iCurrent < iCount; iCurrent++){
		int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);
		if( !IsColumnHidden(iColumn) ){
			cur_rec.right += GetColumnWidth(iColumn);
			switch(iColumn){

				case 0:{
					uint8 image;
					image = 0;
					
					POINT point = {cur_rec.left, cur_rec.top+1};
					Sbuffer = server->GetListName();

					//CString tempStr;
					//tempStr.Format("%s%s", server->GetCountryName(), Sbuffer);
					//Sbuffer = tempStr;
                    
					//Draw Country Flag
					POINT point2= {cur_rec.left,cur_rec.top+1};
					if(theApp.ip2country->ShowCountryFlag()){
						theApp.ip2country->GetFlagImageList()->DrawIndirect(dc, server->GetCountryFlagIndex(), point2, CSize(18,16), CPoint(0,0), ILD_NORMAL);
					}
					else
						imagelist.DrawIndirect(dc, 0, point2, CSize(16,16), CPoint(0,0), ILD_NORMAL);

					cur_rec.left +=20;
					dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
					cur_rec.left -=20;

					break;
				}
				case 1:{
					if (server->GetConnPort() != server->GetPort())
			           Sbuffer.Format(_T("%s : %i/%i"), server->GetAddress(), server->GetPort(), server->GetConnPort());
	               else 
					Sbuffer.Format(_T("%s : %i"), server->GetAddress(), server->GetPort());
					break;
				}
				case 2:{
					Sbuffer = server->GetDescription();
					break;
			  }
				case 3:{
					if(server->GetPing())
						Sbuffer.Format(_T("%i"), server->GetPing());
					else
						Sbuffer = "";
					break;
				}
				case 4:{
					if(server->GetUsers())
						Sbuffer.Format(_T("%i"), server->GetUsers());
					else
						Sbuffer = "";
					break;
				}
				case 5:{
					if(server->GetMaxUsers())
						Sbuffer.Format(_T("%i"), server->GetMaxUsers());
					else
						Sbuffer = "";
					break;
				}
				case 6:{
					if(server->GetFiles())
						Sbuffer.Format(_T("%i"), server->GetFiles());
					else
						Sbuffer = "";
					break;
				}
				case 7:{
					switch(server->GetPreferences()){
						case SRV_PR_LOW:
							Sbuffer = GetResString(IDS_PRIOLOW);
							break;
						case SRV_PR_NORMAL:
							Sbuffer = GetResString(IDS_PRIONORMAL);
							break;
						case SRV_PR_HIGH:
							Sbuffer = GetResString(IDS_PRIOHIGH);
							break;
						default:
							Sbuffer = GetResString(IDS_PRIONOPREF);
						}
					break;
					}
				case 8:{
					Sbuffer.Format(_T("%i"), server->GetFailedCount());
					break;
				}
				case 9:{
					if (server->IsStaticMember())
						Sbuffer = GetResString(IDS_YES); 
					else
						Sbuffer = GetResString(IDS_NO);
					break;
				}
				case 10:{
					if(server->GetSoftFiles())
						Sbuffer.Format(_T("%i"), server->GetSoftFiles());
					else
						Sbuffer = "";
					break;
				}
				case 11:{
					if(server->GetHardFiles())
						Sbuffer.Format(_T("%i"), server->GetHardFiles());
					else
						Sbuffer = "";
					break;
				}
				case 12:{
					Sbuffer = server->GetVersion();
					if (thePrefs.GetDebugServerUDPLevel() > 0){
						if (server->GetUDPFlags() != 0){
							if (!Sbuffer.IsEmpty())
								Sbuffer += _T("; ");
							Sbuffer.AppendFormat(_T("ExtUDP=%x"), server->GetUDPFlags());
							}
						}
						if (thePrefs.GetDebugServerTCPLevel() > 0){
							if (server->GetTCPFlags() != 0){
								if (!Sbuffer.IsEmpty())
									Sbuffer += _T("; ");
								Sbuffer.AppendFormat(_T("ExtTCP=%x"), server->GetTCPFlags());
								}
							}
                        break;
						}
				//Commander - Lowid Column
				case 13:{
					if (server->GetLowIDUsers()){
						CString tempStr2;
						tempStr2.Format(_T("%s"), CastItoIShort(server->GetLowIDUsers()));
						Sbuffer = tempStr2;
					}
					else{
						Sbuffer = _T("");
					}
					break;
						}

				case 14:{
					if(server->GetConnPort() != server->GetPort())
						Sbuffer.Format(_T("%i"), server->GetConnPort());
					else
						Sbuffer = _T("");
					break;
						}

				//Commander - Country Column
				case 15:{
					if(server->GetCountryName()){
						CString tempStr3;
						tempStr3.Format(_T("%s"), server->GetCountryName());
						Sbuffer = tempStr3;
					}
					else{
                        Sbuffer = _T("");
					}
					break;
				}
}//End of Switch
				
					if(iColumn != 0 )
						dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec, DT_LEFT);
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
//Commander - Added: CountryFlag - End
