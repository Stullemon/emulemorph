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

// ChatSelector.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "ChatSelector.h"
#include "packets.h"
#include "otherfunctions.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CChatItem::CChatItem(){
	log = 0;
	messagepending = 0;
	notify = false;
	history_pos=0;
}

// CChatSelector

IMPLEMENT_DYNAMIC(CChatSelector, CClosableTabCtrl)
CChatSelector::CChatSelector()
{
	m_pCloseBtn = NULL;
	m_pMessageBox = NULL;
	m_pSendBtn = NULL;
	lastemptyicon=false;
	blinkstate = false;
	m_Timer = 0;
}

CChatSelector::~CChatSelector(){
}


BEGIN_MESSAGE_MAP(CChatSelector, CClosableTabCtrl)
	ON_WM_TIMER()
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnTcnSelchangeChatsel)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CCLOSE, OnBnClickedCclose)
	ON_BN_CLICKED(IDC_CSEND, OnBnClickedCsend)
	ON_WM_DESTROY()
END_MESSAGE_MAP()



// CChatSelector message handlers
void CChatSelector::Init()
{
	CRect rect;
	GetClientRect(&rect);
	AdjustRect(FALSE, rect);
	
	CRect rClose;
	m_pCloseBtn = GetParent()->GetDlgItem(IDC_CCLOSE);
	m_pCloseBtn->SetParent(this);
	m_pCloseBtn->GetWindowRect(&rClose);
	m_pCloseBtn->SetWindowPos(NULL, rect.right-7-rClose.Width(), rect.bottom-7-rClose.Height(),
							  rClose.Width(), rClose.Height(), SWP_NOZORDER);
	CRect rSend;
	m_pSendBtn = GetParent()->GetDlgItem(IDC_CSEND);
	m_pSendBtn->SetParent(this);
	m_pSendBtn->GetWindowRect(&rSend);
	m_pSendBtn->SetWindowPos(NULL, rect.right-7-rClose.Width()-7-rSend.Width(), rect.bottom-7-rSend.Height(),
							 rSend.Width(), rSend.Height(), SWP_NOZORDER);
	
	CRect rMessage;
	m_pMessageBox = GetParent()->GetDlgItem(IDC_CMESSAGE);
	m_pMessageBox->SetParent(this);
	m_pMessageBox->GetWindowRect(&rMessage);
	m_pMessageBox->SetWindowPos(NULL, rect.left+7, rect.bottom-9-rMessage.Height(), 
								rect.right-7-rClose.Width()-7-rSend.Width()-21, 
								rMessage.Height(), SWP_NOZORDER);

	int iTop = rClose.Height() > rSend.Height() ? rClose.Height() : rSend.Height();
	if(iTop < rMessage.Height())
		iTop = rMessage.Height();
	
	CRect rChatOut = rect;
	rChatOut.top += 7;
	rChatOut.left += 7;
	rChatOut.right -= 7;
	rChatOut.bottom -= iTop + 17;

	ModifyStyle(0, WS_CLIPCHILDREN);
	chatout.CreateEx(0,0,"ChatWnd",WS_VISIBLE | WS_CHILD | WS_BORDER | HTC_WORDWRAP |HTC_AUTO_SCROLL_BARS | HTC_UNDERLINE_HOVER,rChatOut,this,0);
	UpdateFonts(&theApp.emuledlg->m_fontHyperText);
	chatout.AppendHyperLink(CString("eMule "),0,CString("http://www.emule-project.net"),0,0);
	chatout.AppendText(CString(" Version ")+ theApp.m_strCurVersionLong+ CString(" - ")+GetResString(IDS_CHAT_WELCOME));

	imagelist.Create(16,16,theApp.m_iDfltImageListColorFlags | ILC_MASK,0,1);
	imagelist.Add(CTempIconLoader("Chat"));
	imagelist.Add(CTempIconLoader("Message"));
	imagelist.Add(CTempIconLoader("MessagePending"));
	SetImageList(&imagelist);

	VERIFY( (m_Timer = SetTimer(20,1500,0)) );
}

void CChatSelector::UpdateFonts(CFont* pFont)
{
	if (pFont->m_hObject)
		chatout.SetFont(pFont);
}

CChatItem* CChatSelector::StartSession(CUpDownClient* client, bool show){
	m_pMessageBox->SetFocus();
	if (GetTabByClient(client) != 0xFFFF){
		if (show){
			SetCurSel(GetTabByClient(client));
			chatout.SetHyperText(GetItemByClient(client)->log);
		}
		return 0;
	}

	CChatItem* chatitem = new CChatItem();
	chatitem->client = client;
	chatitem->log = new CPreparedHyperText();

	CTime theTime = CTime::GetCurrentTime();
	CString sessions = GetResString(IDS_CHAT_START)+CString(client->GetUserName()) + " - "+theTime.Format("%c")+ "\n";
	chatitem->log->AppendKeyWord(sessions,RGB(255,0,0));
	client->SetChatState(MS_CHATTING);

	CString name;
	if (client->GetUserName()!=NULL) name=client->GetUserName(); else name.Format("(%s)",GetResString(IDS_UNKNOWN));

	TCITEM newitem;
	newitem.mask = TCIF_PARAM|TCIF_TEXT|TCIF_IMAGE;
	newitem.lParam = (LPARAM)chatitem;
	newitem.pszText = name.GetBuffer();
	newitem.cchTextMax = name.GetLength()+1;
	newitem.iImage = 0;
	uint16 itemnr = InsertItem(GetItemCount(),&newitem);
	if (show){
		SetCurSel(itemnr);
		chatout.SetHyperText(chatitem->log);
	}
	return chatitem;
}

uint16 CChatSelector::GetTabByClient(CUpDownClient* client){
	for (int i = 0; i < GetItemCount();i++){
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM;
		GetItem(i,&cur_item);
		if (((CChatItem*)cur_item.lParam)->client == client)
			return i;
	}
	return -1;
}

CChatItem* CChatSelector::GetItemByClient(CUpDownClient* client){
	for (int i = 0; i < GetItemCount();i++){
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM;
		GetItem(i,&cur_item);
		if (((CChatItem*)cur_item.lParam)->client == client)
			return (CChatItem*)cur_item.lParam;
	}
	return 0;
}

void CChatSelector::ProcessMessage(CUpDownClient* sender, char* message){
	sender->m_cMessagesReceived++;

	CString Cmessage=CString(message).MakeLower();
	CString resToken;
	int curPos=0;
	resToken= theApp.glob_prefs->GetMessageFilter().Tokenize("|",curPos);
	while (resToken != "")
	{
		if (Cmessage.Find(resToken.MakeLower())>-1) {return;}
		resToken= theApp.glob_prefs->GetMessageFilter().Tokenize("|",curPos);
	};
	// continue
	CChatItem* ci = GetItemByClient(sender);

	// advanced spamfilter check
	if (IsSpam(Cmessage,sender)){
		if (!sender->m_bIsSpammer)
			AddDebugLogLine(false, "'%s' has been marked as spammer", sender->GetUserName());
		sender->m_bIsSpammer = true;
		if (ci != NULL)
			this->EndSession(sender);
		return;
	}

	bool isNewChatWindow = false;
	if (!ci) {
		if (GetItemCount()>=theApp.glob_prefs->GetMsgSessionsMax()) return;
		ci = StartSession(sender,false);
		isNewChatWindow = true; 
	}
	if (theApp.glob_prefs->GetIRCAddTimestamp())
		AddTimeStamp(ci);
	ci->log->AppendKeyWord(CString(sender->GetUserName()),RGB(50,200,250));
	ci->log->AppendText(CString(": "));
	ci->log->AppendText(CString(message)+ CString("\n"));
	if (GetCurSel() == GetTabByClient(sender) && GetParent()->IsWindowVisible())
		chatout.SetHyperText(ci->log);
	else { 
		ci->notify = true;
		//START - enkeyDEV(kei-kun) -TaskbarNotifier- 	
        if (isNewChatWindow || theApp.glob_prefs->GetNotifierPopsEveryChatMsg())  //<<-31/10/2002 (kei-kun)
                theApp.emuledlg->ShowNotifier(GetResString(IDS_TBN_NEWCHATMSG)+CString(" ")+CString(sender->GetUserName()) + CString(":'") + CString(message)+ CString("'\n"), TBN_CHAT);

		isNewChatWindow = false;
		//END - enkeyDEV(kei-kun) -TaskbarNotifier-
	}
}

bool CChatSelector::SendMessage(char* message){

	CChatItem* ci = GetCurrentChatItem();
	if (ci==NULL) return false;

	if (ci->history.GetCount()==theApp.glob_prefs->GetMaxChatHistoryLines()) ci->history.RemoveAt(0);
	ci->history.Add(CString(message));
	ci->history_pos=ci->history.GetCount();

	// advance spamfilter stuff
	ci->client->m_cMessagesSend++;
	ci->client->m_bIsSpammer = false;
	// *
	if (ci->client->GetChatState() == MS_CONNECTING){
		return false;
	}
	if (theApp.glob_prefs->GetIRCAddTimestamp())
		AddTimeStamp(ci);
	if (ci->client->socket && ci->client->socket->IsConnected()){
		uint16 mlen = (uint16)strlen(message);
		Packet* packet = new Packet(OP_MESSAGE,mlen+2);
		memcpy(packet->pBuffer,&mlen,2);
		memcpy(packet->pBuffer+2,message,mlen);
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		ci->client->socket->SendPacket(packet,true,true);
		ci->log->AppendKeyWord(CString(theApp.glob_prefs->GetUserNick()),RGB(1,180,20));
		ci->log->AppendText(CString(": "));
		ci->log->AppendText(CString(message)+CString("\n"));
		chatout.UpdateSize(true);
	}
	else{
		ci->log->AppendKeyWord(CString("*** ")+GetResString(IDS_CONNECTING),RGB(255,0,0));
		ci->messagepending = nstrdup(message);
		ci->client->SetChatState(MS_CONNECTING);
		ci->client->TryToConnect();
	}
	if (chatout.GetHyperText() == ci->log)
		chatout.UpdateSize(true);
	return true;
}

void CChatSelector::ConnectingResult(CUpDownClient* sender,bool success){
	CChatItem* ci = GetItemByClient(sender);
	if (!ci)
		return;
	ci->client->SetChatState(MS_CHATTING);
	if (!success){
		if (ci->messagepending){
			ci->log->AppendKeyWord(CString(" ")+GetResString(IDS_FAILED) +CString("\n"),RGB(255,0,0));
			delete[] ci->messagepending;
		}
		else
			ci->log->AppendKeyWord(GetResString(IDS_CHATDISCONNECTED) +CString("\n"),RGB(255,0,0));
		ci->messagepending = 0;
	}
	else{
		ci->log->AppendKeyWord(CString(" ok\n"),RGB(255,0,0));
		uint16 mlen = (uint16)strlen(ci->messagepending);
		Packet* packet = new Packet(OP_MESSAGE,mlen+2);
		memcpy(packet->pBuffer,&mlen,2);
		memcpy(packet->pBuffer+2,ci->messagepending,mlen);
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		ci->client->socket->SendPacket(packet,true,true);
		if (theApp.glob_prefs->GetIRCAddTimestamp())
			AddTimeStamp(ci);
		ci->log->AppendKeyWord(CString(theApp.glob_prefs->GetUserNick()),RGB(1,180,20));
		ci->log->AppendText(CString(": "));
		ci->log->AppendText(CString(ci->messagepending)+CString("\n"));
		delete[] ci->messagepending;
		ci->messagepending = 0;
	}
	if (chatout.GetHyperText() == ci->log)
		chatout.UpdateSize(true);
}

void CChatSelector::DeleteAllItems(){
	for (int i = 0; i < GetItemCount();i++){
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM;
		GetItem(i,&cur_item);
		delete (CChatItem*)cur_item.lParam;
	}
}

void CChatSelector::OnTimer(UINT_PTR nIDEvent){
	blinkstate = !blinkstate;
	bool globalnotify = false;
	for (int i = 0; i < GetItemCount();i++){
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM;
		GetItem(i,&cur_item);
		cur_item.mask = TCIF_IMAGE;
		if (((CChatItem*)cur_item.lParam)->notify){
			cur_item.iImage = (blinkstate)? 1:2;
			SetItem(i,&cur_item);
			globalnotify = true;
		}
		else if (cur_item.iImage != 0){
			cur_item.iImage = 0;
			SetItem(i,&cur_item);
		}
	}
	if (globalnotify) {
		theApp.emuledlg->ShowMessageState(((blinkstate)? 1:2));
		lastemptyicon=false;
	}
	else if (!lastemptyicon) {theApp.emuledlg->ShowMessageState(0); lastemptyicon=true;}
}

CChatItem* CChatSelector::GetCurrentChatItem() {
	if (GetCurSel() == (-1))
		return NULL;
	TCITEM cur_item;
	cur_item.mask = TCIF_PARAM;
	GetItem(GetCurSel(),&cur_item);
	CChatItem* ci = (CChatItem*)cur_item.lParam;
	return ci;
}

void CChatSelector::ShowChat(){
	if (GetCurSel() == (-1))
		return;
	/*
	TCITEM cur_item;
	cur_item.mask = TCIF_PARAM;
	GetItem(GetCurSel(),&cur_item);*/

	CChatItem* ci = GetCurrentChatItem();
	chatout.SetHyperText(ci->log);
	ci->notify = false;
}


void CChatSelector::OnTcnSelchangeChatsel(NMHDR *pNMHDR, LRESULT *pResult){
	ShowChat();
	*pResult = 0;
}

INT	CChatSelector::InsertItem(int nItem,TCITEM* pTabCtrlItem){
	if (!GetItemCount()){
		WINDOWPLACEMENT wp;
		chatout.GetWindowPlacement(&wp);
		wp.rcNormalPosition.top +=20;
		chatout.SetWindowPlacement(&wp);
	}
	int result = CClosableTabCtrl::InsertItem(nItem,pTabCtrlItem);
	RedrawWindow();
	return result;
}

BOOL CChatSelector::DeleteItem(int nItem){
	CClosableTabCtrl::DeleteItem(nItem);
	if (!GetItemCount()){
		WINDOWPLACEMENT wp;
		chatout.GetWindowPlacement(&wp);
		wp.rcNormalPosition.top -=20;
		chatout.SetWindowPlacement(&wp);
	}
	RedrawWindow();
	return true;
}

void CChatSelector::EndSession(CUpDownClient* client){
	sint16 usedtab;
	if (client){
		usedtab = GetTabByClient(client);
	}
	else{
		usedtab = GetCurSel();
	}
	if (usedtab == (-1))
		return;
	TCITEM item;
	item.mask = TCIF_PARAM;
	GetItem(usedtab,&item);
	CChatItem* ci = (CChatItem*)item.lParam;
	ci->client->SetChatState(MS_NONE);
	
	DeleteItem(usedtab);
	if (chatout.GetHyperText() == ci->log)
		chatout.SetHyperText(0);
	delete ci;
}
void CChatSelector::OnSize(UINT nType, int cx, int cy)
{
	CClosableTabCtrl::OnSize(nType, cx, cy);	

	CRect rect;
	GetClientRect(&rect);
	AdjustRect(FALSE, rect);
		
	CRect rClose;
	m_pCloseBtn->GetWindowRect(&rClose);
	m_pCloseBtn->SetWindowPos(NULL, rect.right-7-rClose.Width(), rect.bottom-7-rClose.Height(),
								rClose.Width(), rClose.Height(), SWP_NOZORDER);
	CRect rSend;
	m_pSendBtn->GetWindowRect(&rSend);
	m_pSendBtn->SetWindowPos(NULL, rect.right-7-rClose.Width()-7-rSend.Width(), rect.bottom-7-rSend.Height(),
								rSend.Width(), rSend.Height(), SWP_NOZORDER);
	
	CRect rMessage;
	m_pMessageBox->GetWindowRect(&rMessage);
	m_pMessageBox->SetWindowPos(NULL, rect.left+7, rect.bottom-9-rMessage.Height(), 
									rect.right-7-rClose.Width()-7-rSend.Width()-21, 
										rMessage.Height(), SWP_NOZORDER);

	int iTop = rClose.Height() > rSend.Height() ? rClose.Height() : rSend.Height();
	if(iTop < rMessage.Height())
		iTop = rMessage.Height();
	
	chatout.SetWindowPos(NULL, rect.left+7, rect.top+7, rect.right-18, rect.Height()-7-iTop-14, SWP_NOZORDER);
}

void CChatSelector::OnBnClickedCclose(){
	EndSession();
}

void CChatSelector::OnBnClickedCsend()
{
	uint16 len = m_pMessageBox->GetWindowTextLength()+2;
	char* messagetosend = new char[len+1];
	m_pMessageBox->GetWindowText(messagetosend,len);

	if(SendMessage(messagetosend))
		m_pMessageBox->SetWindowText("");
	delete[] messagetosend;
}

BOOL CChatSelector::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN) {
		
		if (pMsg->wParam == VK_RETURN){
		if (pMsg->hwnd == m_pMessageBox->m_hWnd)
			OnBnClickedCsend();
	}

		if ((pMsg->hwnd == GetDlgItem(IDC_CMESSAGE)->m_hWnd) && 
		   (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN)) {
			theApp.emuledlg->chatwnd.ScrollHistory(pMsg->wParam == VK_DOWN);
			return TRUE;
		}
	}
	return CClosableTabCtrl::PreTranslateMessage(pMsg);
}

void CChatSelector::Localize(void)
{
	if(m_hWnd)
	{
		if(m_pSendBtn)
			m_pSendBtn->SetWindowText(GetResString(IDS_CW_SEND));
		else
			GetParent()->GetDlgItem(IDC_CSEND)->SetWindowText(GetResString(IDS_CW_SEND));
		if(m_pCloseBtn)
			m_pCloseBtn->SetWindowText(GetResString(IDS_CW_CLOSE));
		else
			GetParent()->GetDlgItem(IDC_CCLOSE)->SetWindowText(GetResString(IDS_CW_CLOSE));
	}
}

bool CChatSelector::IsSpam(CString strMessage, CUpDownClient* client){
	// first step, spam dectection will be further improved in future versions
	if ( !theApp.glob_prefs->IsAdvSpamfilterEnabled() || client->IsFriend() ) // friends are never spammer... (but what if two spammers are friends :P )
		return false;

	if (client->m_bIsSpammer)
		return true;

	// first fixed criteria: If a client  sends me an URL in his first message before I response to him
	// there is a 99,9% chance that it is some poor guy advising his leech mod, or selling you .. well you know :P
	if (client->m_cMessagesSend == 0){
		int curPos=0;
		bool bFound = false;
		CString resToken = CString(URLINDICATOR).Tokenize("|",curPos);
		while (resToken != ""){
			if (strMessage.Find(resToken) > (-1) )
				return true;
			resToken= CString(URLINDICATOR).Tokenize("|",curPos);
		}
	}
	// second fixed criteria: he sent me 5  or more messages and I didn't answered him once
	if (client->m_cMessagesReceived > 4 && client->m_cMessagesSend == 0)
		return true;

	//MORPH - Added by IceCream, third fixed criteria: leechers who try to afraid other morph/lovelave/blackrat users (NOS, Darkmule ...)
	if (theApp.glob_prefs->GetEnableAntiLeecher())
		if ((client->IsLeecher()) /*client is already or was a leecher?*/ || (client->TestLeecher()) /*client is now a leecher?*/ || (strMessage.Find("leech") > (-1)) /*leecher client try to afraid users*/|| (strMessage.Find("ban") > (-1))/*leecher client try to afraid users*/)
			return true;

	// to be continued

	return false;
}
void CChatSelector::AddTimeStamp(CChatItem* ci)
{
	ci->log->AppendText(CTime::GetCurrentTime().Format(_T("[%X] ")));
}

void CChatSelector::OnDestroy()
{
	if (m_Timer)
		KillTimer(m_Timer);
	CClosableTabCtrl::OnDestroy();
}
