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
#include "ChatSelector.h"
#include "packets.h"
#include "HTRichEditCtrl.h"
#include "emuledlg.h"
#include "UploadQueue.h"
#include "OtherFunctions.h"
#include "UpDownClient.h"
#include "Preferences.h"
#include "TaskbarNotifier.h"
#include "ListenSocket.h"
#include "ChatWnd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define URLINDICATOR	_T("http:|www.|.de |.net |.com |.org |.to |.tk |.cc |.fr |ftp:")


///////////////////////////////////////////////////////////////////////////////
// CChatItem

CChatItem::CChatItem()
{
	client = NULL;
	log = NULL;
	messagepending = NULL;
	notify = false;
	history_pos = 0;
}

CChatItem::~CChatItem()
{
	delete log;
	delete[] messagepending;
}

///////////////////////////////////////////////////////////////////////////////
// CChatSelector

IMPLEMENT_DYNAMIC(CChatSelector, CClosableTabCtrl)

BEGIN_MESSAGE_MAP(CChatSelector, CClosableTabCtrl)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnTcnSelchangeChatsel)
	ON_BN_CLICKED(IDC_CCLOSE, OnBnClickedCclose)
	ON_BN_CLICKED(IDC_CSEND, OnBnClickedCsend)
END_MESSAGE_MAP()

CChatSelector::CChatSelector()
{
	m_pCloseBtn = NULL;
	m_pMessageBox = NULL;
	m_pSendBtn = NULL;
	m_lastemptyicon = false;
	m_blinkstate = false;
	m_Timer = 0;
}

CChatSelector::~CChatSelector()
{
}

void CChatSelector::Init()
{
	m_pCloseBtn = GetParent()->GetDlgItem(IDC_CCLOSE);
	m_pCloseBtn->SetParent(this);
	m_pSendBtn = GetParent()->GetDlgItem(IDC_CSEND);
	m_pSendBtn->SetParent(this);
	m_pMessageBox = GetParent()->GetDlgItem(IDC_CMESSAGE);
	m_pMessageBox->SetParent(this);

	ModifyStyle(0, WS_CLIPCHILDREN);

	// as long we use the 'CCloseableTabCtrl' we can't use icons..
//	m_imagelist.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
//	m_imagelist.Add(CTempIconLoader(_T("Chat")));
//	m_imagelist.Add(CTempIconLoader(_T("Message")));
//	m_imagelist.Add(CTempIconLoader(_T("MessagePending")));
//	SetImageList(&m_imagelist);

	VERIFY( (m_Timer = SetTimer(20, 1500, 0)) != NULL );
}

void CChatSelector::UpdateFonts(CFont* pFont)
{
	TCITEM item;
	item.mask = TCIF_PARAM;
	int i = 0;
	while (GetItem(i++, &item)){
		CChatItem* ci = (CChatItem*)item.lParam;
		ci->log->SetFont(pFont);
	}
}

CChatItem* CChatSelector::StartSession(CUpDownClient* client, bool show)
{
	m_pMessageBox->SetFocus();
	if (GetTabByClient(client) != 0xFFFF){
		if (show){
			SetCurSel(GetTabByClient(client));
			ShowChat();
		}
		return NULL;
	}

	CChatItem* chatitem = new CChatItem();
	chatitem->client = client;
	chatitem->log = new CHTRichEditCtrl;

	CRect rcChat;
	GetChatSize(rcChat);
	if (GetItemCount() == 0)
		rcChat.top += 20;
	chatitem->log->Create(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | ES_MULTILINE | ES_READONLY, rcChat, this, (UINT)-1);
	chatitem->log->ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_FRAMECHANGED);
	chatitem->log->SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
	chatitem->log->SetEventMask(chatitem->log->GetEventMask() | ENM_LINK);
	chatitem->log->SetFont(&theApp.emuledlg->m_fontHyperText);

	CTime theTime = CTime::GetCurrentTime();
	CString sessions = GetResString(IDS_CHAT_START) + client->GetUserName() + CString(_T(" - ")) + theTime.Format(_T("%c"))+ _T("\n");
	chatitem->log->AppendKeyWord(sessions, RGB(255,0,0));
	client->SetChatState(MS_CHATTING);

	CString name;
	if (client->GetUserName() != NULL)
		name = client->GetUserName();
	else
		name.Format(_T("(%s)"), GetResString(IDS_UNKNOWN));
	chatitem->log->SetTitle(name);

	// CCloseableTabCtrl doesn't draw the text really nice.. add some "margins"
	CString strLabel = _T("  ") + name + _T("  ");
	TCITEM newitem;
	newitem.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
	newitem.lParam = (LPARAM)chatitem;
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)strLabel);
	newitem.iImage = 0;
	int iItemNr = InsertItem(GetItemCount(), &newitem);
	if (show || IsWindowVisible()){
		SetCurSel(iItemNr);
		ShowChat();
	}
	return chatitem;
}

uint16 CChatSelector::GetTabByClient(CUpDownClient* client)
{
	for (int i = 0; i < GetItemCount(); i++){
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM;
		if (GetItem(i, &cur_item) && ((CChatItem*)cur_item.lParam)->client == client)
			return i;
	}
	return (uint16)-1;
}

CChatItem* CChatSelector::GetItemByClient(CUpDownClient* client)
{
	for (int i = 0; i < GetItemCount(); i++){
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM;
		if (GetItem(i, &cur_item) && ((CChatItem*)cur_item.lParam)->client == client)
			return (CChatItem*)cur_item.lParam;
	}
	return NULL;
}

void CChatSelector::ProcessMessage(CUpDownClient* sender, char* message)
{
	sender->m_cMessagesReceived++;

	CString strMessage = CString(message).MakeLower();
	CString resToken;
	int curPos = 0;
	resToken = theApp.glob_prefs->GetMessageFilter().Tokenize(_T("|"), curPos);
	while (resToken != _T(""))
	{
		if (strMessage.Find(resToken.MakeLower()) > -1)
			return;
		resToken = theApp.glob_prefs->GetMessageFilter().Tokenize(_T("|"), curPos);
	}

	CChatItem* ci = GetItemByClient(sender);

	// advanced spamfilter check
	if (IsSpam(strMessage, sender))
	{
		if (!sender->m_bIsSpammer)
			theApp.emuledlg->AddDebugLogLine(false, _T("'%s' has been marked as spammer"), sender->GetUserName());
		sender->m_bIsSpammer = true;
		if (ci)
			EndSession(sender);
		return;
	}

	bool isNewChatWindow = false;
	if (!ci)
	{
		if (GetItemCount() >= theApp.glob_prefs->GetMsgSessionsMax())
			return;
		ci = StartSession(sender, false);
		isNewChatWindow = true; 
	}
	if (theApp.glob_prefs->GetIRCAddTimestamp())
		AddTimeStamp(ci);
	ci->log->AppendKeyWord(sender->GetUserName(), RGB(50,200,250));
	ci->log->AppendText(_T(": "));
	ci->log->AppendText(CString(message) + _T("\n"));
	if (GetCurSel() == GetTabByClient(sender) && GetParent()->IsWindowVisible()){
		; // chat window is already visible
	}
	else{
		ci->notify = true;
        if (isNewChatWindow || theApp.glob_prefs->GetNotifierPopsEveryChatMsg())
			theApp.emuledlg->ShowNotifier(GetResString(IDS_TBN_NEWCHATMSG) + _T(" ") + CString(sender->GetUserName()) + _T(":'") + CString(message) + _T("'\n"), TBN_CHAT);
		isNewChatWindow = false;
	}
}

bool CChatSelector::SendMessage(LPCTSTR message)
{
	CChatItem* ci = GetCurrentChatItem();
	if (!ci)
		return false;

	if (ci->history.GetCount() == theApp.glob_prefs->GetMaxChatHistoryLines())
		ci->history.RemoveAt(0);
	ci->history.Add(CString(message));
	ci->history_pos = ci->history.GetCount();

	// advance spamfilter stuff
	ci->client->m_cMessagesSend++;
	ci->client->m_bIsSpammer = false;
	if (ci->client->GetChatState() == MS_CONNECTING)
		return false;

	if (theApp.glob_prefs->GetIRCAddTimestamp())
		AddTimeStamp(ci);
	if (ci->client->socket && ci->client->socket->IsConnected())
	{
		uint16 mlen = (uint16)strlen(message);
		Packet* packet = new Packet(OP_MESSAGE, mlen+2);
		memcpy(packet->pBuffer, &mlen, 2);
		memcpy(packet->pBuffer + 2, message, mlen);
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		ci->client->socket->SendPacket(packet, true, true);

		ci->log->AppendKeyWord(theApp.glob_prefs->GetUserNick(), RGB(1,180,20));
		ci->log->AppendText(_T(": "));
		ci->log->AppendText(CString(message) + _T("\n"));
	}
	else
	{
		ci->log->AppendKeyWord(_T("*** ") + GetResString(IDS_CONNECTING), RGB(255,0,0));
		ci->messagepending = nstrdup(message);
		ci->client->SetChatState(MS_CONNECTING);
		ci->client->TryToConnect();
	}
	return true;
}

void CChatSelector::ConnectingResult(CUpDownClient* sender, bool success)
{
	CChatItem* ci = GetItemByClient(sender);
	if (!ci)
		return;

	ci->client->SetChatState(MS_CHATTING);
	if (!success){
		if (ci->messagepending){
			ci->log->AppendKeyWord(_T(" ") + GetResString(IDS_FAILED) + _T("\n"), RGB(255,0,0));
			delete[] ci->messagepending;
			ci->messagepending = NULL;
		}
		else{
			if (theApp.glob_prefs->GetIRCAddTimestamp())
				AddTimeStamp(ci);
			ci->log->AppendKeyWord(GetResString(IDS_CHATDISCONNECTED) + _T("\n"), RGB(255,0,0));
		}
	}
	else if (ci->messagepending){
		ci->log->AppendKeyWord(_T(" ok\n"), RGB(255,0,0));
		
		uint16 mlen = (uint16)strlen(ci->messagepending);
		Packet* packet = new Packet(OP_MESSAGE, mlen+2);
		memcpy(packet->pBuffer, &mlen, 2);
		memcpy(packet->pBuffer + 2, ci->messagepending, mlen);
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		ci->client->socket->SendPacket(packet, true, true);

		if (theApp.glob_prefs->GetIRCAddTimestamp())
			AddTimeStamp(ci);
		ci->log->AppendKeyWord(theApp.glob_prefs->GetUserNick(), RGB(1,180,20));
		ci->log->AppendText(_T(": "));
		ci->log->AppendText(CString(ci->messagepending) + _T("\n"));
		
		delete[] ci->messagepending;
		ci->messagepending = NULL;
	}
	else{
		if (theApp.glob_prefs->GetIRCAddTimestamp())
			AddTimeStamp(ci);
		ci->log->AppendKeyWord(_T("*** Connected\n"), RGB(255,0,0));
	}
}

void CChatSelector::DeleteAllItems()
{
	for (int i = 0; i < GetItemCount(); i++){
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM;
		if (GetItem(i, &cur_item))
			delete (CChatItem*)cur_item.lParam;
	}
}

void CChatSelector::OnTimer(UINT_PTR nIDEvent)
{
	m_blinkstate = !m_blinkstate;
	bool globalnotify = false;
	for (int i = 0; i < GetItemCount();i++)
	{
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM;
		if (!GetItem(i, &cur_item))
			break;

		cur_item.mask = TCIF_IMAGE;
		if (((CChatItem*)cur_item.lParam)->notify){
			cur_item.iImage = (m_blinkstate) ? 1 : 2;
			SetItem(i, &cur_item);
			globalnotify = true;
		}
		else if (cur_item.iImage != 0){
			cur_item.iImage = 0;
			SetItem(i, &cur_item);
		}
	}

	if (globalnotify) {
		theApp.emuledlg->ShowMessageState(m_blinkstate ? 1 : 2);
		m_lastemptyicon = false;
	}
	else if (!m_lastemptyicon) {
		theApp.emuledlg->ShowMessageState(0);
		m_lastemptyicon = true;
	}
}

CChatItem* CChatSelector::GetCurrentChatItem()
{
	int iCurSel = GetCurSel();
	if (iCurSel == -1)
		return NULL;

	TCITEM cur_item;
	cur_item.mask = TCIF_PARAM;
	if (!GetItem(iCurSel, &cur_item))
		return NULL;

	return (CChatItem*)cur_item.lParam;
}

void CChatSelector::ShowChat()
{
	CChatItem* ci = GetCurrentChatItem();
	if (!ci)
		return;

	// show current chat window
	ci->log->ShowWindow(SW_SHOW);
	m_pMessageBox->SetFocus();

	// hide all other chat windows
	TCITEM item;
	item.mask = TCIF_PARAM;
	int i = 0;
	while (GetItem(i++, &item)){
		CChatItem* ci2 = (CChatItem*)item.lParam;
		if (ci2 != ci)
			ci2->log->ShowWindow(SW_HIDE);
	}

	ci->notify = false;
}

void CChatSelector::OnTcnSelchangeChatsel(NMHDR *pNMHDR, LRESULT *pResult)
{
	ShowChat();
	*pResult = 0;
}

int CChatSelector::InsertItem(int nItem, TCITEM* pTabCtrlItem)
{
	int iResult = CClosableTabCtrl::InsertItem(nItem, pTabCtrlItem);
	RedrawWindow();
	return iResult;
}

BOOL CChatSelector::DeleteItem(int nItem)
{
	CClosableTabCtrl::DeleteItem(nItem);
	RedrawWindow();
	return TRUE;
}

void CChatSelector::EndSession(CUpDownClient* client)
{
	int iCurSel;
	if (client)
		iCurSel = GetTabByClient(client);
	else
		iCurSel = GetCurSel();
	if (iCurSel == -1)
		return;
	
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (!GetItem(iCurSel, &item) || item.lParam == 0)
		return;
	CChatItem* ci = (CChatItem*)item.lParam;
	ci->client->SetChatState(MS_NONE);

	DeleteItem(iCurSel);
	delete ci;

	int iTabItems = GetItemCount();
	if (iTabItems > 0){
		// select next tab
		if (iCurSel == CB_ERR)
			iCurSel = 0;
		else if (iCurSel >= iTabItems)
			iCurSel = iTabItems - 1;
		(void)SetCurSel(iCurSel);				// returns CB_ERR if error or no prev. selection(!)
		iCurSel = GetCurSel();					// get the real current selection
		if (iCurSel == CB_ERR)					// if still error
			iCurSel = SetCurSel(0);
		ShowChat();
	}
}

void CChatSelector::GetChatSize(CRect& rcChat)
{
	CRect rcClose, rcSend, rcMessage;
	m_pCloseBtn->GetWindowRect(&rcClose);
	m_pSendBtn->GetWindowRect(&rcSend);
	m_pMessageBox->GetWindowRect(&rcMessage);

	int iTop = rcClose.Height() > rcSend.Height() ? rcClose.Height() : rcSend.Height();
	if (iTop < rcMessage.Height())
		iTop = rcMessage.Height();
	
	CRect rcClient;
	GetClientRect(&rcClient);
	AdjustRect(FALSE, rcClient);
	rcChat.left = rcClient.left + 7;
	rcChat.top = rcClient.top + 7;
	rcChat.right = rcChat.left + rcClient.right - 18;
	rcChat.bottom = rcChat.top + rcClient.Height() - 7 - iTop - 14;
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
								rect.right-7-rClose.Width()-7-rSend.Width()-21, rMessage.Height(), SWP_NOZORDER);

	CRect rcChat;
	GetChatSize(rcChat);

	TCITEM item;
	item.mask = TCIF_PARAM;
	int i = 0;
	while (GetItem(i++, &item)){
		CChatItem* ci = (CChatItem*)item.lParam;
		ci->log->SetWindowPos(NULL, rcChat.left, rcChat.top, rcChat.Width(), rcChat.Height(), SWP_NOZORDER);
	}
}

void CChatSelector::OnBnClickedCclose()
{
	EndSession();
}

void CChatSelector::OnBnClickedCsend()
{
	CString strMessage;
	m_pMessageBox->GetWindowText(strMessage);
	strMessage.Trim();
	if (!strMessage.IsEmpty())
	{
		if (SendMessage(strMessage))
			m_pMessageBox->SetWindowText(_T(""));
	}

	m_pMessageBox->SetFocus();
}

BOOL CChatSelector::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN){
			if (pMsg->hwnd == m_pMessageBox->m_hWnd)
				OnBnClickedCsend();
		}

		if (pMsg->hwnd == m_pMessageBox->m_hWnd && (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN)){
			theApp.emuledlg->chatwnd->ScrollHistory(pMsg->wParam == VK_DOWN);
			return TRUE;
		}
	}
	return CClosableTabCtrl::PreTranslateMessage(pMsg);
}

void CChatSelector::Localize(void)
{
	if (m_hWnd)
	{
		if (m_pSendBtn)
			m_pSendBtn->SetWindowText(GetResString(IDS_CW_SEND));
		else
			GetParent()->GetDlgItem(IDC_CSEND)->SetWindowText(GetResString(IDS_CW_SEND));

		if (m_pCloseBtn)
			m_pCloseBtn->SetWindowText(GetResString(IDS_CW_CLOSE));
		else
			GetParent()->GetDlgItem(IDC_CCLOSE)->SetWindowText(GetResString(IDS_CW_CLOSE));
	}
}

bool CChatSelector::IsSpam(CString strMessage, CUpDownClient* client)
{
	// first step, spam dectection will be further improved in future versions
	if ( !theApp.glob_prefs->IsAdvSpamfilterEnabled() || client->IsFriend() ) // friends are never spammer... (but what if two spammers are friends :P )
		return false;

	if (client->m_bIsSpammer)
		return true;

	// first fixed criteria: If a client  sends me an URL in his first message before I response to him
	// there is a 99,9% chance that it is some poor guy advising his leech mod, or selling you .. well you know :P
	if (client->m_cMessagesSend == 0){
		int curPos=0;
		CString resToken = CString(URLINDICATOR).Tokenize(_T("|"), curPos);
		while (resToken != _T("")){
			if (strMessage.Find(resToken) > (-1) )
				return true;
			resToken= CString(URLINDICATOR).Tokenize(_T("|"),curPos);
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
	if (m_Timer){
		KillTimer(m_Timer);
		m_Timer = NULL;
	}
	CClosableTabCtrl::OnDestroy();
}
