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


// ServerListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "ServerListCtrl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CServerListCtrl

IMPLEMENT_DYNAMIC(CServerListCtrl, CMuleListCtrl/*CTreeCtrl*/)
CServerListCtrl::CServerListCtrl() {
	server_list = 0; // i_a 
	SetGeneralPurposeFind(true);
}

bool CServerListCtrl::Init(CServerList* in_list)
{
	server_list = in_list;
	ModifyStyle(0,LVS_SINGLESEL|LVS_REPORT);
	ModifyStyle(LVS_SINGLESEL|LVS_LIST|LVS_ICON|LVS_SMALLICON,LVS_REPORT); //here the CListCtrl is set to report-style

	InsertColumn(0,GetResString(IDS_SL_SERVERNAME),LVCFMT_LEFT, 150);
	InsertColumn(1,GetResString(IDS_IP),LVCFMT_LEFT, 140);
	InsertColumn(2,GetResString(IDS_DESCRIPTION) ,LVCFMT_LEFT, 150);
	InsertColumn(3, GetResString(IDS_PING),			LVCFMT_RIGHT, 50);
	InsertColumn(4, GetResString(IDS_UUSERS),		LVCFMT_RIGHT, 50);
	InsertColumn(5, GetResString(IDS_MAXCLIENT),	LVCFMT_RIGHT, 50);
	InsertColumn(6, GetResString(IDS_PW_FILES) ,	LVCFMT_RIGHT, 50);
	InsertColumn(7,GetResString(IDS_PREFERENCE),LVCFMT_LEFT, 60);
	InsertColumn(8, GetResString(IDS_UFAILED),		LVCFMT_RIGHT, 50);
	InsertColumn(9,GetResString(IDS_STATICSERVER),LVCFMT_LEFT, 50);
	InsertColumn(10,GetResString(IDS_SOFTFILES),	LVCFMT_RIGHT, 50);
	InsertColumn(11,GetResString(IDS_HARDFILES),	LVCFMT_RIGHT, 50);
	InsertColumn(12,GetResString(IDS_VERSION),LVCFMT_LEFT, 50);

	Localize();
	LoadSettings(CPreferences::tableServer);

	// Barry - Use preferred sort order from preferences
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableServer);
	bool sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableServer);
	SetSortArrow(sortItem, sortAscending);
	// SLUGFILLER: multiSort - load multiple params
	for (int i = theApp.glob_prefs->GetColumnSortCount(CPreferences::tableServer); i > 0; ) {
		i--;
		sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableServer, i);
		sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableServer, i);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));
	}
	// SLUGFILLER: multiSort

	return true;
} 

CServerListCtrl::~CServerListCtrl() {
}

void CServerListCtrl::Localize()
{
	CImageList iml;
	iml.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	iml.SetBkColor(CLR_NONE);
	iml.Add(CTempIconLoader("Server"));
	HIMAGELIST himl = ApplyImageList(iml.Detach());
	if (himl)
		ImageList_Destroy(himl);

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
}

/*void CServerListCtrl::ShowServers(){ 
   DeleteAllItems(); 
   int i=0; 
   CString temp; 
   for(POSITION pos = server_list->list.GetHeadPosition(); pos != NULL;server_list->list.GetNext(pos)) { 
      CServer* cur_server = server_list->list.GetAt(pos); 
      InsertItem(LVIF_TEXT|LVIF_PARAM,i,cur_server->GetListName(),0,0,0,(LPARAM)cur_server);
	  RefreshServer( cur_server );
      i++; 
   } 
}
*/

void CServerListCtrl::RemoveServer(CServer* todel,bool bDelToList){
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)todel;
	sint32 result = FindItem(&find);
	if (result != (-1) ){
		server_list->RemoveServer((CServer*)GetItemData(result));
		DeleteItem(result); 
	}
	ShowFilesCount();
	return;
}

// Remove Dead Servers
void CServerListCtrl::RemoveDeadServer(){
	if( theApp.glob_prefs->DeadServer() ){
	   ShowWindow(SW_HIDE); 
		for(POSITION pos = server_list->list.GetHeadPosition(); pos != NULL;server_list->list.GetNext(pos)) { 
			CServer* cur_server = server_list->list.GetAt(pos); 
			if( cur_server->GetFailedCount() > theApp.glob_prefs->GetDeadserverRetries() ){	// MAX_SERVERFAILCOUNT 
				RemoveServer(cur_server);
				pos = server_list->list.GetHeadPosition();
			}
		}
	   ShowWindow(SW_SHOW); 
	}
}

bool CServerListCtrl::AddServer(CServer* toadd,bool bAddToList){ 
   if (!server_list->AddServer(toadd)) 
      return false; 
   if (bAddToList) 
   {
	   uint32 itemnr = GetItemCount();
	   InsertItem(LVIF_TEXT|LVIF_PARAM,itemnr,toadd->GetListName(),0,0,1,(LPARAM)toadd);
	   RefreshServer( toadd );
   }
   ShowFilesCount();
   return true; 
}


void CServerListCtrl::RefreshServer( CServer* server ){

	if (!theApp.emuledlg->IsRunning()) return;

	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)server;
	sint32 itemnr = FindItem(&find);
	if (itemnr == (-1))
		return;
	if( !server )
		return;

	if (theApp.serverconnect->IsConnected() && 
		theApp.serverconnect->GetCurrentServer()->GetPort() ==server->GetPort() &&
		strcmp(theApp.serverconnect->GetCurrentServer()->GetAddress(),server->GetAddress())==0
		)
		SetItemState(itemnr,LVIS_GLOW,LVIS_GLOW);
	else SetItemState(itemnr,0,LVIS_GLOW);

	CString temp;
	temp.Format( "%s : %i",server->GetAddress(),server->GetPort());

	SetItemText(itemnr,1,(LPCTSTR)temp);
	SetItemText(itemnr,0,server->GetListName()?(LPCTSTR)server->GetListName():_T(""));
	SetItemText(itemnr,2,server->GetDescription()?(LPCTSTR)server->GetDescription():_T(""));
	if(server->GetPing()){
		temp.Format( "%i",server->GetPing()); 
		SetItemText(itemnr,3,(LPCTSTR)temp);
	}
	else
		SetItemText(itemnr,3,_T(""));
	if(server->GetUsers()){
		temp.Format( "%i",server->GetUsers());
		SetItemText(itemnr,4,(LPCTSTR)temp);
	}
	else
		SetItemText(itemnr,4,_T(""));

	if(server->GetMaxUsers()){
		temp.Format( "%i",server->GetMaxUsers()); 
		SetItemText(itemnr,5,(LPCTSTR)temp);
	}
	else
		SetItemText(itemnr,5,_T(""));

	if(server->GetFiles()){
		temp.Format( "%i",server->GetFiles()); 
		SetItemText(itemnr,6,(LPCTSTR)temp);
	}
	else
		SetItemText(itemnr,6,_T(""));
	switch(server->GetPreferences()){
		case SRV_PR_LOW:
			temp.Format(GetResString(IDS_PRIOLOW));
			SetItemText(itemnr,7,(LPCTSTR)temp);
			break;
		case SRV_PR_NORMAL:
			temp.Format(GetResString(IDS_PRIONORMAL));
			SetItemText(itemnr,7,(LPCTSTR)temp);
			break;
		case SRV_PR_HIGH:
			temp.Format(GetResString(IDS_PRIOHIGH));
			SetItemText(itemnr,7,(LPCTSTR)temp);
			break;
		default:
			temp.Format( GetResString(IDS_PRIONOPREF));
			SetItemText(itemnr,7,(LPCTSTR)temp);
	}
	temp.Format( "%i",server->GetFailedCount()); 
	SetItemText(itemnr,8,(LPCTSTR)temp);
	
	if (server->IsStaticMember())
		SetItemText(itemnr,9,GetResString(IDS_YES)); 
	else
		SetItemText(itemnr,9,GetResString(IDS_NO));

	if(server->GetSoftFiles()){
		temp.Format( "%i",server->GetSoftFiles()); 
		SetItemText(itemnr,10,(LPCTSTR)temp);
	}
	else
		SetItemText(itemnr,10,_T(""));

	if(server->GetHardFiles()){
		temp.Format( "%i",server->GetHardFiles()); 
		SetItemText(itemnr,11,(LPCTSTR)temp);
	}
	else
		SetItemText(itemnr,11,_T(""));

	temp = server->GetVersion();
	if (theApp.glob_prefs->GetDebugServerUDP()){
		if (server->GetUDPFlags() != 0){
			if (!temp.IsEmpty())
				temp += _T("; ");
			temp.AppendFormat(_T("ExtUDP=%x"), server->GetUDPFlags());
		}
	}
	if (theApp.glob_prefs->GetDebugServerTCP()){
		if (server->GetTCPFlags() != 0){
			if (!temp.IsEmpty())
				temp += _T("; ");
			temp.AppendFormat(_T("ExtTCP=%x"), server->GetTCPFlags());
		}
	}
	SetItemText(itemnr,12,temp);
}

BEGIN_MESSAGE_MAP(CServerListCtrl, CMuleListCtrl/*CTreeCtrl*/) 
   ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick) 
   ON_NOTIFY_REFLECT (NM_DBLCLK, OnNMLdblclk) //<-- mod bb 27.09.02 
   ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

// CServerListCtrl message handlers

void CServerListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{ 
//   POINT point; 
//   ::GetCursorPos(&point); 

   // tecxx 0609 2002 
   // fix - on right click, we also want to change the current selection like the left click does 
   CPoint p = point; 
   ScreenToClient(&p); 
   int it = HitTest(p); 
   if (it != -1) 
      SetSelectionMark(it);   // display selection mark correctly! 
   // fix end 

   // Create up-to-date popupmenu
   UINT flags,flagSSL1,flagSSL2;
   CTitleMenu m_ServerMenu;
   CMenu m_ServerPrioMenu;

   CServer* test=NULL;
   if (this->GetSelectionMark() != -1) test=(CServer*)GetItemData(GetSelectionMark());

   // set state of selection-dependent menuitems
   flags=MF_STRING || MF_DISABLED;
   if (this->GetSelectionMark() != -1) if (test != NULL) flags=MF_STRING;
   flagSSL1=MF_STRING || MF_DISABLED;
   flagSSL2=MF_STRING || MF_DISABLED;

   if (test != NULL) 
	   if (test->IsStaticMember()) flagSSL2=MF_STRING; else flagSSL1=MF_STRING;

   // add priority switcher
   m_ServerPrioMenu.CreateMenu();
   m_ServerPrioMenu.AppendMenu(MF_STRING,MP_PRIOLOW,GetResString(IDS_PRIOLOW));
   m_ServerPrioMenu.AppendMenu(MF_STRING,MP_PRIONORMAL,GetResString(IDS_PRIONORMAL));
   m_ServerPrioMenu.AppendMenu(MF_STRING,MP_PRIOHIGH,GetResString(IDS_PRIOHIGH));

   m_ServerMenu.CreatePopupMenu(); 
   m_ServerMenu.AddMenuTitle(GetResString(IDS_EM_SERVER));
   m_ServerMenu.AppendMenu(flags,MP_CONNECTTO, GetResString(IDS_CONNECTTHIS)); 
   m_ServerMenu.AppendMenu(flags|MF_POPUP,(UINT_PTR)m_ServerPrioMenu.m_hMenu, GetResString(IDS_PRIORITY));

   m_ServerMenu.AppendMenu( flagSSL1,MP_ADDTOSTATIC, GetResString(IDS_ADDTOSTATIC));
   m_ServerMenu.AppendMenu( flagSSL2,MP_REMOVEFROMSTATIC, GetResString(IDS_REMOVEFROMSTATIC));

   m_ServerMenu.AppendMenu(MF_STRING|MF_SEPARATOR);	
   m_ServerMenu.AppendMenu(flags,MP_REMOVE,   GetResString(IDS_REMOVETHIS)); 
   m_ServerMenu.AppendMenu(MF_STRING,MP_REMOVEALL, GetResString(IDS_REMOVEALL));
   m_ServerMenu.AppendMenu(MF_SEPARATOR); 
   m_ServerMenu.AppendMenu(MF_STRING,MP_GETED2KLINK, GetResString(IDS_DL_LINK1) );
//   m_ServerMenu.AppendMenu(MF_STRING,Irc_SetSendLink,GetResString(IDS_IRC_ADDLINKTOIRC)); 

   m_ServerMenu.SetDefaultItem(MP_CONNECTTO);
   m_ServerMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this); 

   VERIFY( m_ServerPrioMenu.DestroyMenu() );
   VERIFY( m_ServerMenu.DestroyMenu() );
}

BOOL CServerListCtrl::OnCommand(WPARAM wParam,LPARAM lParam ){ 
   int item= this->GetSelectionMark(); 

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
	   ShowFilesCount();
       return true;
   } 

   if (item != -1){ 
      if (((CServer*)GetItemData(GetSelectionMark())) != NULL){ 

		  switch (wParam){ 
            case MP_CONNECTTO: 
			{
				if ( this->GetSelectedCount()>1 ) {
					CServer* aServer;

					theApp.serverconnect->Disconnect();
					POSITION pos=GetFirstSelectedItemPosition();
					while (pos!=NULL )
					{ 
						item = this->GetNextSelectedItem(pos); 
						if (item>-1) {
							aServer=(CServer*)this->GetItemData(item);
							theApp.serverlist->MoveServerDown(aServer);
						}
					}
					theApp.serverconnect->ConnectToAnyServer( theApp.serverlist->GetServerCount()-  this->GetSelectedCount(),false, false );
				} else {
					theApp.serverconnect->ConnectToServer((CServer*)GetItemData(GetSelectionMark()));
				}
				theApp.emuledlg->ShowConnectionState();
			    break; 
			}			
			case MPG_DELETE:
            case MP_REMOVE: 
            { 
				ShowWindow(SW_HIDE); 
				POSITION pos;
				while (GetFirstSelectedItemPosition()!=NULL) //(pos != NULL) 
				{ 
					pos=GetFirstSelectedItemPosition();
					item = this->GetNextSelectedItem(pos); 
					server_list->RemoveServer( (CServer*)this->GetItemData(item));
					DeleteItem(item);
				}
				ShowFilesCount();
				ShowWindow(SW_SHOW); 
				break; 
            }
			case MP_ADDTOSTATIC:{
				POSITION pos=GetFirstSelectedItemPosition();
				while( pos != NULL ){
					CServer* change = (CServer*)this->GetItemData(this->GetNextSelectedItem(pos));
					if (!StaticServerFileAppend(change))
						return false;
					change->SetIsStaticMember(true);
					theApp.emuledlg->serverwnd.serverlistctrl.RefreshServer(change);
				}
				break;
			}
			// Remove Static Servers [Barry]
			case MP_REMOVEFROMSTATIC:
				{
				POSITION pos=GetFirstSelectedItemPosition();
				while( pos != NULL ){
					CServer* change = (CServer*)this->GetItemData(this->GetNextSelectedItem(pos));
					if (!StaticServerFileRemove(change))
						return false;
					change->SetIsStaticMember(false);
					theApp.emuledlg->serverwnd.serverlistctrl.RefreshServer(change);
				}
				break;
			}
			case MP_PRIOLOW:
			{
				POSITION pos = this->GetFirstSelectedItemPosition();
				while( pos != NULL ){
					CServer* change = (CServer*)this->GetItemData(this->GetNextSelectedItem(pos));
					change->SetPreference( SRV_PR_LOW);
//					if (change->IsStaticMember())
//						StaticServerFileAppend(change); //Why are you adding to static when changing prioity? If I want it static I set it static.. I set server to LOW because I HATE this server, not because I like it!!!
					theApp.emuledlg->serverwnd.serverlistctrl.RefreshServer(change);
				}
				break;
			}
			case MP_PRIONORMAL:
			{
				POSITION pos = this->GetFirstSelectedItemPosition();
				while( pos != NULL ){
					CServer* change = (CServer*)this->GetItemData(this->GetNextSelectedItem(pos));
					change->SetPreference( SRV_PR_NORMAL );
//					if (change->IsStaticMember())
//						StaticServerFileAppend(change);
					theApp.emuledlg->serverwnd.serverlistctrl.RefreshServer(change);
				}
				break;
			}
			case MP_PRIOHIGH:
			{
				POSITION pos = this->GetFirstSelectedItemPosition();
				while( pos != NULL ){
					CServer* change = (CServer*)this->GetItemData(this->GetNextSelectedItem(pos));
					change->SetPreference( SRV_PR_HIGH );
//					if (change->IsStaticMember())
//						StaticServerFileAppend(change);
					theApp.emuledlg->serverwnd.serverlistctrl.RefreshServer(change);
				}
				break;
			}
			case MP_GETED2KLINK: 
			{ 
				POSITION pos = this->GetFirstSelectedItemPosition(); 
				CString buffer, link; 
				while( pos != NULL ){ 
					CServer* change = (CServer*)this->GetItemData(this->GetNextSelectedItem(pos)); 
					buffer.Format("ed2k://|server|%s|%d|/", change->GetFullIP(), change->GetPort()); 
					if (link.GetLength()>0) buffer="\n"+buffer;
					link += buffer; 
				} 
				theApp.CopyTextToClipboard(link); 
				break; 
			}
			case Irc_SetSendLink:
			{
				POSITION pos = this->GetFirstSelectedItemPosition(); 
				CString buffer, link; 
				while( pos != NULL ){ 
					CServer* change = (CServer*)this->GetItemData(this->GetNextSelectedItem(pos)); 
					buffer.Format("ed2k://|server|%s|%d|/", change->GetFullIP(), change->GetPort()); 
					if (link.GetLength()>0) buffer="\n"+buffer;
					link += buffer; 
				} 
				theApp.emuledlg->ircwnd.SetSendFileString(link);
				break;
			}

         } 
      } 
   } 
   return true; 
}
void CServerListCtrl::OnNMLdblclk(NMHDR *pNMHDR, LRESULT *pResult){ // mod bb 27.09.02
	if (GetSelectionMark() != (-1)) {
		theApp.serverconnect->ConnectToServer((CServer*)GetItemData(GetSelectionMark())); 
	   theApp.emuledlg->ShowConnectionState();
	}
}

bool CServerListCtrl::AddServermetToList(CString strFile) 
{ 
   SetRedraw(false);
   bool flag=server_list->AddServermetToList(strFile);
   RemoveDeadServer();
   ShowFilesCount();
   SetRedraw(true);
   return flag;
}

void CServerListCtrl::OnColumnClick(NMHDR *pNMHDR, LRESULT *pResult) 
{ 
   NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR; 

	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableServer);
	bool m_oldSortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableServer);
	bool sortAscending = (sortItem != pNMListView->iSubItem) ? true : !m_oldSortAscending;

	// Item is column clicked
	sortItem = pNMListView->iSubItem;

	// Save new preferences
	theApp.glob_prefs->SetColumnSortItem(CPreferences::tableServer, sortItem);
	theApp.glob_prefs->SetColumnSortAscending(CPreferences::tableServer, sortAscending);

	// Sort table
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));

   this->Invalidate(); 
   *pResult = 0; 
} 


int CServerListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort){ 
   CServer* item1 = (CServer*)lParam1; 
   CServer* item2 = (CServer*)lParam2; 
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

		FILE* staticservers = fopen(theApp.glob_prefs->GetConfigDir() + CString("staticservers.dat"), "a");
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
			theApp.emuledlg->serverwnd.serverlistctrl.RefreshServer(server);
		}

		fclose(staticservers);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool CServerListCtrl::StaticServerFileRemove(CServer *server)
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
		CString StaticFilePath = theApp.glob_prefs->GetConfigDir() + CString("staticservers.dat");
		CString StaticTempPath = theApp.glob_prefs->GetConfigDir() + CString("statictemp.dat");
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
		return false;
	}
	return true;
}

void CServerListCtrl::ShowFilesCount() {
	CString counter;

	counter.Format(" (%i)", GetItemCount());
	theApp.emuledlg->serverwnd.GetDlgItem(IDC_SERVLIST_TEXT)->SetWindowText(GetResString(IDS_SV_SERVERLIST)+counter  );
}
