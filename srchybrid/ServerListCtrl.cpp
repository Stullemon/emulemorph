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
#include "IP2Country.h" //EastShare - added by AndCycle, IP to Country
#include "MemDC.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CServerListCtrl

IMPLEMENT_DYNAMIC(CServerListCtrl, CMuleListCtrl)
CServerListCtrl::CServerListCtrl() {
	server_list = 0; // i_a 
	SetGeneralPurposeFind(true);
}

bool CServerListCtrl::Init(CServerList* in_list)
{
	server_list = in_list;
	ModifyStyle(0,LVS_SINGLESEL|LVS_REPORT);
	ModifyStyle(LVS_SINGLESEL|LVS_LIST|LVS_ICON|LVS_SMALLICON,LVS_REPORT); //here the CListCtrl is set to report-style

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
	InsertColumn(13,GetResString(IDS_AUXPORTS),		LVCFMT_LEFT,  50);//Morph - added by AndCycle, aux Ports, by lugdunummaster

	SetAllIcons();
	Localize();
	LoadSettings(CPreferences::tableServer);

	// Barry - Use preferred sort order from preferences
	int sortItem = thePrefs.GetColumnSortItem(CPreferences::tableServer);
	bool sortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableServer);
	SetSortArrow(sortItem, sortAscending);
	// SLUGFILLER: multiSort - load multiple params
	for (int i = thePrefs.GetColumnSortCount(CPreferences::tableServer); i > 0; ) {
		i--;
		sortItem = thePrefs.GetColumnSortItem(CPreferences::tableServer, i);
		sortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableServer, i);
		SortItems(SortProc, sortItem + (sortAscending ? 0:100));
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
	iml.Add(CTempIconLoader("Server"));
	HIMAGELIST himl = ApplyImageList(iml.Detach());
	if (himl)
		ImageList_Destroy(himl);
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

	//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
	strRes = GetResString(IDS_AUXPORTS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(13, &hdi);
	strRes.ReleaseBuffer();
	//Morph End - added by AndCycle, aux Ports, by lugdunummaster
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
	}
	ShowServerCount();
	return;
}

// Remove Dead Servers
void CServerListCtrl::RemoveDeadServer()
{
	if( thePrefs.DeadServer() ){
	   ShowWindow(SW_HIDE); 
		for(POSITION pos = server_list->list.GetHeadPosition(); pos != NULL;server_list->list.GetNext(pos)) { 
			CServer* cur_server = server_list->list.GetAt(pos); 
			if( cur_server->GetFailedCount() > thePrefs.GetDeadserverRetries() ){	// MAX_SERVERFAILCOUNT 
				RemoveServer(cur_server);
				pos = server_list->list.GetHeadPosition();
			}
		}
	   ShowWindow(SW_SHOW); 
	}
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

	const CServer* cur_srv;
	if (theApp.serverconnect->IsConnected()
		&& (cur_srv = theApp.serverconnect->GetCurrentServer()) != NULL
		&& cur_srv->GetPort() == server->GetPort()
		&& cur_srv->GetConnPort() == server->GetConnPort()//Morph - added by AndCycle, aux Ports, by lugdunummaster
		&& stricmp(cur_srv->GetAddress(), server->GetAddress()) == 0)
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

	//EastShare Start - added by AndCycle, IP to Country
	CString tempServerName;
	tempServerName = server->GetCountryName();
	tempServerName.Append(server->GetListName()?(LPCTSTR)server->GetListName():_T(""));
	SetItemText(itemnr,0,tempServerName);

	/*
	//original
	SetItemText(itemnr,0,server->GetListName()?(LPCTSTR)server->GetListName():_T(""));
	*/
	//EastShare End - added by AndCycle, IP to Country

	SetItemText(itemnr,2,server->GetDescription()?(LPCTSTR)server->GetDescription():_T(""));

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
	//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
	// aux Port
	if(server->GetConnPort() != server->GetPort()){
		temp.Format(_T("%i"), server->GetConnPort());
		SetItemText(itemnr, 13, temp);
	}
	else{
		SetItemText(itemnr,13,_T(""));
	}
	//Morph End - added by AndCycle, aux Ports, by lugdunummaster
}

//EastShare Start - added by AndCycle, IP to Country
void CServerListCtrl::RefreshAllServer(){

	for(POSITION pos = server_list->list.GetHeadPosition(); pos != NULL;){
		RefreshServer(server_list->list.GetAt(pos));
		server_list->list.GetNext(pos);
	}

}
//EastShare End - added by AndCycle, IP to Country

BEGIN_MESSAGE_MAP(CServerListCtrl, CMuleListCtrl) 
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick) 
	ON_NOTIFY_REFLECT (NM_DBLCLK, OnNMLdblclk) //<-- mod bb 27.09.02 
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

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
       return true;
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
   RemoveDeadServer();
	ShowServerCount();
   SetRedraw(true);
   return flag;
}

void CServerListCtrl::OnColumnClick(NMHDR *pNMHDR, LRESULT *pResult) 
{ 
   NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR; 

	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int sortItem = thePrefs.GetColumnSortItem(CPreferences::tableServer);
	bool m_oldSortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableServer);
	bool sortAscending = (sortItem != pNMListView->iSubItem) ? true : !m_oldSortAscending;

	// Item is column clicked
	sortItem = pNMListView->iSubItem;

	// Save new preferences
	thePrefs.SetColumnSortItem(CPreferences::tableServer, sortItem);
	thePrefs.SetColumnSortAscending(CPreferences::tableServer, sortAscending);

	// Sort table
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));

   this->Invalidate(); 
   *pResult = 0; 
} 

int CServerListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const CServer* item1 = (CServer*)lParam1;
	const CServer* item2 = (CServer*)lParam2;
   if((item1 == NULL) || (item2 == NULL))
	   return 0; 
   int iTemp=0; 
    
   switch(lParamSort){ 
      case 0: //(List) Server-name asc
		  if (item1->GetListName() && item2->GetListName())
			  return _tcsicmp(item1->GetListName(), item2->GetListName());
		  return CString(item1->GetListName()).CompareNoCase(item2->GetListName());
      case 100: //(List) Server-name desc 
		  if (item2->GetListName() && item1->GetListName())
			  return _tcsicmp(item2->GetListName(), item1->GetListName());
		  return CString(item2->GetListName()).CompareNoCase(item1->GetListName());
      case 1:{ //IP asc
		  if( item1->HasDynIP() && item2->HasDynIP() )
			  return CString(item1->GetDynIP()).CompareNoCase(item2->GetDynIP());
		  else if( item1->HasDynIP() == true && item2->HasDynIP() == true )
			  return 0;
		  else if( item1->HasDynIP() )
			  return 1;
		  else if( item2->HasDynIP() )
			  return 0;
		  else{
			  uint32 uIP1 = item1->GetIP();
			  uint32 uIP2 = item2->GetIP();
			  const uchar* pIP1 = (uchar*)&uIP1;
			  const uchar* pIP2 = (uchar*)&uIP2;
			  iTemp = 0;
			  for (int i = 0; i < 4 && iTemp == 0; i++)
				  iTemp = *pIP1++ - *pIP2++;
			  if (iTemp == 0)
				  iTemp = item1->GetPort() - item2->GetPort();
			  return iTemp; 
		  }
			 }
      case 101:{ //IP desc 
		  if( item1->HasDynIP() && item2->HasDynIP() )
			  return CString(item2->GetDynIP()).CompareNoCase(item1->GetDynIP());
		  else if( item1->HasDynIP() == true && item2->HasDynIP() == true )
			  return 0;
		  else if( item1->HasDynIP() )
			  return 0;
		  else if( item2->HasDynIP() )
			  return 1;
		  else{
			  uint32 uIP1 = item1->GetIP();
			  uint32 uIP2 = item2->GetIP();
			  const uchar* pIP1 = (uchar*)&uIP1;
			  const uchar* pIP2 = (uchar*)&uIP2;
			  iTemp = 0;
			  for (int i = 0; i < 4 && iTemp == 0; i++)
				  iTemp = *pIP1++ - *pIP2++;
			  if (iTemp == 0)
				  iTemp = item1->GetPort() - item2->GetPort();
			  return -iTemp;
		  }
			  }
      case 2: //Description asc 
         if((item1->GetDescription() != NULL) && (item2->GetDescription() != NULL)) 
            //the 'if' is necessary, because the Description-String is not 
            //always initialisized in server.cpp 
			return _tcsicmp(item2->GetDescription(), item1->GetDescription());
		 else if( item1->GetDescription() == item2->GetDescription() )
			 return 0;
		 else if( item1->GetDescription() == NULL )
			 return 1;
		 else
			 return 0;
      case 102: //Desciption desc 
         if((item1->GetDescription() != NULL) && (item2->GetDescription() != NULL))
			return _tcsicmp(item1->GetDescription(), item2->GetDescription());
		 else if( item1->GetDescription() == item2->GetDescription() )
			 return 0;
		 else if( item1->GetDescription() == NULL )
			 return 1;
		 else
			 return 0;
      case 3: //Ping asc
		  if(item1->GetPing() == item2->GetPing())
			  return 0;
		  if(!item1->GetPing())
			  return 1;
		  if(!item2->GetPing())
			  return -1;
         return item1->GetPing() - item2->GetPing(); 
      case 103: //Ping desc 
		  if(item1->GetPing() == item2->GetPing())
			  return 0;
		  if(!item1->GetPing())
			  return 1;
		  if(!item2->GetPing())
			  return -1;
         return item2->GetPing() - item1->GetPing(); 
      case 4: //Users asc 
		  if(item1->GetUsers() == item2->GetUsers())
			  return 0;
		  if(!item1->GetUsers())
			  return 1;
		  if(!item2->GetUsers())
			  return -1;
         return item1->GetUsers() - item2->GetUsers(); 
      case 104: //Users desc 
		  if(item1->GetUsers() == item2->GetUsers())
			  return 0;
		  if(!item1->GetUsers())
			  return 1;
		  if(!item2->GetUsers())
			  return -1;
         return item2->GetUsers() - item1->GetUsers(); 
      case 5: //Users asc 
		  if(item1->GetMaxUsers() == item2->GetMaxUsers())
			  return 0;
		  if(!item1->GetMaxUsers())
			  return 1;
		  if(!item2->GetMaxUsers())
			  return -1;
         return item1->GetMaxUsers() - item2->GetMaxUsers(); 
      case 105: //Users desc 
		  if(item1->GetMaxUsers() == item2->GetMaxUsers())
			  return 0;
		  if(!item1->GetMaxUsers())
			  return 1;
		  if(!item2->GetMaxUsers())
			  return -1;
         return item2->GetMaxUsers() - item1->GetMaxUsers(); 
      case 6: //Files asc 
		  if(item1->GetFiles() == item2->GetFiles())
			  return 0;
		  if(!item1->GetFiles())
			  return 1;
		  if(!item2->GetFiles())
			  return -1;
         return item1->GetFiles() - item2->GetFiles(); 
      case 106: //Files desc 
		  if(item1->GetFiles() == item2->GetFiles())
			  return 0;
		  if(!item1->GetFiles())
			  return 1;
		  if(!item2->GetFiles())
			  return -1;
         return item2->GetFiles() - item1->GetFiles(); 
      case 7: //Preferences asc 
		  if( item2->GetPreferences() == item1->GetPreferences() )
			  return 0;
		  if( item2->GetPreferences() == SRV_PR_LOW )
			  return 1;
		  else if ( item1->GetPreferences() == SRV_PR_LOW )
			  return -1;
		  if( item2->GetPreferences() == SRV_PR_HIGH )
			  return -1;
		  else if ( item1->GetPreferences() == SRV_PR_HIGH )
			  return 1;
      case 107: //Preferences desc 
		  if( item2->GetPreferences() == item1->GetPreferences() )
			  return 0;
		  if( item2->GetPreferences() == SRV_PR_LOW )
			  return -1;
		  else if ( item1->GetPreferences() == SRV_PR_LOW )
			  return 1;
		  if( item2->GetPreferences() == SRV_PR_HIGH )
			  return 1;
		  else if ( item1->GetPreferences() == SRV_PR_HIGH )
			  return -1;
      case 8: //failed asc 
         return item1->GetFailedCount() - item2->GetFailedCount(); 
      case 108: //failed desc 
		  return item2->GetFailedCount() - item1->GetFailedCount(); 
      case 9: //staticservers 
		  return item2->IsStaticMember() - item1->IsStaticMember(); 
      case 109: //staticservers-
		  return item1->IsStaticMember() - item2->IsStaticMember(); 
      case 10:  
		  if(item1->GetSoftFiles() == item2->GetSoftFiles())
			  return 0;
		  if(!item1->GetSoftFiles())
			  return 1;
		  if(!item2->GetSoftFiles())
			  return -1;
		  return item1->GetSoftFiles() - item2->GetSoftFiles(); 
      case 110: 
		  if(item1->GetSoftFiles() == item2->GetSoftFiles())
			  return 0;
		  if(!item1->GetSoftFiles())
			  return 1;
		  if(!item2->GetSoftFiles())
			  return -1;
		  return item2->GetSoftFiles() - item1->GetSoftFiles(); 
      case 11: 
		  if(item1->GetHardFiles() == item2->GetHardFiles())
			  return 0;
		  if(!item1->GetHardFiles())
			  return 1;
		  if(!item2->GetHardFiles())
			  return -1;
		  return item1->GetHardFiles() - item2->GetHardFiles(); 
      case 111: 
		  if(item1->GetHardFiles() == item2->GetHardFiles())
			  return 0;
		  if(!item1->GetHardFiles())
			  return 1;
		  if(!item2->GetHardFiles())
			  return -1;
		  return item2->GetHardFiles() - item1->GetHardFiles(); 

	  case 12:
		  return item1->GetVersion().CompareNoCase(item2->GetVersion());
	  case 112:
		  return item2->GetVersion().CompareNoCase(item1->GetVersion());
      default: 
         return 0; 
   } 
}

bool CServerListCtrl::StaticServerFileAppend(CServer *server)
{
	try
	{
		// Remove any entry before writing to avoid duplicates
		StaticServerFileRemove(server);

		FILE* staticservers = fopen(thePrefs.GetConfigDir() + CString("staticservers.dat"), "a");
		if (staticservers==NULL) 
		{
			AddLogLine( false, GetResString(IDS_ERROR_SSF));
			return false;
		}
		
		if (fprintf(staticservers,
					"%s:%i,%i,%s\n",
					server->GetAddress(),
					server->GetPort(), 
					server->GetPreferences(),
					server->GetListName()) != EOF) 
		{
			AddLogLine(false, "'%s:%i,%s' %s", server->GetAddress(), server->GetPort(), server->GetListName(), GetResString(IDS_ADDED2SSF));
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
		char buffer[1024];
		int lenBuf = 1024;
		int pos;
		CString StaticFilePath = thePrefs.GetConfigDir() + CString("staticservers.dat");
		CString StaticTempPath = thePrefs.GetConfigDir() + CString("statictemp.dat");
		FILE* staticservers = fopen(StaticFilePath , "r");
		FILE* statictemp = fopen(StaticTempPath , "w");

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
			if (fgets(buffer, lenBuf, staticservers) == 0)
				break;

			strLine = buffer;

			// ignore comments or invalid lines
			if (strLine.GetAt(0) == '#' || strLine.GetAt(0) == '/')
				continue;
			if (strLine.GetLength() < 5)
				continue;

			// Only interested in "host:port"
			pos = strLine.Find(',');
			if (pos == -1)
				continue;
			strLine = strLine.Left(pos);

			// Get host and port from given server
			strTest.Format("%s:%i", server->GetAddress(), server->GetPort());

			// Compare, if not the same server write original line to temp file
			if (strLine.Compare(strTest) != 0)
				fprintf(statictemp, buffer);
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

void CServerListCtrl::ShowServerCount() {
	CString counter;

	counter.Format(" (%i)", GetItemCount());
	theApp.emuledlg->serverwnd->GetDlgItem(IDC_SERVLIST_TEXT)->SetWindowText(GetResString(IDS_SV_SERVERLIST)+counter  );
}

void CServerListCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		// Ctrl+C: Copy listview items to clipboard
		SendMessage(WM_COMMAND, MP_GETED2KLINK);
		return;
	}
	else if (nChar == 'V' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		// Ctrl+C: Copy listview items to clipboard
		SendMessage(WM_COMMAND, MP_PASTE);
		return;
	}
	CMuleListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}
