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
#include "IrcWnd.h"
#include "IrcMain.h"
#include "emuledlg.h"
#include "otherfunctions.h"
#include "MenuCmds.h"
#include "HTRichEditCtrl.h"
#include "ClosableTabCtrl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define NICK_LV_PROFILE_NAME _T("IRCNicksLv")
#define CHAN_LV_PROFILE_NAME _T("IRCChannelsLv")


struct ChannelList
{
	CString name;
	CString users;
	CString desc;
};

struct Channel
{
	CString	name;
	CHTRichEditCtrl log;
	CString title;
	CPtrList nicks;
	uint8 type;
	CStringArray history;
	uint16 history_pos;
	// Type is mainly so that we can use this for IRC and the eMule Messages..
	// 1-Status, 2-Channel list, 4-Channel, 5-Private Channel, 6-eMule Message(Add later)
};

struct Nick{
	CString nick;
	CString op;
	CString hop;
	CString voice;
	CString uop;
	CString owner;
	CString protect;
};


/////////////////////////////////////////////////////////////////////////////////////////
// CIrcWnd dialog
//

IMPLEMENT_DYNAMIC(CIrcWnd, CDialog)

BEGIN_MESSAGE_MAP(CIrcWnd, CDialog)
	// Tab control
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB2, OnTcnSelchangeTab2)
	ON_MESSAGE(WM_CLOSETAB, OnCloseTab)

	ON_WM_SIZE()
	ON_WM_CREATE()
    ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

CIrcWnd::CIrcWnd(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CIrcWnd::IDD, pParent)
{
   m_pIrcMain = NULL;
   m_bConnected = false;
   m_bLoggedIn = false;
   m_pCurrentChannel = NULL;
   nickList.m_pParent = this;
   serverChannelList.m_pParent = this;
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

void CIrcWnd::OnSysColorChange()
{ 
	CResizableDialog::OnSysColorChange();
	SetAllIcons();
}

void CIrcWnd::SetAllIcons()
{ 
	CImageList iml;
	iml.DeleteImageList();
	iml.Create(16,16,theApp.m_iDfltImageListColorFlags | ILC_MASK,0,1);
	iml.Add(CTempIconLoader("Chat"));
	iml.Add(CTempIconLoader("Message"));
	iml.Add(CTempIconLoader("MessagePending"));
	channelselect.SetImageList(&iml);
	m_imagelist.DeleteImageList();
	m_imagelist.Attach(iml.Detach());
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

	pHeaderCtrl = nickList.GetHeaderCtrl();

	strRes = GetResString(IDS_STATUS);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(1, &hdi);

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

BOOL CIrcWnd::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
#ifdef _DEBUG
	CString strBuff;
	nickList.GetWindowText(strBuff);
	ASSERT( strBuff == NICK_LV_PROFILE_NAME );

	strBuff.Empty();
	serverChannelList.GetWindowText(strBuff);
	ASSERT( strBuff == CHAN_LV_PROFILE_NAME );
#endif

	SetAllIcons();

	m_bConnected = false;
	m_bLoggedIn = false;
	Localize();
	m_pIrcMain = new CIrcMain();
	m_pIrcMain->SetIRCWnd(this);

	nickList.InsertColumn(0,GetResString(IDS_IRC_NICK),LVCFMT_LEFT,113);
	nickList.InsertColumn(1,GetResString(IDS_STATUS),LVCFMT_LEFT,60);

	serverChannelList.InsertColumn(0, GetResString(IDS_IRC_NAME), LVCFMT_LEFT, 203 );
	serverChannelList.InsertColumn(1, GetResString(IDS_UUSERS), LVCFMT_LEFT, 50 );
	serverChannelList.InsertColumn(2, GetResString(IDS_DESCRIPTION), LVCFMT_LEFT, 350 );

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
	AddAnchor(IDC_TAB2,TOP_LEFT, TOP_RIGHT);

	serverChannelList.SortItems(serverChannelList.SortProc, 11);
	serverChannelList.SetSortArrow(1, false);
	nickList.SortItems(nickList.SortProc, 0);
	return true;
}

void CIrcWnd::UpdateFonts(CFont* pFont)
{
	TCITEM tci;
	tci.mask = TCIF_PARAM;
	int i = 0;
	while (channelselect.GetItem(i++, &tci)){
		Channel* ch = (Channel*)tci.lParam;
		if (ch->log.m_hWnd != NULL)
			ch->log.SetFont(pFont);
	}
}

void CIrcWnd::OnSize(UINT nType, int cx, int cy) 
{
	CResizableDialog::OnSize(nType, cx, cy);

	if (m_pCurrentChannel && m_pCurrentChannel->log.m_hWnd){
		CRect rcChannel;
		serverChannelList.GetWindowRect(&rcChannel);
		ScreenToClient(&rcChannel);
		m_pCurrentChannel->log.SetWindowPos(NULL, rcChannel.left, rcChannel.top, rcChannel.Width(), rcChannel.Height(), SWP_NOZORDER);
	}
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
   int nickItem = nickList.GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
   int chanItem= channelselect.GetCurSel(); 
   //int chanLItem= serverChannelList.GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED); 
   switch( wParam ){
	   case IDC_BN_IRCCONNECT: {
		   OnBnClickedBnIrcconnect();
		   return true;
	   }
	   case IDC_CHATSEND: {
		   OnBnClickedChatsend();
		   return true;
	   }
	   case MP_REMOVE:
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
			   send.Format( "PRIVMSG %s :\001SENDLINK|%s|%s\001", nick->nick, EncodeBase16((const unsigned char*)thePrefs.GetUserHash(), 16), GetSendFileString() );
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
	if (nItem == -1)
		nItem = channelselect.GetCurSel();

	if (nItem == -1)
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
	nickList.DeleteAllItems();

	TCITEM item;
	item.mask = TCIF_PARAM;
	int cur_sel = channelselect.GetCurSel();
	if (cur_sel == -1)
		return;
	if (!channelselect.GetItem(cur_sel, &item))
		return;
	Channel* update = (Channel*)item.lParam;

	m_pCurrentChannel = update;
	UpdateNickCount();

	if (m_pCurrentChannel->type == 1){
		titleWindow.SetWindowText(GetResString(IDS_STATUS));
	}
	if (m_pCurrentChannel->type == 2){
		titleWindow.SetWindowText(GetResString(IDS_IRC_CHANNELLIST));
		serverChannelList.ShowWindow(SW_SHOW);
		TCITEM tci;
		tci.mask = TCIF_PARAM;
		int i = 0;
		while (channelselect.GetItem(i++, &tci)){
			Channel* ch2 = (Channel*)tci.lParam;
			if (ch2 != m_pCurrentChannel && ch2->log.m_hWnd != NULL)
				ch2->log.ShowWindow(SW_HIDE);
		}
		return;
	}

	SetActivity( m_pCurrentChannel->name, false );
	CRect rcChannel;
	serverChannelList.GetWindowRect(&rcChannel);
	ScreenToClient(&rcChannel);
	m_pCurrentChannel->log.SetWindowPos(NULL, rcChannel.left, rcChannel.top, rcChannel.Width(), rcChannel.Height(), SWP_NOZORDER);
	m_pCurrentChannel->log.ShowWindow(SW_SHOW);
	TCITEM tci;
	tci.mask = TCIF_PARAM;
	int i = 0;
	while (channelselect.GetItem(i++, &tci)){
		Channel* ch2 = (Channel*)tci.lParam;
		if (ch2 != m_pCurrentChannel && ch2->log.m_hWnd != NULL)
			ch2->log.ShowWindow(SW_HIDE);
	}
	serverChannelList.ShowWindow(SW_HIDE);
	RefreshNickList( update->name );
	SetTitle( update->name, update->title );
	GetDlgItem(IDC_INPUTWINDOW)->SetFocus();
	if( pResult )
		*pResult = 0;
}

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
	if( (thePrefs.GetIRCChanNameFilter() || thePrefs.GetIRCChannelUserFilter()) && thePrefs.GetIRCUseChanFilter()){
		if( usertest < thePrefs.GetIRCChannelUserFilter() )
			return;
		if( dtemp.MakeLower().Find(thePrefs.GetIRCChanNameFilter().MakeLower()) == -1 && ntemp.MakeLower().Find(thePrefs.GetIRCChanNameFilter().MakeLower()) == -1)
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

///////////////////////////////////////////////////////////////////////////////
//  CIrcChannelListCtrl

IMPLEMENT_DYNAMIC(CIrcChannelListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CIrcChannelListCtrl, CMuleListCtrl)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnLvnColumnclick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
END_MESSAGE_MAP()

CIrcChannelListCtrl::CIrcChannelListCtrl()
{
	memset(m_asc_sort, 0, sizeof m_asc_sort);
	m_pParent = NULL;
}

int CIrcChannelListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	ChannelList* item1 = (ChannelList*)lParam1;
	ChannelList* item2 = (ChannelList*)lParam2;
	switch(lParamSort){
		case 0: 
			return item1->name.CompareNoCase(item2->name);
		case 10:
			return item2->name.CompareNoCase(item1->name);
		case 1: 
			return atoi(item1->users) - atoi(item2->users);
		case 11:
			return atoi(item2->users) - atoi(item1->users);
		case 2: 
			return item1->desc.CompareNoCase(item2->desc);
		case 12:
			return item2->desc.CompareNoCase(item1->desc);
		default:
			return 0;
	}
}

void CIrcChannelListCtrl::OnLvnColumnclick(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	m_asc_sort[pNMListView->iSubItem] = !m_asc_sort[pNMListView->iSubItem];
	SetSortArrow(pNMListView->iSubItem, m_asc_sort[pNMListView->iSubItem]);
	SortItems(SortProc, pNMListView->iSubItem + ((m_asc_sort[pNMListView->iSubItem]) ? 0 : 10));
	*pResult = 0;
}

void CIrcChannelListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	int iCurSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	CTitleMenu ChanLMenu;
	ChanLMenu.CreatePopupMenu();
	ChanLMenu.AddMenuTitle(GetResString(IDS_IRC_CHANNEL));
	ChanLMenu.AppendMenu(MF_STRING, Irc_Join, GetResString(IDS_IRC_JOIN));
	if (iCurSel == -1)
		ChanLMenu.EnableMenuItem(Irc_Join, MF_GRAYED);
	GetPopupMenuPos(*this, point);
	ChanLMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, m_pParent); 
	VERIFY( ChanLMenu.DestroyMenu() );
}

void CIrcChannelListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	m_pParent->JoinChannels();
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
			if( !thePrefs.GetIrcIgnoreInfoMessage() )
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
			if((update = m_pCurrentChannel) != NULL){
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
			if( (update = m_pCurrentChannel) != NULL ){
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
				if( !thePrefs.GetIrcIgnoreInfoMessage() )
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
						if( !thePrefs.GetIrcIgnoreInfoMessage() )
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
		ASSERT(0);
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
		channelselect.SetItem( i, &item);
	}
	POSITION pos1, pos2;
	for (pos1 = channelPtrList.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		channelPtrList.GetNext(pos1);
		Channel* cur_channel = (Channel*)channelPtrList.GetAt(pos2);
		if(ChangeNick( cur_channel->name, oldnick, newnick )){
			if( !thePrefs.GetIrcIgnoreInfoMessage() )
				AddInfoMessage( cur_channel->name, GetResString(IDS_IRC_NOWKNOWNAS), oldnick, newnick);
		}
	}
}

/*
void CIrcWnd::SetNick( CString in_nick ){
	thePrefs.SetIRCNick( in_nick.GetBuffer() );
	//Need to also update the preference window for this to work right..
}
*/

///////////////////////////////////////////////////////////////////////////////
//  CIrcNickListCtrl

IMPLEMENT_DYNAMIC(CIrcNickListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CIrcNickListCtrl, CMuleListCtrl)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnLvnColumnclick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
END_MESSAGE_MAP()

CIrcNickListCtrl::CIrcNickListCtrl()
{
	memset(m_asc_sort, 0, sizeof m_asc_sort);
	m_pParent = NULL;
}

int CIrcNickListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
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
		return item1->nick.CompareNoCase(item2->nick);
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
		return item1->nick.CompareNoCase(item2->nick);
	default:
		return 0;
	}
}

void CIrcNickListCtrl::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	m_asc_sort[pNMLV->iSubItem] = !m_asc_sort[pNMLV->iSubItem];
	//SetSortArrow(pNMLV->iSubItem, m_asc_sort[pNMLV->iSubItem]);
	SortItems(SortProc, pNMLV->iSubItem + ((m_asc_sort[pNMLV->iSubItem]) ? 0 : 10));
	*pResult = 0;
}

void CIrcNickListCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	int iCurSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	CTitleMenu NickMenu;
	NickMenu.CreatePopupMenu(); 
	NickMenu.AddMenuTitle(GetResString(IDS_IRC_NICK));
	NickMenu.AppendMenu(MF_STRING, Irc_Priv, GetResString(IDS_IRC_PRIVMESSAGE));
	NickMenu.AppendMenu(MF_STRING, Irc_AddFriend, GetResString(IDS_IRC_ADDTOFRIENDLIST));
	if (!m_pParent->GetSendFileString().IsEmpty())
		NickMenu.AppendMenu(MF_STRING, Irc_SendLink, GetResString(IDS_IRC_SENDLINK) + m_pParent->GetSendFileString());
	else
		NickMenu.AppendMenu(MF_STRING, Irc_SendLink, GetResString(IDS_IRC_SENDLINK) + _T("No Shared File Selected"));
	NickMenu.AppendMenu(MF_STRING, Irc_Slap, GetResString(IDS_IRC_SLAP));
	NickMenu.AppendMenu(MF_STRING, Irc_Owner, _T("Owner"));
	NickMenu.AppendMenu(MF_STRING, Irc_DeOwner, _T("DeOwner"));
	NickMenu.AppendMenu(MF_STRING, Irc_Op, GetResString(IDS_IRC_OP));
	NickMenu.AppendMenu(MF_STRING, Irc_DeOp, GetResString(IDS_IRC_DEOP));
	NickMenu.AppendMenu(MF_STRING, Irc_HalfOp, GetResString(IDS_IRC_HALFOP));
	NickMenu.AppendMenu(MF_STRING, Irc_DeHalfOp, GetResString(IDS_IRC_DEHALFOP));
	NickMenu.AppendMenu(MF_STRING, Irc_Voice, GetResString(IDS_IRC_VOICE));
	NickMenu.AppendMenu(MF_STRING, Irc_DeVoice, GetResString(IDS_IRC_DEVOICE));
	NickMenu.AppendMenu(MF_STRING, Irc_Protect, _T("Protect"));
	NickMenu.AppendMenu(MF_STRING, Irc_DeProtect, _T("DeProtect"));
	NickMenu.AppendMenu(MF_STRING, Irc_Kick, GetResString(IDS_IRC_KICK));

	if (iCurSel == -1)
	{
		NickMenu.EnableMenuItem(Irc_Priv, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_AddFriend, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_SendLink, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_Slap, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_Owner, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_DeOwner, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_Op, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_DeOp, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_HalfOp, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_DeHalfOp, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_Voice, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_DeVoice, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_Protect, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_DeProtect, MF_GRAYED);
		NickMenu.EnableMenuItem(Irc_Kick, MF_GRAYED);
	}

	GetPopupMenuPos(*this, point);
	NickMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, m_pParent); 
	VERIFY( NickMenu.DestroyMenu() );
}

void CIrcNickListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	int nickItem = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (nickItem != -1) {
		Nick* nick = (Nick*)GetItemData(nickItem);
		if (nick)
			m_pParent->AddInfoMessage(nick->nick, GetResString(IDS_IRC_PRIVATECHANSTART));
	}
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
	if( thePrefs.GetIRCAddTimestamp() )
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
	if( m_pCurrentChannel == update_channel )
		return;
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
	if( thePrefs.GetIRCAddTimestamp() )
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
	
	if( m_pCurrentChannel == update_channel )
		return;
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
	if( thePrefs.GetIRCAddTimestamp() )
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
	if( m_pCurrentChannel == update_channel )
		return;
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
		item.iImage = 2; // 'MessagePending'
		channelselect.SetItem( i, &item );
    }
	else{
		item.mask = TCIF_IMAGE;
 		item.iImage = 1; // 'Message'
		channelselect.SetItem( i, &item );
    }
}

void CIrcWnd::OnBnClickedChatsend()
{
	CString send;
	GetDlgItem(IDC_INPUTWINDOW)->GetWindowText(send);
	GetDlgItem(IDC_INPUTWINDOW)->SetWindowText("");
	GetDlgItem(IDC_INPUTWINDOW)->SetFocus();

	if (m_pCurrentChannel->history.GetCount()==thePrefs.GetMaxChatHistoryLines()) m_pCurrentChannel->history.RemoveAt(0);
	m_pCurrentChannel->history.Add(send);
	m_pCurrentChannel->history_pos=m_pCurrentChannel->history.GetCount();

	if( send.IsEmpty() )
		return;

	if( !this->m_bConnected )
		return;
	if( send.Left(4) == "/hop" )
	{
		if( m_pCurrentChannel->name.Left(1) == "#" )
		{
			CString channel = m_pCurrentChannel->name;
			m_pIrcMain->SendString( "PART " + channel );
			m_pIrcMain->SendString( "JOIN " + channel );
			return;
		}
		return;
	}
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
	toadd->history_pos = 0;
	if (type != 2)
	{
		CRect rcChannel;
		serverChannelList.GetWindowRect(&rcChannel);
		toadd->log.Create(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | ES_MULTILINE | ES_READONLY, rcChannel, this, (UINT)-1);
		toadd->log.ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_FRAMECHANGED);
		toadd->log.SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
		toadd->log.SetEventMask(toadd->log.GetEventMask() | ENM_LINK);
		toadd->log.SetFont(&theApp.emuledlg->m_fontHyperText);
		toadd->log.SetTitle(name);
	}
	channelPtrList.AddTail(toadd);
	
	TCITEM newitem;
	newitem.mask = TCIF_PARAM|TCIF_TEXT|TCIF_IMAGE;
	newitem.lParam = (LPARAM)toadd;
	newitem.pszText = name.GetBuffer();
	newitem.iImage = 1; // 'Message'
	uint32 pos = channelselect.GetItemCount();
	channelselect.InsertItem(pos,&newitem);
	if(type == 4)
	{
		channelselect.SetCurSel(pos);
		channelselect.SetCurFocus(pos);
		OnTcnSelchangeTab2( NULL, NULL );
	}
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
	if( this->m_bConnected )
		m_pIrcMain->SendString( send );
}

void CIrcWnd::ScrollHistory(bool down) {
	CString buffer;

	if ( (m_pCurrentChannel->history_pos==0 && !down) || (m_pCurrentChannel->history_pos==m_pCurrentChannel->history.GetCount() && down))
		return;
	
	if (down)
		++m_pCurrentChannel->history_pos;
	else
		--m_pCurrentChannel->history_pos;

	buffer= (m_pCurrentChannel->history_pos==m_pCurrentChannel->history.GetCount()) ? "" : m_pCurrentChannel->history.GetAt(m_pCurrentChannel->history_pos);

	GetDlgItem(IDC_INPUTWINDOW)->SetWindowText(buffer);
	inputWindow.SetSel(buffer.GetLength(),buffer.GetLength());
}

void CIrcWnd::OnContextMenu(CWnd* pWnd, CPoint point)
{ 
	int iCurTab = GetTabUnderMouse(point); //channelselect.GetCurSel();

	CTitleMenu ChatMenu;
	ChatMenu.CreatePopupMenu();
	ChatMenu.AddMenuTitle(GetResString(IDS_IRC));
	ChatMenu.AppendMenu(MF_STRING, MP_REMOVE, GetResString(IDS_FD_CLOSE));
	if (iCurTab < 2) // no 'Close' for status log and channel list
		ChatMenu.EnableMenuItem(MP_REMOVE, MF_GRAYED);
	ChatMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	VERIFY( ChatMenu.DestroyMenu() );
}

int CIrcWnd::GetTabUnderMouse(CPoint point) {
		TCHITTESTINFO hitinfo;
		CRect rect;
		channelselect.GetWindowRect(&rect);
		point.Offset(0-rect.left,0-rect.top);
		hitinfo.pt = point;

		if( channelselect.GetItemRect( 0, &rect ) )
			if (hitinfo.pt.y< rect.top+30 && hitinfo.pt.y >rect.top-30)
				hitinfo.pt.y = rect.top;

		// Find the destination tab...
		unsigned int nTab = channelselect.HitTest( &hitinfo );
		if( hitinfo.flags != TCHT_NOWHERE )
			return nTab;
		else return -1;
}