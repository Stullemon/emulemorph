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

// IrcWnd.cpp : implementation file
//

#include "stdafx.h"
#include "IrcWnd.h"
#include "emule.h"
#include "otherfunctions.h"
#include "opcodes.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// CIrcWnd dialog
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(CIrcWnd, CDialog)
CIrcWnd::CIrcWnd(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CIrcWnd::IDD, pParent)
{
   m_pIrcMain = 0; // i_a: bugfix: crash prevention 
   m_bConnected = false;          // i_a 
   m_bLoggedIn = false;          // i_a 
   m_pCurrentChannel = 0;         // i_a 
   memset(&asc_sort, 0, sizeof(asc_sort)); // i_a 
}

CIrcWnd::~CIrcWnd()
{
	if( m_bConnected )
		m_pIrcMain->Disconnect(true);
	POSITION pos1, pos2;
	for (pos1 = channelLPtrList.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		channelLPtrList.GetNext(pos1);
		ChannelList* cur_channel =	(ChannelList*)channelLPtrList.GetAt(pos2);
		channelLPtrList.RemoveAt(pos2);
		delete cur_channel;
	}
	DeleteAllChannel();
	delete m_pIrcMain;
}

void CIrcWnd::UpdateNickCount(){
	CHeaderCtrl* pHeaderCtrl;
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;
	pHeaderCtrl = nickList.GetHeaderCtrl();
	if( nickList.GetItemCount() )
		strRes.Format( "%s[%i]", GetResString(IDS_IRC_NICK), nickList.GetItemCount());
	else
		strRes.Format( "%s", GetResString(IDS_IRC_NICK));
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(0, &hdi);
	strRes.ReleaseBuffer();
}

void CIrcWnd::Localize(){
	if( m_bConnected )
		GetDlgItem(IDC_BN_IRCCONNECT)->SetWindowText(GetResString(IDS_IRC_DISCONNECT));
	else
		GetDlgItem(IDC_BN_IRCCONNECT)->SetWindowText(GetResString(IDS_IRC_CONNECT));
	GetDlgItem(IDC_CHATSEND)->SetWindowText(GetResString(IDS_IRC_SEND));
	GetDlgItem(IDC_CLOSECHAT)->SetWindowText(GetResString(IDS_FD_CLOSE));

	for (int i = 0; i < channelselect.GetItemCount();i++){
		TCITEM item;
		item.mask = TCIF_PARAM;
		item.lParam = -1;
		channelselect.GetItem(i,&item);
		Channel* cur_chan = (Channel*)item.lParam;
		if (cur_chan != NULL){
			if( cur_chan->type == 1 ){
				cur_chan->title = GetResString(IDS_STATUS);
				item.mask = TCIF_TEXT;
				item.pszText = cur_chan->title.GetBuffer();
				channelselect.SetItem(i,&item);
				cur_chan->title.ReleaseBuffer();
			}
			if( cur_chan->type == 2 ){
				cur_chan->title = GetResString(IDS_IRC_CHANNELLIST);
				item.mask = TCIF_TEXT;
				item.pszText = cur_chan->title.GetBuffer();
				channelselect.SetItem(i,&item);
				cur_chan->title.ReleaseBuffer();
			}
		}
	}
	if (m_pCurrentChannel){
		if( m_pCurrentChannel->type == 1 )
			titleWindow.SetWindowText(GetResString(IDS_STATUS));
		if( m_pCurrentChannel->type == 2 )
			titleWindow.SetWindowText(GetResString(IDS_IRC_CHANNELLIST));
	}

	CHeaderCtrl* pHeaderCtrl;
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

	UpdateNickCount();

	pHeaderCtrl = serverChannelList.GetHeaderCtrl();

	strRes = GetResString(IDS_UUSERS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(1, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_DESCRIPTION);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(2, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_IRC_NAME);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(0, &hdi);
	strRes.ReleaseBuffer();

}

BOOL CIrcWnd::OnInitDialog(){
	CResizableDialog::OnInitDialog();

	imagelist.Create(16,16,theApp.m_iDfltImageListColorFlags | ILC_MASK ,0,10);
	imagelist.Add(theApp.LoadIcon(IDI_CHAT));
	imagelist.Add(theApp.LoadIcon(IDI_MESSAGE));
	imagelist.Add(theApp.LoadIcon(IDI_MPENDING));
	channelselect.SetImageList(&imagelist);

	m_bConnected = false;
	m_bLoggedIn = false;
	Localize();
	m_pIrcMain = new CIrcMain();
	m_pIrcMain->SetIRCWnd(this);

	nickList.InsertColumn(0,GetResString(IDS_IRC_NICK),LVCFMT_LEFT,113);
	nickList.InsertColumn(1,"",LVCFMT_LEFT,60);

	serverChannelList.InsertColumn(0, GetResString(IDS_IRC_NAME), LVCFMT_LEFT, 203 );
	serverChannelList.InsertColumn(1, GetResString(IDS_UUSERS), LVCFMT_LEFT, 50 );
	serverChannelList.InsertColumn(2, GetResString(IDS_DESCRIPTION), LVCFMT_LEFT, 350 );

	CRect rect;
	GetDlgItem(IDC_STATUSWINDOW)->GetWindowRect(rect);
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rect, 2);
	statusWindow.CreateEx(WS_EX_STATICEDGE,0,"MsgWnd",WS_CHILD | HTC_WORDWRAP |HTC_AUTO_SCROLL_BARS | HTC_UNDERLINE_HOVER,rect.left,rect.top,rect.Width(),rect.Height(),m_hWnd,0);
	NewChannel( GetResString(IDS_STATUS), 1 );
	NewChannel( GetResString(IDS_IRC_CHANNELLIST), 2);
	UpdateFonts(&theApp.emuledlg->m_fontHyperText);
	InitWindowStyles(this);

	m_pCurrentChannel = (Channel*)channelPtrList.GetTail();
	AddStatus( GetResString(IDS_IRC_STATUSLOG ));
	titleWindow.SetWindowText(GetResString(IDS_IRC_CHANNELLIST));
	channelselect.SetCurSel(1);
	
	AddAnchor(IDC_BN_IRCCONNECT,BOTTOM_LEFT);
	AddAnchor(IDC_CLOSECHAT,BOTTOM_LEFT);
	AddAnchor(IDC_CHATSEND,BOTTOM_RIGHT);
	AddAnchor(IDC_INPUTWINDOW,BOTTOM_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_NICKLIST,TOP_LEFT,BOTTOM_LEFT);
	AddAnchor(IDC_TITLEWINDOW,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SERVERCHANNELLIST,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(statusWindow,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_TAB2,TOP_LEFT, TOP_RIGHT);

	serverChannelList.SortItems(SortProcChanL, 11);
	serverChannelList.SetSortArrow(1, false);
	nickList.SortItems(SortProcNick, 0);
	return true;
}

void CIrcWnd::UpdateFonts(CFont* pFont)
{
	if (pFont->m_hObject){
		// since the font is selectable this does not make sense any longer. would have to size those controls
		// according the selected font. maybe in some future version.
		//statusWindow.SetFont(pFont);
		//titleWindow.SetFont(pFont);
		//inputWindow.SetFont(pFont);
	}
}

void CIrcWnd::OnSize(UINT nType, int cx, int cy) 
{
	CResizableDialog::OnSize(nType, cx, cy);
}

int CIrcWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) {
	return CResizableDialog::OnCreate(lpCreateStruct);
}

void CIrcWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NICKLIST, nickList);
	DDX_Control(pDX, IDC_INPUTWINDOW, inputWindow);
	DDX_Control(pDX, IDC_TITLEWINDOW, titleWindow);
	DDX_Control(pDX, IDC_SERVERCHANNELLIST, serverChannelList);
	DDX_Control(pDX, IDC_TAB2, channelselect);

}

BOOL CIrcWnd::OnCommand(WPARAM wParam,LPARAM lParam ){ 
   int nickItem= nickList.GetSelectionMark(); 
   int chanItem= channelselect.GetCurSel(); 
   //int chanLItem= serverChannelList.GetSelectionMark(); 
   switch( wParam ){
	   case IDC_BN_IRCCONNECT: {
		   OnBnClickedBnIrcconnect();
		   return true;
	   }
	   case IDC_CHATSEND: {
		   OnBnClickedChatsend();
		   return true;
	   }
	   case IDC_CLOSECHAT: {
		   OnBnClickedClosechat();
		   return true;
	   }
	   case Irc_Priv: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   if(nick)
			   AddInfoMessage( nick->nick, GetResString(IDS_IRC_PRIVATECHANSTART));
		   return true;
	   }
	   case Irc_Owner: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( "PRIVMSG chanserv owner %s %s", chan->name, nick->nick );
			   m_pIrcMain->SendString(send);
		   }
		   return true;
	   }
	   case Irc_DeOwner: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( "PRIVMSG chanserv deowner %s %s", chan->name, nick->nick );
			   m_pIrcMain->SendString(send);
		   }
		   return true;
	   }
	   case Irc_Op: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( "MODE %s +o %s", chan->name, nick->nick );
			   m_pIrcMain->SendString(send);
		   }
		   return true;
	   }
	   case Irc_DeOp: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( "MODE %s -o %s", chan->name, nick->nick );
			   m_pIrcMain->SendString(send);
		   }
		   return true;
	   }
	   case Irc_HalfOp: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( "MODE %s +h %s", chan->name, nick->nick );
			   m_pIrcMain->SendString(send);
		   }
		   return true;
	   }
	   case Irc_DeHalfOp: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( "MODE %s -h %s", chan->name, nick->nick );
			   m_pIrcMain->SendString(send);
		   }
		   return true;
	   }
	   case Irc_Voice: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( "MODE %s +v %s", chan->name, nick->nick );
			   m_pIrcMain->SendString(send);
		   }
		   return true;
	   }
	   case Irc_DeVoice: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( "MODE %s -v %s", chan->name, nick->nick );
			   m_pIrcMain->SendString(send);
		   }
		   return true;
	   }
	   case Irc_Protect: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( "PRIVMSG chanserv protect %s %s", chan->name, nick->nick );
			   m_pIrcMain->SendString(send);
		   }
		   return true;
	   }
	   case Irc_DeProtect: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( "PRIVMSG chanserv deprotect %s %s", chan->name, nick->nick );
			   m_pIrcMain->SendString(send);
		   }
		   return true;
	   }
	   case Irc_Kick: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( "KICK %s %s", chan->name, nick->nick );
			   m_pIrcMain->SendString(send);
		   }
		   return true;
	   }
	   case Irc_Slap: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( GetResString(IDS_IRC_SLAPMSGSEND), chan->name, nick->nick );
			   AddInfoMessage( chan->name, GetResString(IDS_IRC_SLAPMSG), m_pIrcMain->GetNick(), nick->nick);
			   m_pIrcMain->SendString(send);
		   }
		   return true;
	   }
	   case Irc_AddFriend: {
		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( "PRIVMSG %s :\001RQSFRIEND|%i|\001", nick->nick, m_pIrcMain->SetVerify() );
			   m_pIrcMain->SendString(send);
		   }
		   return true;
	   }
	   case Irc_SendLink: {
		   if(!GetSendFileString())
			   return true;
   		   Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		   TCITEM item;
		   item.mask = TCIF_PARAM;
		   channelselect.GetItem(chanItem,&item);
		   Channel* chan = (Channel*)item.lParam;
		   if( nick && chan ){
			   CString send;
			   send.Format( "PRIVMSG %s :\001SENDLINK|%s|%s\001", nick->nick, EncodeBase16((const unsigned char*)theApp.glob_prefs->GetUserHash(), 16), GetSendFileString() );
			   m_pIrcMain->SendString(send);
		   }
		   return true;
		}

	   case Irc_Close: {
		   OnBnClickedClosechat();
		   return true;
	   }
	   case Irc_Join: {
		   JoinChannels();
		   return true;
	   }
   }
   return true;
}

BOOL CIrcWnd::PreTranslateMessage(MSG* pMsg) {
	if(pMsg->message == WM_KEYDOWN && (pMsg->hwnd == GetDlgItem(IDC_INPUTWINDOW)->m_hWnd)) {
		if (pMsg->wParam == VK_RETURN) {
			OnBnClickedChatsend();
			return TRUE;
		}

		if (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN) {
			ScrollHistory(pMsg->wParam == VK_DOWN);
			return TRUE;
		}	   
   }

	return CResizableDialog::PreTranslateMessage(pMsg);
}

void CIrcWnd::OnBnClickedBnIrcconnect()
{
	if(!m_bConnected){
		m_pIrcMain->Connect();
	}
	else{
		m_pIrcMain->Disconnect();
	}
}

void CIrcWnd::OnBnClickedClosechat(int nItem)
{
	TCITEM item;
	item.mask = TCIF_PARAM;

	if (nItem==-1) {
		nItem = channelselect.GetCurSel();
	}
	if (nItem== (-1))
		return;

	channelselect.GetItem(nItem,&item);
	Channel* partChannel = (Channel*)item.lParam;
	if( partChannel ){
		if( partChannel->type == 4 &&  m_bConnected){
			CString part;
			part = "PART " + partChannel->name;
			m_pIrcMain->SendString( part );
			return;
		}
		else if (partChannel->type == 5 || partChannel->type == 4){
			RemoveChannel(partChannel->name);
			return;
		}
	}
}

void CIrcWnd::OnTcnSelchangeTab2(NMHDR *pNMHDR, LRESULT *pResult)
{
//	int index = -1;
//	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	nickList.DeleteAllItems();

	TCITEM item;
	item.mask = TCIF_PARAM;
	int cur_sel = channelselect.GetCurSel();
	if (cur_sel == (-1))
		return;
	channelselect.GetItem(cur_sel,&item);
	Channel* update = (Channel*)item.lParam;
	m_pCurrentChannel = update;
	UpdateNickCount();
	if( m_pCurrentChannel->type == 1 ){
		titleWindow.SetWindowText(GetResString(IDS_STATUS));
		statusWindow.ShowWindow(SW_SHOW);
		serverChannelList.ShowWindow(SW_HIDE);
	}
	if( m_pCurrentChannel->type == 2 ){
		titleWindow.SetWindowText(GetResString(IDS_IRC_CHANNELLIST));
		statusWindow.ShowWindow(SW_HIDE);
		serverChannelList.ShowWindow(SW_SHOW);
		return;
	}
	SetActivity( m_pCurrentChannel->name, false );
	statusWindow.SetHyperText(&m_pCurrentChannel->log);
	statusWindow.ShowWindow(SW_SHOW);
	serverChannelList.ShowWindow(SW_HIDE);
	RefreshNickList( update->name );
	SetTitle( update->name, update->title );
	if( pResult )
		*pResult = 0;
}

BEGIN_MESSAGE_MAP(CIrcWnd, CDialog)
	ON_NOTIFY(NM_CLICK, IDC_NICKLIST, OnNMClickNicklist)
	ON_NOTIFY(NM_DBLCLK, IDC_SERVERCHANNELLIST, OnNMDblclkserverChannelList)
	ON_NOTIFY(NM_DBLCLK, IDC_NICKLIST, OnNMDblclkNickList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_NICKLIST, OnColumnClickNick)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_SERVERCHANNELLIST, OnColumnClickChanL)
	ON_NOTIFY(NM_RCLICK, IDC_NICKLIST, OnNMRclickNick)
	ON_NOTIFY(NM_RCLICK, IDC_SERVERCHANNELLIST, OnNMRclickChanL)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB2, OnTcnSelchangeTab2)
	ON_MESSAGE(WM_CLOSETAB, OnCloseTab)

	ON_WM_SIZE()
	ON_WM_CREATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Channel List
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void CIrcWnd::ResetServerChannelList(){
	POSITION pos1, pos2;
	for (pos1 = channelLPtrList.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		channelLPtrList.GetNext(pos1);
		ChannelList* cur_channel =	(ChannelList*)channelLPtrList.GetAt(pos2);
		channelLPtrList.RemoveAt(pos2);
		delete cur_channel;
	}
	serverChannelList.DeleteAllItems();
}

void CIrcWnd::AddChannelToList( CString name, CString user, CString description ){
	CString ntemp = name;
	CString dtemp = description;
	int usertest = atoi(user);
	if( (theApp.glob_prefs->GetIRCChanNameFilter() || theApp.glob_prefs->GetIRCChannelUserFilter()) && theApp.glob_prefs->GetIRCUseChanFilter()){
		if( usertest < theApp.glob_prefs->GetIRCChannelUserFilter() )
			return;
		if( dtemp.MakeLower().Find(theApp.glob_prefs->GetIRCChanNameFilter().MakeLower()) == -1 && ntemp.MakeLower().Find(theApp.glob_prefs->GetIRCChanNameFilter().MakeLower()) == -1)
			return;
	}
	ChannelList* toadd = new ChannelList;
	toadd->name = name;
	toadd->users = user;
	toadd->desc = StripMessageOfFontCodes(description);
	channelLPtrList.AddTail( toadd);
	uint16 itemnr = serverChannelList.GetItemCount();
	itemnr = serverChannelList.InsertItem(LVIF_PARAM,itemnr,0,0,0,0,(LPARAM)toadd);
	serverChannelList.SetItemText(itemnr,0,toadd->name);
	serverChannelList.SetItemText(itemnr,1,toadd->users);
	serverChannelList.SetItemText(itemnr,2,toadd->desc);
}

int CIrcWnd::SortProcChanL(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort){
	ChannelList* item1 = (ChannelList*)lParam1;
	ChannelList* item2 = (ChannelList*)lParam2;
		switch(lParamSort){
		case 0: 
			return CString(item1->name).CompareNoCase(item2->name);
		case 10:
			return CString(item2->name).CompareNoCase(item1->name);
		case 1: 
			return atoi(item1->users) - atoi(item2->users);
		case 11:
			return atoi(item2->users) - atoi(item1->users);
		case 2: 
			return CString(item1->desc).CompareNoCase(item2->desc);
		case 12:
			return CString(item2->desc).CompareNoCase(item1->desc);
		default:
			return 0;
	}
}

void CIrcWnd::OnColumnClickChanL( NMHDR* pNMHDR, LRESULT* pResult){

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	asc_sort[pNMListView->iSubItem] = !asc_sort[pNMListView->iSubItem];
	serverChannelList.SetSortArrow(pNMListView->iSubItem, asc_sort[pNMListView->iSubItem]);
	serverChannelList.SortItems(SortProcChanL, pNMListView->iSubItem+ ((asc_sort[pNMListView->iSubItem])? 0:10));
	*pResult = 0;
}

void CIrcWnd::OnNMRclickChanL(NMHDR *pNMHDR, LRESULT *pResult){
	POINT point; 
	::GetCursorPos(&point); 
	CPoint p = point; 
	ScreenToClient(&p); 
	CTitleMenu m_ChanLMenu;
	m_ChanLMenu.CreatePopupMenu();
	m_ChanLMenu.AddMenuTitle(GetResString(IDS_IRC_CHANNEL));
	m_ChanLMenu.AppendMenu(MF_STRING,Irc_Join,GetResString(IDS_IRC_JOIN)); 
	m_ChanLMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this); 
	VERIFY( m_ChanLMenu.DestroyMenu() );
	*pResult = 0; 
}

void CIrcWnd::OnNMDblclkserverChannelList(NMHDR *pNMHDR, LRESULT *pResult){
	JoinChannels();
	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Nick List
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

Nick* CIrcWnd::FindNickByName(CString channel, CString name){
	Channel* curr_channel = FindChannelByName(channel);
	if( !curr_channel)
		return 0;
	POSITION pos1, pos2;
	for (pos1 = curr_channel->nicks.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		curr_channel->nicks.GetNext(pos1);
		Nick* cur_nick = (Nick*)curr_channel->nicks.GetAt(pos2);
		if (cur_nick->nick == name)
			return cur_nick;
	}
	return 0;
}

Nick* CIrcWnd::NewNick( CString channel, CString nick ){
	Channel* toaddchan = FindChannelByName( channel );
	if( !toaddchan )
		return NULL;
	Nick* toaddnick=NULL;
	if(toaddchan){
		toaddnick = new Nick;

		if( nick.Left(1) == "!" ){
			nick = nick.Mid(1);
			toaddnick->owner = "!";
		}
		else
			toaddnick->owner = "";

		if( nick.Left(1) == "*" ){
			nick = nick.Mid(1);
			toaddnick->protect = "*";
		}
		else
			toaddnick->protect = "";

		if( nick.Left(1) == "@" ){
			toaddnick->op = "@";
			toaddnick->hop = "";
			toaddnick->voice = "";
			toaddnick->uop = "";
			toaddnick->nick = nick.Mid(1);
		}
		else if( nick.Left(1) == "%" ){
			toaddnick->op = "";
			toaddnick->hop = "%";
			toaddnick->voice = "";
			toaddnick->uop = "";
			toaddnick->nick = nick.Mid(1);
		}
		else if( nick.Left(1) == "+"){
			toaddnick->op = "";
			toaddnick->hop = "";
			toaddnick->voice = "+";
			toaddnick->uop = "";
			toaddnick->nick = nick.Mid(1);
		}
		else if( nick.Left(1) == "-"){
			toaddnick->op = "";
			toaddnick->hop = "";
			toaddnick->voice = "";
			toaddnick->uop = "-";
			toaddnick->nick = nick.Mid(1);
		}
		else{
			toaddnick->op = "";
			toaddnick->hop = "";
			toaddnick->voice = "";
			toaddnick->nick = nick;
		}
		toaddchan->nicks.AddTail(toaddnick);
		if( toaddchan == m_pCurrentChannel ){
			uint16 itemnr = nickList.GetItemCount();
			itemnr = nickList.InsertItem(LVIF_PARAM,itemnr,0,0,0,0,(LPARAM)toaddnick);
			nickList.SetItemText(itemnr,0,(LPCTSTR)toaddnick->nick);
			CString mode;
			mode.Format("%s%s%s%s%s%s", toaddnick->owner, toaddnick->protect, toaddnick->op, toaddnick->hop, toaddnick->voice, toaddnick->uop);
			nickList.SetItemText(itemnr,1,(LPCTSTR)mode);
			UpdateNickCount();
		}
	}
	return toaddnick;
}

void CIrcWnd::RefreshNickList( CString channel ){
	nickList.DeleteAllItems();
	Channel* refresh = FindChannelByName( channel );
	if(!refresh )
		return;
	POSITION pos1, pos2;
	for (pos1 = refresh->nicks.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		refresh->nicks.GetNext(pos1);
		Nick* curr_nick = (Nick*)refresh->nicks.GetAt(pos2);
		uint16 itemnr = nickList.GetItemCount();
		itemnr = nickList.InsertItem(LVIF_PARAM,itemnr,0,0,0,0,(LPARAM)curr_nick);
		nickList.SetItemText(itemnr,0,(LPCTSTR)curr_nick->nick);
		CString mode;
		mode.Format("%s%s%s%s%s%s",curr_nick->owner, curr_nick->protect, curr_nick->op, curr_nick->hop, curr_nick->voice, curr_nick->uop);
		nickList.SetItemText(itemnr,1,(LPCTSTR)mode);
	}
	UpdateNickCount();
}

bool CIrcWnd::RemoveNick( CString channel, CString nick ){
	Channel* update = FindChannelByName( channel );
	if( !update )
		return false;
	POSITION pos1, pos2;
	for( pos1 = update->nicks.GetHeadPosition();(pos2=pos1)!=NULL;){
		update->nicks.GetNext(pos1);
		Nick* curr_nick = (Nick*)update->nicks.GetAt(pos2);
		if( curr_nick->nick == nick ){
			if( update == m_pCurrentChannel ){
				LVFINDINFO find;
				find.flags = LVFI_PARAM;
				find.lParam = (LPARAM)curr_nick;
				sint32 result = nickList.FindItem(&find);
				nickList.DeleteItem(result);
				UpdateNickCount();
			}
			update->nicks.RemoveAt(pos2);
			delete curr_nick;
			return true;
		}
	}
	return false;
}

void CIrcWnd::DeleteAllNick( CString channel ){
	Channel* curr_channel = FindChannelByName(channel);
	if( !curr_channel )
		return;
	POSITION pos3, pos4;
	for(pos3 = curr_channel->nicks.GetHeadPosition();( pos4 = pos3) != NULL;){
		curr_channel->nicks.GetNext(pos3);
		Nick* cur_nick = (Nick*)curr_channel->nicks.GetAt(pos4);
		curr_channel->nicks.RemoveAt(pos4);
		delete cur_nick;
	}
}

void CIrcWnd::DeleteNickInAll( CString nick, CString message ){
	POSITION pos1, pos2;
	for (pos1 = channelPtrList.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		channelPtrList.GetNext(pos1);
		Channel* cur_channel = (Channel*)channelPtrList.GetAt(pos2);
		if(RemoveNick( cur_channel->name, nick )){
			if( !theApp.glob_prefs->GetIrcIgnoreInfoMessage() )
				AddInfoMessage( cur_channel->name, GetResString(IDS_IRC_HASQUIT), nick, message);
		}
	}
}

bool CIrcWnd::ChangeNick( CString channel, CString oldnick, CString newnick ){
	Channel* update = FindChannelByName( channel );
	if( !update )
		return false;
	POSITION pos1, pos2;
	for( pos1 = update->nicks.GetHeadPosition();(pos2=pos1)!=NULL;){
		update->nicks.GetNext(pos1);
		Nick* curr_nick = (Nick*)update->nicks.GetAt(pos2);
		if( curr_nick->nick == oldnick ){
			if( update = m_pCurrentChannel ){
				LVFINDINFO find;
				find.flags = LVFI_PARAM;
				find.lParam = (LPARAM)curr_nick;
				sint32 itemnr = nickList.FindItem(&find);
				if (itemnr != (-1))
					nickList.SetItemText(itemnr,0,(LPCTSTR)newnick);
			}
			curr_nick->nick = newnick;
			return true;
		}
	}
	return false;
}

bool CIrcWnd::ChangeMode( CString channel, CString nick, CString mode ){
	Channel* update = FindChannelByName( channel );
	if( !update )
		return false;
	POSITION pos1, pos2;
	for( pos1 = update->nicks.GetHeadPosition();(pos2=pos1)!=NULL;){
		update->nicks.GetNext(pos1);
		Nick* curr_nick = (Nick*)update->nicks.GetAt(pos2);
		if( curr_nick->nick == nick ){
			sint32 itemnr = -1;
			if( update = m_pCurrentChannel ){
				LVFINDINFO find;
				find.flags = LVFI_PARAM;
				find.lParam = (LPARAM)curr_nick;
				itemnr = nickList.FindItem(&find);
			}
			if( mode == "+a" ){
				curr_nick->protect = "*";
			}
			if( mode == "-a" ){
				curr_nick->protect = "";
			}
			if(mode == "+h"){
				curr_nick->hop = "%";
			}
			if(mode == "-h"){
				curr_nick->hop = "";
			}
			if(mode == "+o"){
				curr_nick->op = "@";
			}
			if(mode == "-o"){
				curr_nick->op = "";
			}
			if( mode == "+q" ){
				curr_nick->owner = "!";
			}
			if( mode == "-q" ){
				curr_nick->owner = "";
			}
			if( mode == "+u" ){
				curr_nick->uop = "-";
			}
			if( mode == "-u" ){
				curr_nick->uop = "";
			}
			if( mode == "+v" ){
				curr_nick->voice = "+";
			}
			if( mode == "-v" ){
				curr_nick->voice = "";
			}
			if( itemnr != (-1) ){
					CString mode;
					mode.Format("%s%s%s%s%s%s",curr_nick->owner, curr_nick->protect, curr_nick->op, curr_nick->hop, curr_nick->voice, curr_nick->uop);
					nickList.SetItemText(itemnr,1,(LPCTSTR)mode);
			}
		}
	}
	return true;
}

void CIrcWnd::ParseChangeMode( CString channel, CString changer, CString commands, CString names ){
	try{
	if( commands.GetLength() == 2 ){
		if( ChangeMode( channel, names, commands ))
			if( !theApp.glob_prefs->GetIrcIgnoreInfoMessage() )
				AddInfoMessage( channel, GetResString(IDS_IRC_SETSMODE), changer, commands, names);
		return;
	}
		else{
			CString dir;
			dir = commands[0];
			if( dir == "+" || dir == "-"){
				int currMode = 1;
				int currName = 0;
				int currNameBack = names.Find( " ", currName);
				while( currMode < commands.GetLength()){
					CString test;
					test = names.Mid(currName, currNameBack-currName);
					currName = currNameBack +1;
					if( ChangeMode( channel, test, dir + commands[currMode]))
						if( !theApp.glob_prefs->GetIrcIgnoreInfoMessage() )
							AddInfoMessage( channel, GetResString(IDS_IRC_SETSMODE), changer, dir + commands[currMode] , test);
					currNameBack = names.Find(" ", currName+1);
					if( currNameBack == -1)
						currNameBack = names.GetLength();
					currMode++;
				}
			}
		}
	}
	catch(...){
	AddInfoMessage( channel, GetResString(IDS_IRC_NOTSUPPORTED));
}
}

void CIrcWnd::ChangeAllNick( CString oldnick, CString newnick ){
	Channel* currchannel = FindChannelByName( oldnick );
	if( currchannel ){
		currchannel->name = newnick;
		TCITEM item;
		item.mask = TCIF_PARAM;
		item.lParam = -1;
		int i;
		for (i = 0; i < channelselect.GetItemCount();i++){
			channelselect.GetItem(i,&item);
			if (((Channel*)item.lParam) == currchannel)
				break;
		}
		if (((Channel*)item.lParam) != currchannel)
			return;
		item.mask = TCIF_TEXT;
		item.pszText = newnick.GetBuffer();
		item.cchTextMax = (int)newnick.GetLength()+1;
		channelselect.SetItem( i, &item);
	}
	POSITION pos1, pos2;
	for (pos1 = channelPtrList.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		channelPtrList.GetNext(pos1);
		Channel* cur_channel = (Channel*)channelPtrList.GetAt(pos2);
		if(ChangeNick( cur_channel->name, oldnick, newnick )){
			if( !theApp.glob_prefs->GetIrcIgnoreInfoMessage() )
				AddInfoMessage( cur_channel->name, GetResString(IDS_IRC_NOWKNOWNAS), oldnick, newnick);
		}
	}
}

/*
void CIrcWnd::SetNick( CString in_nick ){
	theApp.glob_prefs->SetIRCNick( in_nick.GetBuffer() );
	//Need to also update the preference window for this to work right..
}
*/

int CIrcWnd::SortProcNick(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort){
	Nick* item1 = (Nick*)lParam1;
	Nick* item2 = (Nick*)lParam2;
		switch(lParamSort){
		case 0: 
			if( item1->owner == "!" ){
				if( item2->owner != "!" )
					return -1;
			}
			else if( item2->owner == "!" ){
				return 1;
			}
			if( item1->protect == "*" ){
				if( item2->protect != "*" )
					return -1;
			}
			else if( item2->protect == "*" ){
				return 1;
			}
			if( item1->op == "@" ){
				if( item2->op != "@" )
					return -1;
			}
			else if( item2->op == "@" ){
				return 1;
			}
			if( item1->hop == "%" ){
				if( item2->hop != "%" )
					return -1;
			}
			else if( item2->hop == "%" ){
				return 1;
			}
			if( item1->voice == "+" ){
				if( item2->voice != "+" )
					return -1;
			}
			else if( item2->voice == "+" ){
				return 1;
			}
			if( item1->uop == "-" ){
				if( item2->uop != "-" )
					return -1;
			}
			else if( item2->uop == "-" ){
				return 1;
			}
			return CString(item1->nick).CompareNoCase(item2->nick);
		case 10:
			if( item1->owner == "!" ){
				if( item2->owner != "!" )
					return -1;
			}
			else if( item2->owner == "!" ){
				return 1;
			}
			if( item1->op == "@" ){
				if( item2->op != "@" )
					return -1;
			}
			else if( item2->op == "@" ){
				return 1;
			}
			if( item1->voice == "+" ){
				if( item2->voice != "+" )
					return -1;
			}
			else if( item2->voice == "+" ){
				return 1;
			}
			if( item1->uop == "-" ){
				if( item2->uop != "-" )
					return -1;
			}
			else if( item2->uop == "-" ){
				return 1;
			}
			return CString(item1->nick).CompareNoCase(item2->nick);
		default:
			return 0;
	}
}

void CIrcWnd::OnColumnClickNick( NMHDR* pNMHDR, LRESULT* pResult){

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	asc_sort[pNMListView->iSubItem] = !asc_sort[pNMListView->iSubItem];
//	nickList.SetSortArrow(pNMListView->iSubItem, asc_sort[pNMListView->iSubItem]);
	nickList.SortItems(SortProcNick, pNMListView->iSubItem+ ((asc_sort[pNMListView->iSubItem])? 0:10));
	*pResult = 0;
}

void CIrcWnd::OnNMRclickNick(NMHDR *pNMHDR, LRESULT *pResult) 
{ 
	
	if (nickList.GetSelectionMark() != (-1) ){
		POINT point; 
		::GetCursorPos(&point); 
		CPoint p = point; 
		ScreenToClient(&p); 
		CTitleMenu m_NickMenu;
		m_NickMenu.CreatePopupMenu(); 
		m_NickMenu.AddMenuTitle(GetResString(IDS_IRC_NICK));
		m_NickMenu.AppendMenu(MF_STRING,Irc_Priv,GetResString(IDS_IRC_PRIVMESSAGE));
		m_NickMenu.AppendMenu(MF_STRING,Irc_AddFriend,GetResString(IDS_IRC_ADDTOFRIENDLIST));
		if( !GetSendFileString().IsEmpty() )
			m_NickMenu.AppendMenu(MF_STRING,Irc_SendLink, (CString)GetResString(IDS_IRC_SENDLINK) + GetSendFileString() );
		else
			m_NickMenu.AppendMenu(MF_STRING,Irc_SendLink, (CString)GetResString(IDS_IRC_SENDLINK) + "No Shared File Selected" );
		m_NickMenu.AppendMenu(MF_STRING,Irc_Slap,GetResString(IDS_IRC_SLAP));
		m_NickMenu.AppendMenu(MF_STRING,Irc_Owner,"Owner");
		m_NickMenu.AppendMenu(MF_STRING,Irc_DeOwner,"DeOwner");
		m_NickMenu.AppendMenu(MF_STRING,Irc_Op,GetResString(IDS_IRC_OP));
		m_NickMenu.AppendMenu(MF_STRING,Irc_DeOp,GetResString(IDS_IRC_DEOP));
		m_NickMenu.AppendMenu(MF_STRING,Irc_HalfOp,GetResString(IDS_IRC_HALFOP));
		m_NickMenu.AppendMenu(MF_STRING,Irc_DeHalfOp,GetResString(IDS_IRC_DEHALFOP));
		m_NickMenu.AppendMenu(MF_STRING,Irc_Voice,GetResString(IDS_IRC_VOICE));
		m_NickMenu.AppendMenu(MF_STRING,Irc_DeVoice,GetResString(IDS_IRC_DEVOICE));
		m_NickMenu.AppendMenu(MF_STRING,Irc_Protect,"Protect");
		m_NickMenu.AppendMenu(MF_STRING,Irc_DeProtect,"DeProtect");
		m_NickMenu.AppendMenu(MF_STRING,Irc_Kick,GetResString(IDS_IRC_KICK));
		m_NickMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this); 
		VERIFY( m_NickMenu.DestroyMenu() );
	}
   *pResult = 0; 
}

void CIrcWnd::OnNMDblclkNickList(NMHDR *pNMHDR, LRESULT *pResult){
	int nickItem= nickList.GetSelectionMark();
	if(nickItem != -1) {
		Nick* nick = (Nick*)nickList.GetItemData(nickItem);
		if(nick)
			AddInfoMessage( nick->nick, GetResString(IDS_IRC_PRIVATECHANSTART));
	}
	*pResult = 0;
}

void CIrcWnd::OnNMClickNicklist(NMHDR *pNMHDR, LRESULT *pResult)
{
	//LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Messages
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void CIrcWnd::AddStatus( CString line,...){
	va_list argptr;
	va_start(argptr, line);
	CString temp;
	temp.FormatV(line, argptr);
	va_end(argptr);
	CString timestamp = "";
	if( theApp.glob_prefs->GetIRCAddTimestamp() )
		timestamp = CTime::GetCurrentTime().Format("%X: ");
	Channel* update_channel = (Channel*)channelPtrList.GetHead();
	if( !update_channel ){
		return;
	}
	line = StripMessageOfFontCodes( temp );
	line += "\r\n";
	line.Replace( "\004", "%" );
	if (line.Mid(0,1) == "*"){
		update_channel->log.AppendText(timestamp);
		update_channel->log.AppendKeyWord(line.Left(2),RGB(255,0,0));
		update_channel->log.AppendText(line.Mid(1) );
	}
	else if (line.Mid(0,1) == "-"){
		int index = line.Find( "-", 1 );
		update_channel->log.AppendText(timestamp);
		update_channel->log.AppendKeyWord(line.Left(index),RGB(150,0,0));
		update_channel->log.AppendText(line.Mid(index) );
	}
	else
		update_channel->log.AppendText(timestamp + line);
	if( m_pCurrentChannel == update_channel ){
		statusWindow.SetHyperText(&update_channel->log,true);
		return;
	}
	SetActivity( update_channel->name, true );
}

void CIrcWnd::AddInfoMessage( CString channelName, CString line,...){
	if(channelName.IsEmpty())
		return;
	va_list argptr;
	va_start(argptr, line);
	CString temp;
	temp.FormatV(line, argptr);
	va_end(argptr);
	CString timestamp = "";
	if( theApp.glob_prefs->GetIRCAddTimestamp() )
		timestamp = CTime::GetCurrentTime().Format("%X: ");
	Channel* update_channel = FindChannelByName(channelName);
	if( !update_channel ){
		if( channelName.Left(1) == "#" )
			update_channel = NewChannel( channelName, 4);
		else
			update_channel = NewChannel( channelName, 5);
	}
	line = StripMessageOfFontCodes( temp );
	line += "\r\n";
	line.Replace( "\004", "%" );
	if (line.Mid(0,1) == "*"){
		update_channel->log.AppendText(timestamp);
		update_channel->log.AppendKeyWord(line.Left(2),RGB(255,0,0));
		update_channel->log.AppendText(line.Mid(1) );
	}
	else if (line.Mid(0,1) == "-"){
		int index = line.Find( "-", 1 );
		update_channel->log.AppendText(timestamp);
		update_channel->log.AppendKeyWord(line.Left(index),RGB(150,0,0));
		update_channel->log.AppendText(line.Mid(index) );
	}
	else
		update_channel->log.AppendText(timestamp + line);
	
	if( m_pCurrentChannel == update_channel ){
		statusWindow.SetHyperText(&update_channel->log,true);
		return;
	}
	SetActivity( update_channel->name, true );
}

void CIrcWnd::AddMessage( CString channelName, CString targetname, CString line,...){
	if(channelName.IsEmpty() || targetname.IsEmpty())
		return;
	va_list argptr;
	va_start(argptr, line);
	CString temp;
	temp.FormatV(line, argptr);
	line = temp;
	va_end(argptr);
	CString timestamp = "";
	if( theApp.glob_prefs->GetIRCAddTimestamp() )
		timestamp = CTime::GetCurrentTime().Format("%X: ");
	Channel* update_channel = FindChannelByName(channelName);
	if( !update_channel ){
		if( channelName.Left(1) == "#" )
			update_channel = NewChannel( channelName, 4);
		else
			update_channel = NewChannel( channelName, 5);
	}
	line = StripMessageOfFontCodes( line );
	line += "\r\n";
	line.Replace( "\004", "%" );
	COLORREF color;
	if (m_pIrcMain->GetNick() == targetname)
		color = RGB(1,100,1);
	else
		color = RGB(1,20,130);	
	targetname = CString("<")+ targetname + CString(">");
	update_channel->log.AppendText(timestamp);
	update_channel->log.AppendKeyWord(targetname, color);
	update_channel->log.AppendText(CString(" ")+line);
	if( m_pCurrentChannel == update_channel ){
		statusWindow.SetHyperText(&update_channel->log,true);
		return;
	}
	SetActivity( update_channel->name, true );	
}

void CIrcWnd::SetConnectStatus( bool flag ){
	if(flag){
		GetDlgItem(IDC_BN_IRCCONNECT)->SetWindowText(GetResString(IDS_IRC_DISCONNECT));
		AddStatus( GetResString(IDS_CONNECTED));
		m_bConnected = true;
	}
	else{
		GetDlgItem(IDC_BN_IRCCONNECT)->SetWindowText(GetResString(IDS_IRC_CONNECT));
		AddStatus( GetResString(IDS_DISCONNECTED));
		m_bConnected = false;
		m_bLoggedIn = false;
		while( channelPtrList.GetCount() > 2 ){
			Channel* todel = (Channel*)channelPtrList.GetTail();
			RemoveChannel( todel->name );
		}
	}
}

void CIrcWnd::NoticeMessage( CString source, CString message ){
	bool flag = false;
	Channel* curr_channel = FindChannelByName( source );
	if( curr_channel ){
		AddInfoMessage( source, "-%s- %s", source, message);
		flag = true;
	}
	POSITION pos1, pos2;
	for (pos1 = channelPtrList.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		channelPtrList.GetNext(pos1);
		curr_channel = (Channel*)channelPtrList.GetAt(pos2);
		Nick* curr_nick = FindNickByName(curr_channel->name, source );
		if( curr_nick){
			AddInfoMessage( curr_channel->name, "-%s- %s", source, message);
			flag = true;			
		}
	}
	if( flag == false ){
		if( m_pCurrentChannel->type == 4 )
			AddInfoMessage( m_pCurrentChannel->name, "-%s- %s", source, message);
		else
			AddStatus( "-%s- %s", source, message );
	}
}

//We cannot support color within the text since HyperTextCtrl does not detect hyperlinks with color. So I will filter it.
CString CIrcWnd::StripMessageOfFontCodes( CString temp ){
	temp = StripMessageOfColorCodes( temp );
	temp.Replace("\002","");
	temp.Replace("\003","");
	temp.Replace("\017","");
	temp.Replace("\026","");
	temp.Replace("\047","");
	return temp;
}

CString CIrcWnd::StripMessageOfColorCodes( CString temp ){
	if( !temp.IsEmpty() )
	{
		CString temp1, temp2;
		int test = temp.Find( 3 );
		if( test != -1 ){
			int testlength = temp.GetLength() - test;
			if( testlength < 2 )
				return temp;
			temp1 = temp.Left( test );
			temp2 = temp.Mid( test + 2);
			if( testlength < 4 )
				return temp1+temp2;
			if( temp2[0] == 44 && temp2.GetLength() > 2){
				temp2 = temp2.Mid(2);
				for( int I = 48; I < 58; I++ ){
					if( temp2[0] == I ){
						temp2 = temp2.Mid(1);
					}
				}
			}
			else{
				for( int I = 48; I < 58; I++ ){
					if( temp2[0] == I ){
						temp2 = temp2.Mid(1);
						if( temp2[0] == 44 && temp2.GetLength() > 2){
							temp2 = temp2.Mid(2);
							for( int I = 48; I < 58; I++ ){
								if( temp2[0] == I ){
									temp2 = temp2.Mid(1);
								}
							}
						}
					}
				}
			}
			temp = temp1 + temp2;
			temp = StripMessageOfColorCodes(temp);
		}
	}
	return temp;
}

void CIrcWnd::SetTitle( CString channel, CString title ){
	Channel* curr_channel = FindChannelByName(channel);
	if(!curr_channel)
		return;
	curr_channel->title = StripMessageOfFontCodes(title);
	if( curr_channel == m_pCurrentChannel )
		titleWindow.SetWindowText( curr_channel->title );
}

void CIrcWnd::SetActivity( CString channel, bool flag){
	Channel* refresh = FindChannelByName( channel );
	if( !refresh ){
		refresh = (Channel*)channelPtrList.GetHead();
		if( !refresh )
			return;
	}
	TCITEM item;
	item.mask = TCIF_PARAM;
	item.lParam = -1;
	int i;
	for (i = 0; i < channelselect.GetItemCount();i++){
		channelselect.GetItem(i,&item);
		if (((Channel*)item.lParam) == refresh)
			break;
	}
	if (((Channel*)item.lParam) != refresh)
		return;
    if( flag ){
		item.mask = TCIF_IMAGE;
 	   item.iImage = 2;
	   channelselect.SetItem( i, &item );
    }
	else{
	   item.mask = TCIF_IMAGE;
 	   item.iImage = 1;
	   channelselect.SetItem( i, &item );
    }
}

void CIrcWnd::OnBnClickedChatsend()
{
	CString send;
	GetDlgItem(IDC_INPUTWINDOW)->GetWindowText(send);
	GetDlgItem(IDC_INPUTWINDOW)->SetWindowText("");

	if (m_pCurrentChannel->history.GetCount()==theApp.glob_prefs->GetMaxChatHistoryLines()) m_pCurrentChannel->history.RemoveAt(0);
	m_pCurrentChannel->history.Add(send);
	m_pCurrentChannel->history_pos=m_pCurrentChannel->history.GetCount();

	if( send.IsEmpty() )
		return;

	if( !this->m_bConnected )
		return;
	if( send.Left(1) == "/" && send.Left(3) != "/me"){
		if (send.Left(4) == "/msg"){
			if( m_pCurrentChannel->type == 4 || m_pCurrentChannel->type == 5){
				send.Replace( "%", "\004" );
				AddInfoMessage( m_pCurrentChannel->name ,CString("* >> ")+send.Mid(5));
				send.Replace( "\004", "%" );
			}
			else{
				send.Replace( "%", "\004" );
				AddStatus( CString("* >> ")+send.Mid(5));
				send.Replace( "\004", "%" );
			}
			send = CString("/PRIVMSG") + send.Mid(4);
			
		}
		if( ((CString)send.Left(17)).CompareNoCase( "/PRIVMSG nickserv"  )== 0){
			send = CString("/ns") + send.Mid(17);
		}
		if( ((CString)send.Left(17)).CompareNoCase( "/PRIVMSG chanserv" )== 0){
			send = CString("/cs") + send.Mid(17);
		}
		m_pIrcMain->SendString(send.Mid(1));
		return;
	}
	if( m_pCurrentChannel->type < 4 ){
		m_pIrcMain->SendString(send);
		return;
	}
	if( send.Left(3) == "/me" ){
		CString build;
	   build.Format( "PRIVMSG %s :\001ACTION %s\001", m_pCurrentChannel->name, send.Mid(4) );
	   send.Replace( "%", "\004" );
	   AddInfoMessage( m_pCurrentChannel->name, "* %s %s", m_pIrcMain->GetNick(), send.Mid(4));
	   send.Replace( "\004", "%" );
	   m_pIrcMain->SendString(build);
	   return;
	}
	CString build = "PRIVMSG " + m_pCurrentChannel->name + " :" + send;
	m_pIrcMain->SendString(build);
	send.Replace( "%", "\004" );
	AddMessage( m_pCurrentChannel->name, m_pIrcMain->GetNick(), send );
	send.Replace( "\004", "%" );
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Channels
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

Channel* CIrcWnd::FindChannelByName(CString name){
	POSITION pos1, pos2;
	for (pos1 = channelPtrList.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		channelPtrList.GetNext(pos1);
		Channel* cur_channel = (Channel*)channelPtrList.GetAt(pos2);
		if (cur_channel->name.CompareNoCase(name.Trim()) == 0 && (cur_channel->type == 4 || cur_channel->type == 5))
			return cur_channel;
	}
	return 0;
}

Channel* CIrcWnd::NewChannel( CString name, uint8 type ){
	Channel* toadd = new Channel;
	toadd->name = name;
	toadd->title = name;
	toadd->type = type;
	toadd->history_pos=0;
	channelPtrList.AddTail(toadd);
	TCITEM newitem;
	newitem.mask = TCIF_PARAM|TCIF_TEXT|TCIF_IMAGE;
	newitem.lParam = (LPARAM)toadd;
	newitem.pszText = name.GetBuffer();
	newitem.cchTextMax = (int)name.GetLength()+1;
	newitem.iImage = 1;
	channelselect.InsertItem(channelselect.GetItemCount(),&newitem);
	return toadd;
}

void CIrcWnd::RemoveChannel( CString channel ){
	Channel* todel = FindChannelByName( channel );
	if( !todel )
		return;
	TCITEM item;
	item.mask = TCIF_PARAM;
	item.lParam = -1;
	int i;
	for (i = 0; i < channelselect.GetItemCount();i++){
		channelselect.GetItem(i,&item);
		if (((Channel*)item.lParam) == todel)
			break;
	}
	if (((Channel*)item.lParam) != todel)
		return;
	channelselect.DeleteItem(i);

	if( todel == m_pCurrentChannel ){
		nickList.DeleteAllItems();
		statusWindow.SetWindowText("");
		if( channelselect.GetItemCount() > 2 && i > 1 ) {
			if ( i == 2 )
				i++;
			channelselect.SetCurSel(i-1);
			channelselect.SetCurFocus(i-1);
			OnTcnSelchangeTab2( NULL, NULL );
		}
		else {
			channelselect.SetCurSel(0);
			channelselect.SetCurFocus(0);
			OnTcnSelchangeTab2( NULL, NULL );
		}
	}
	DeleteAllNick(todel->name);
	channelPtrList.RemoveAt(channelPtrList.Find(todel));
	delete todel;
}

void CIrcWnd::DeleteAllChannel(){
	POSITION pos1, pos2;
	for (pos1 = channelPtrList.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		channelPtrList.GetNext(pos1);
		Channel* cur_channel =	(Channel*)channelPtrList.GetAt(pos2);
		DeleteAllNick(cur_channel->name);
		channelPtrList.RemoveAt(pos2);
		delete cur_channel;
	}
}

void CIrcWnd::JoinChannels(){
	if( !this->m_bConnected ) return;

	int index = -1; 
	POSITION pos = serverChannelList.GetFirstSelectedItemPosition(); 
	while(pos != NULL) 
	{ 
		index = serverChannelList.GetNextSelectedItem(pos); 
		if(index > -1){ 
			CString join;
			join = "JOIN " + serverChannelList.GetItemText(index, 0 );
			m_pIrcMain->SendString( join );
		}
	} 
}


LRESULT CIrcWnd::OnCloseTab(WPARAM wparam, LPARAM lparam) {
	OnBnClickedClosechat( (int)wparam );
	return true;
}

void CIrcWnd::SendString( CString send ){ 
	if( this->m_bConnected ) m_pIrcMain->SendString( send );
}

void CIrcWnd::ScrollHistory(bool down) {
	CString buffer;

	if ( (m_pCurrentChannel->history_pos==0 && !down) || (m_pCurrentChannel->history_pos==m_pCurrentChannel->history.GetCount() && down)) return;
	
	
	if (down) ++m_pCurrentChannel->history_pos; else --m_pCurrentChannel->history_pos;

	buffer= (m_pCurrentChannel->history_pos==m_pCurrentChannel->history.GetCount())?"":m_pCurrentChannel->history.GetAt(m_pCurrentChannel->history_pos);

	GetDlgItem(IDC_INPUTWINDOW)->SetWindowText(buffer);
	inputWindow.SetSel(buffer.GetLength(),buffer.GetLength());
}
