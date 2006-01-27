//this file is part of eMule
//Copyright (C)2002-2006 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "./emule.h"
#include "./emuleDlg.h"
#include "./IrcWnd.h"
#include "./IrcMain.h"
#include "./otherfunctions.h"
#include "./MenuCmds.h"
#include "./HTRichEditCtrl.h"
#include "./ClosableTabCtrl.h"
#include "./HelpIDs.h"
#include "./opcodes.h"
#include "./InputBox.h"
#include "./UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define NICK_LV_PROFILE_NAME _T("IRCNicksLv")
#define CHAN_LV_PROFILE_NAME _T("IRCChannelsLv")

struct Nick
{
	CString m_sNick;
	CString m_sModes;
	int m_iLevel;
};

struct Channel
{
	CString	m_sName;
	CString m_sModesA;
	CString m_sModesB;
	CString m_sModesC;
	CString m_sModesD;
	CHTRichEditCtrl m_editctrlLog;
	CString m_sTitle;
	CPtrList m_ptrlistNicks;
	uint8 m_uType;
	CStringArray m_sarrayHistory;
	int m_iHistoryPos;
	// Type is mainly so that we can use this for IRC and the eMule Messages..
	// 1-Status, 2-Channel list, 4-Channel, 5-Private Channel, 6-eMule Message(Add later)
};

IMPLEMENT_DYNAMIC(CIrcWnd, CDialog)

BEGIN_MESSAGE_MAP(CIrcWnd, CResizableDialog)
	// Tab control
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_HELPINFO()
	ON_MESSAGE(UM_CLOSETAB, OnCloseTab)
	ON_MESSAGE(UM_QUERYTAB, OnQueryTab)
END_MESSAGE_MAP()

CIrcWnd::CIrcWnd(CWnd* pParent ) : CResizableDialog(CIrcWnd::IDD, pParent)
{
	m_pIrcMain = NULL;
	m_bConnected = false;
	m_bLoggedIn = false;
	m_listctrlNickList.m_pParent = this;
	m_listctrlServerChannelList.m_pParent = this;
	m_tabctrlChannelSelect.m_bCloseable = true;
	m_tabctrlChannelSelect.m_pParent = this;
}

CIrcWnd::~CIrcWnd()
{
	if( m_bConnected )
	{
		//Do a safe disconnect
		m_pIrcMain->Disconnect(true);
	}
	//Delete our core client..
	delete m_pIrcMain;
}

void CIrcWnd::OnSysColorChange()
{
	CResizableDialog::OnSysColorChange();
}

void CIrcWnd::Localize()
{
	//Set all controls to the correct language.
	if( m_bConnected )
		GetDlgItem(IDC_BN_IRCCONNECT)->SetWindowText(GetResString(IDS_IRC_DISCONNECT));
	else
		GetDlgItem(IDC_BN_IRCCONNECT)->SetWindowText(GetResString(IDS_IRC_CONNECT));
	GetDlgItem(IDC_CHATSEND)->SetWindowText(GetResString(IDS_IRC_SEND));
	GetDlgItem(IDC_CLOSECHAT)->SetWindowText(GetResString(IDS_FD_CLOSE));
	m_listctrlServerChannelList.Localize();
	m_tabctrlChannelSelect.Localize();
	m_listctrlNickList.Localize();
}

BOOL CIrcWnd::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
#ifdef _DEBUG

	CString sBuffer;
	m_listctrlNickList.GetWindowText(sBuffer);
	ASSERT( sBuffer == NICK_LV_PROFILE_NAME );

	sBuffer.Empty();
	m_listctrlServerChannelList.GetWindowText(sBuffer);
	ASSERT( sBuffer == CHAN_LV_PROFILE_NAME );
#endif

	m_bConnected = false;
	m_bLoggedIn = false;
	Localize();
	m_pIrcMain = new CIrcMain();
	m_pIrcMain->SetIRCWnd(this);

	UpdateFonts(&theApp.m_fontHyperText);
	InitWindowStyles(this);

	((CEdit*)GetDlgItem(IDC_INPUTWINDOW))->SetLimitText(MAX_IRC_MSG_LEN);

	CRect rc, rcSpl;

	GetDlgItem(IDC_NICKLIST)->GetWindowRect(rcSpl);
	ScreenToClient(rcSpl);

	GetWindowRect(rc);
	ScreenToClient(rc);

	rcSpl.bottom=rc.bottom-10;
	rcSpl.left=rcSpl.right +3;
	rcSpl.right=rcSpl.left+4;
	m_wndSplitterIRC.Create(WS_CHILD | WS_VISIBLE, rcSpl, this, IDC_SPLITTER_IRC);

	AddAnchor(IDC_BN_IRCCONNECT,BOTTOM_LEFT);
	AddAnchor(IDC_CLOSECHAT,BOTTOM_LEFT);
	AddAnchor(IDC_CHATSEND,BOTTOM_RIGHT);
	AddAnchor(IDC_INPUTWINDOW,BOTTOM_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_NICKLIST,TOP_LEFT,BOTTOM_LEFT);
	AddAnchor(IDC_TITLEWINDOW,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SERVERCHANNELLIST,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_TAB2,TOP_LEFT, TOP_RIGHT);
	AddAnchor(m_wndSplitterIRC,TOP_LEFT, BOTTOM_LEFT);

	int iPosStatInit = rcSpl.left;
	int iPosStatNew = thePrefs.GetSplitterbarPositionIRC();
	if (thePrefs.GetSplitterbarPositionIRC() > 600)
		iPosStatNew = 600;
	else if (thePrefs.GetSplitterbarPositionIRC() < 200)
		iPosStatNew = 200;
	rcSpl.left = iPosStatNew;
	rcSpl.right = iPosStatNew+5;

	m_wndSplitterIRC.MoveWindow(rcSpl);
	DoResize(iPosStatNew-iPosStatInit);

	m_listctrlServerChannelList.Init();
	m_listctrlNickList.Init();
	m_tabctrlChannelSelect.Init();
	OnChatTextChange();

	return true;
}

void CIrcWnd::DoResize(int iDelta)
{

	CSplitterControl::ChangeWidth(GetDlgItem(IDC_NICKLIST), iDelta);
	CSplitterControl::ChangeWidth(GetDlgItem(IDC_INPUTWINDOW), -iDelta, CW_RIGHTALIGN);
	CSplitterControl::ChangeWidth(GetDlgItem(IDC_TITLEWINDOW), -iDelta, CW_RIGHTALIGN);
	CSplitterControl::ChangeWidth(GetDlgItem(IDC_SERVERCHANNELLIST), -iDelta, CW_RIGHTALIGN);
	CSplitterControl::ChangeWidth(GetDlgItem(IDC_STATUSWINDOW), -iDelta, CW_RIGHTALIGN);
	CSplitterControl::ChangeWidth(GetDlgItem(IDC_TAB2), -iDelta, CW_RIGHTALIGN);

	CRect rcChannel;
	m_listctrlServerChannelList.GetWindowRect(&rcChannel);
	ScreenToClient(&rcChannel);
	if (m_tabctrlChannelSelect.m_pCurrentChannel)
		m_tabctrlChannelSelect.m_pCurrentChannel->m_editctrlLog.SetWindowPos(NULL, rcChannel.left, rcChannel.top, rcChannel.Width(), rcChannel.Height(), SWP_NOZORDER);

	CRect rcW;

	GetWindowRect(rcW);
	ScreenToClient(rcW);

	CRect rcspl;
	GetDlgItem(IDC_NICKLIST)->GetClientRect(rcspl);

	thePrefs.SetSplitterbarPositionIRC(rcspl.right);

	RemoveAnchor(IDC_BN_IRCCONNECT);
	AddAnchor(IDC_BN_IRCCONNECT,BOTTOM_LEFT);
	RemoveAnchor(IDC_CLOSECHAT);
	AddAnchor(IDC_CLOSECHAT,BOTTOM_LEFT);
	RemoveAnchor(IDC_INPUTWINDOW);
	AddAnchor(IDC_INPUTWINDOW,BOTTOM_LEFT,BOTTOM_RIGHT);
	RemoveAnchor(IDC_NICKLIST);
	AddAnchor(IDC_NICKLIST,TOP_LEFT,BOTTOM_LEFT);
	RemoveAnchor(IDC_TITLEWINDOW);
	AddAnchor(IDC_TITLEWINDOW,TOP_LEFT,TOP_RIGHT);
	RemoveAnchor(IDC_SERVERCHANNELLIST);
	AddAnchor(IDC_SERVERCHANNELLIST,TOP_LEFT,BOTTOM_RIGHT);
	RemoveAnchor(IDC_TAB2);
	AddAnchor(IDC_TAB2,TOP_LEFT, TOP_RIGHT);
	RemoveAnchor(m_wndSplitterIRC);
	AddAnchor(m_wndSplitterIRC,TOP_LEFT, BOTTOM_LEFT);

	m_wndSplitterIRC.SetRange(rcW.left+190, rcW.left+600);

	Invalidate();
	UpdateWindow();
}

LRESULT CIrcWnd::DefWindowProc(UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
		case WM_PAINT:
			if (m_wndSplitterIRC)
			{
				CRect rctree, rcSpl, rcW;
				CWnd* pWnd;

				GetWindowRect(rcW);
				ScreenToClient(rcW);

				pWnd = GetDlgItem(IDC_NICKLIST);
				pWnd->GetWindowRect(rctree);

				ScreenToClient(rctree);

				if (rcW.Width()>0)
				{
					rcSpl.left=rctree.right;
					rcSpl.right=rcSpl.left+5;
					rcSpl.top=rctree.top;
					rcSpl.bottom=rcW.bottom-40;
					m_wndSplitterIRC.MoveWindow(rcSpl, true);
				}
			}
			break;
		case WM_NOTIFY:
			if (wParam == IDC_SPLITTER_IRC)
			{
				SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
				DoResize(pHdr->delta);
			}
			break;
		case WM_WINDOWPOSCHANGED :
			{
				CRect rcW;
				GetWindowRect(rcW);
				ScreenToClient(rcW);
				if (m_wndSplitterIRC && rcW.Width()>0)
					Invalidate();
				break;
			}
		case WM_SIZE:
			{
				//set range
				if (m_wndSplitterIRC)
				{
					CRect rc;
					GetWindowRect(rc);
					ScreenToClient(rc);
					m_wndSplitterIRC.SetRange(rc.left+190 , rc.left+600);
				}
				break;
			}
	}

	return CResizableDialog::DefWindowProc(uMessage, wParam, lParam);

}

void CIrcWnd::UpdateFonts(CFont* pFont)
{
	TCITEM tci;
	tci.mask = TCIF_PARAM;
	int iIndex = 0;
	while (m_tabctrlChannelSelect.GetItem(iIndex++, &tci))
	{
		Channel* pChannel = (Channel*)tci.lParam;
		if (pChannel->m_editctrlLog.m_hWnd != NULL)
			pChannel->m_editctrlLog.SetFont(pFont);
	}
}

void CIrcWnd::OnSize(UINT uType, int iCx, int iCy)
{
	CResizableDialog::OnSize(uType, iCx, iCy);

	if (m_tabctrlChannelSelect.m_pCurrentChannel && m_tabctrlChannelSelect.m_pCurrentChannel->m_editctrlLog.m_hWnd)
	{
		CRect rcChannel;
		m_listctrlServerChannelList.GetWindowRect(&rcChannel);
		ScreenToClient(&rcChannel);
		m_tabctrlChannelSelect.m_pCurrentChannel->m_editctrlLog.SetWindowPos(NULL, rcChannel.left, rcChannel.top, rcChannel.Width(), rcChannel.Height(), SWP_NOZORDER);
	}
}

int CIrcWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	return CResizableDialog::OnCreate(lpCreateStruct);
}

void CIrcWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NICKLIST, m_listctrlNickList);
	DDX_Control(pDX, IDC_INPUTWINDOW, m_editInputWindow);
	DDX_Control(pDX, IDC_TITLEWINDOW, m_editTitleWindow);
	DDX_Control(pDX, IDC_SERVERCHANNELLIST, m_listctrlServerChannelList);
	DDX_Control(pDX, IDC_TAB2, m_tabctrlChannelSelect);
}

BOOL CIrcWnd::OnCommand(WPARAM wParam, LPARAM)
{
	switch( wParam )
	{
		case IDC_BN_IRCCONNECT:
			{
				//Pressed the connect button..
				OnBnClickedBnIrcconnect();
				return true;
			}
		case IDC_CHATSEND:
			{
				//Pressed the send button..
				OnBnClickedChatsend();
				return true;
			}
		case IDC_CLOSECHAT:
			{
				//Pressed the close button
				OnBnClickedClosechat();
				return true;
			}
	}
	return true;
}

BOOL CIrcWnd::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		// Don't handle Ctrl+Tab in this window. It will be handled by main window.
		if (pMsg->wParam == VK_TAB && GetAsyncKeyState(VK_CONTROL) < 0)
			return FALSE;

		if (pMsg->hwnd == GetDlgItem(IDC_INPUTWINDOW)->m_hWnd)
		{
			if (pMsg->wParam == VK_RETURN)
			{
				//If we press the enter key, treat is as if we pressed the send button.
				OnBnClickedChatsend();
				return TRUE;
			}

			if (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN)
			{
				//If we press page up/down scroll..
				m_tabctrlChannelSelect.ScrollHistory(pMsg->wParam == VK_DOWN);
				return TRUE;
			}

			if (pMsg->wParam == VK_TAB)
			{
				AutoComplete();
				return TRUE;
			}
		}
	}
	OnChatTextChange();
	return CResizableDialog::PreTranslateMessage(pMsg);
}

void CIrcWnd::AutoComplete()
{
	CString sSend;
	CString sName;
	GetDlgItem(IDC_INPUTWINDOW)->GetWindowText(sSend);
	if( sSend.ReverseFind(_T(' ')) == -1 )
	{
		if(!sSend.GetLength())
			return;
		sName = sSend;
		sSend = _T("");
	}
	else
	{
		sName = sSend.Mid(sSend.ReverseFind(_T(' '))+1);
		sSend = sSend.Mid(0, sSend.ReverseFind(_T(' '))+1);
	}

	POSITION pos1, pos2;
	for (pos1 = m_tabctrlChannelSelect.m_pCurrentChannel->m_ptrlistNicks.GetHeadPosition();( pos2 = pos1 ) != NULL;)
	{
		m_tabctrlChannelSelect.m_pCurrentChannel->m_ptrlistNicks.GetNext(pos1);
		Nick* pCurrNick = (Nick*)(m_tabctrlChannelSelect.m_pCurrentChannel)->m_ptrlistNicks.GetAt(pos2);
		if (pCurrNick->m_sNick.Left(sName.GetLength()) == sName)
		{
			sName = pCurrNick->m_sNick;
			GetDlgItem(IDC_INPUTWINDOW)->SetWindowText(sSend+sName);
			GetDlgItem(IDC_INPUTWINDOW)->SetFocus();
			GetDlgItem(IDC_INPUTWINDOW)->SendMessage(WM_KEYDOWN, VK_END);
			break;
		}
	}
}

void CIrcWnd::OnBnClickedBnIrcconnect()
{
	if(!m_bConnected)
	{
		CString sInput = thePrefs.GetIRCNick();
		sInput.Trim();
		sInput = sInput.SpanExcluding(_T(" !@#$%^&*():;<>,.?{}~`+=-"));
		sInput = sInput.Left(25);
		if( thePrefs.GetIRCNick().MakeLower() == _T("emule") || thePrefs.GetIRCNick().MakeLower().Find(_T("emuleirc")) != -1 || sInput == "" )
		{
			do
			{
				InputBox inputBox;
				inputBox.SetLabels(GetResString(IDS_IRC_NEWNICK), GetResString(IDS_IRC_NEWNICKDESC), _T("eMule"));
				if (inputBox.DoModal() == IDOK)
				{
					sInput = inputBox.GetInput();
					sInput.Trim();
					sInput = sInput.SpanExcluding(_T(" !@#$%^&*():;<>,.?{}~`+=-"));
					sInput = sInput.Left(25);
				}
				else
				{
					if(sInput == "")
						sInput = _T("eMule");
				}
			}
			while(sInput == "");
		}
		thePrefs.SetIRCNick(sInput);
		//if not connected, connect..
		m_pIrcMain->Connect();
	}
	else
	{
		//If connected, disconnect..
		m_pIrcMain->Disconnect();
	}
}

void CIrcWnd::OnBnClickedClosechat(int iItem)
{
	//Remove a channel..
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (iItem == -1)
	{
		//If no item was send, get our current channel..
		iItem = m_tabctrlChannelSelect.GetCurSel();
	}

	if (iItem == -1)
	{
		//We have no channel, abort.
		return;
	}

	if (!m_tabctrlChannelSelect.GetItem(iItem, &item))
	{
		//We had no valid item here.. Something isn't right..
		//TODO: this should never happen, so maybe we should remove this tab?
		return;
	}
	Channel* pPartChannel = (Channel*)item.lParam;
	if( pPartChannel->m_uType == 4 &&  m_bConnected)
	{
		//If this was a channel and we were connected, do not just delete the channel!!
		//Send a part command and the server must respond with a successful part which will remove the channel!
		CString sPart;
		sPart = _T("PART ") + pPartChannel->m_sName;
		m_pIrcMain->SendString( sPart );
		return;
	}
	else if (pPartChannel->m_uType == 5 || pPartChannel->m_uType == 4)
	{
		//If this is a private room, we just remove it as the server doesn't track this.
		//If this was a channel, but we are disconnected, remove the channel..
		m_tabctrlChannelSelect.RemoveChannel(pPartChannel->m_sName);
		return;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Messages
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void CIrcWnd::AddStatus( CString sLine,...)
{
	//Add entry to status window with arguments..
	va_list argptr;
	va_start(argptr, sLine);
	CString sTemp;
	sTemp.FormatV(sLine, argptr);
	va_end(argptr);
	CString sTimeStamp;
	if( thePrefs.GetIRCAddTimestamp() )
	{
		//Append time stamp..
		sTimeStamp = CTime::GetCurrentTime().Format(_T("%X: "));
	}
	Channel* pUpdateChannel = (Channel*)(m_tabctrlChannelSelect.m_ptrlistChannel).GetHead();
	if( !pUpdateChannel )
	{
		//This should never happen!
		return;
	}
	//We do not support color codes..
	sLine = StripMessageOfFontCodes( sTemp );
	sLine += _T("\r\n");
	//Now that incoming arguments are finished, it's now safe to put back the % chars.
	sLine.Replace( _T("\004"), _T("%") );
	if (sLine == _T("\r\n") )
	{
		//This allows for us to add blank lines to the status..
		pUpdateChannel->m_editctrlLog.AppendText(sLine);
	}
	else if (sLine.Mid(0,1) == _T("*"))
	{
		pUpdateChannel->m_editctrlLog.AppendText(sTimeStamp);
		pUpdateChannel->m_editctrlLog.AppendKeyWord(sLine.Left(2),RGB(255,0,0));
		pUpdateChannel->m_editctrlLog.AppendText(sLine.Mid(1) );
	}
	else
		pUpdateChannel->m_editctrlLog.AppendText(sTimeStamp + sLine);
	if( m_tabctrlChannelSelect.m_pCurrentChannel == pUpdateChannel )
		return;
	m_tabctrlChannelSelect.SetActivity( pUpdateChannel->m_sName, true );
}

void CIrcWnd::AddInfoMessage( CString sChannelName, CString sLine,...)
{
	if(sChannelName.IsEmpty())
		return;
	va_list argptr;
	va_start(argptr, sLine);
	CString sTemp;
	sTemp.FormatV(sLine, argptr);
	va_end(argptr);
	CString sTimeStamp = _T("");
	if( thePrefs.GetIRCAddTimestamp() )
		sTimeStamp = CTime::GetCurrentTime().Format(_T("%X: "));
	Channel* pUpdateChannel = m_tabctrlChannelSelect.FindChannelByName(sChannelName);
	if( !pUpdateChannel )
	{
		if( sChannelName.Left(1) == _T("#") )
			pUpdateChannel = m_tabctrlChannelSelect.NewChannel( sChannelName, 4);
		else
			pUpdateChannel = m_tabctrlChannelSelect.NewChannel( sChannelName, 5);
	}
	sLine = StripMessageOfFontCodes( sTemp );
	sLine += _T("\r\n");
	sLine.Replace( _T("\004"), _T("%") );
	if (sLine.Mid(0,1) == _T("*"))
	{
		pUpdateChannel->m_editctrlLog.AppendText(sTimeStamp);
		pUpdateChannel->m_editctrlLog.AppendKeyWord(sLine.Left(2),RGB(255,0,0));
		pUpdateChannel->m_editctrlLog.AppendText(sLine.Mid(1) );
	}
	else if (sLine.Mid(0,1) == _T("-") && sLine.Find( _T("-"), 1 ) != -1)
	{
		int iIndex = sLine.Find( _T("-"), 1 );
		pUpdateChannel->m_editctrlLog.AppendText(sTimeStamp);
		pUpdateChannel->m_editctrlLog.AppendKeyWord(sLine.Left(iIndex),RGB(150,0,0));
		pUpdateChannel->m_editctrlLog.AppendText(sLine.Mid(iIndex) );
	}
	else
		pUpdateChannel->m_editctrlLog.AppendText(sTimeStamp + sLine);

	if( m_tabctrlChannelSelect.m_pCurrentChannel == pUpdateChannel )
		return;
	m_tabctrlChannelSelect.SetActivity( pUpdateChannel->m_sName, true );
}

void CIrcWnd::AddMessage( CString sChannelName, CString sTargetName, CString sLine,...)
{
	if(sChannelName.IsEmpty() || sTargetName.IsEmpty())
		return;
	va_list argptr;
	va_start(argptr, sLine);
	CString sTemp;
	sTemp.FormatV(sLine, argptr);
	sLine = sTemp;
	va_end(argptr);
	CString sTimeStamp = _T("");
	if( thePrefs.GetIRCAddTimestamp() )
		sTimeStamp = CTime::GetCurrentTime().Format(_T("%X: "));
	Channel* pUpdateChannel = m_tabctrlChannelSelect.FindChannelByName(sChannelName);
	if( !pUpdateChannel )
	{
		if( sChannelName.Left(1) == _T("#") )
			pUpdateChannel = m_tabctrlChannelSelect.NewChannel( sChannelName, 4);
		else
			pUpdateChannel = m_tabctrlChannelSelect.NewChannel( sChannelName, 5);
	}
	sLine = StripMessageOfFontCodes( sLine );
	sLine += _T("\r\n");
	sLine.Replace( _T("\004"), _T("%") );
	COLORREF color;
	if (m_pIrcMain->GetNick() == sTargetName)
		color = RGB(1,100,1);
	else
		color = RGB(1,20,130);
	sTargetName = CString(_T("<"))+ sTargetName + CString(_T(">"));
	pUpdateChannel->m_editctrlLog.AppendText(sTimeStamp);
	pUpdateChannel->m_editctrlLog.AppendKeyWord(sTargetName, color);
	pUpdateChannel->m_editctrlLog.AppendText(CString(_T(" "))+sLine);
	if( m_tabctrlChannelSelect.m_pCurrentChannel == pUpdateChannel )
		return;
	m_tabctrlChannelSelect.SetActivity( pUpdateChannel->m_sName, true );
}

void CIrcWnd::SetConnectStatus( bool bFlag )
{
	if(bFlag)
	{
		GetDlgItem(IDC_BN_IRCCONNECT)->SetWindowText(GetResString(IDS_IRC_DISCONNECT));
		AddStatus( GetResString(IDS_CONNECTED));
		m_bConnected = true;
	}
	else
	{
		GetDlgItem(IDC_BN_IRCCONNECT)->SetWindowText(GetResString(IDS_IRC_CONNECT));
		AddStatus( GetResString(IDS_DISCONNECTED));
		m_bConnected = false;
		m_bLoggedIn = false;
		while( m_tabctrlChannelSelect.m_ptrlistChannel.GetCount() > 2 )
		{
			Channel* pToDel = (Channel*)(m_tabctrlChannelSelect.m_ptrlistChannel).GetTail();
			m_tabctrlChannelSelect.RemoveChannel( pToDel->m_sName );
		}
	}
}

void CIrcWnd::NoticeMessage( CString sSource, CString sTarget, CString sMessage )
{
	bool bFlag = false;
	if( m_tabctrlChannelSelect.FindChannelByName( sTarget ) )
	{
		AddInfoMessage( sTarget, _T("-%s:%s- %s"), sSource, sTarget, sMessage);
		bFlag = true;
	}
	else
	{
		for (POSITION pos1 = m_tabctrlChannelSelect.m_ptrlistChannel.GetHeadPosition(); pos1 != NULL;)
		{
			Channel* pCurrChannel = (Channel*)(m_tabctrlChannelSelect.m_ptrlistChannel).GetNext(pos1);
			if(pCurrChannel)
			{
				Nick* pCurrNick = m_listctrlNickList.FindNickByName(pCurrChannel->m_sName, sSource );
				if( pCurrNick)
				{
					AddInfoMessage( pCurrChannel->m_sName, _T("-%s:%s- %s"), sSource, sTarget, sMessage);
					bFlag = true;
				}
			}
		}
	}
	if( bFlag == false )
		AddStatus( _T("-%s:%s- %s"), sSource, sTarget, sMessage );
}

//We cannot support color within the text since HyperTextCtrl does not detect hyperlinks with color. So I will filter it.
CString CIrcWnd::StripMessageOfFontCodes( CString sTemp )
{
	sTemp = StripMessageOfColorCodes( sTemp );
	sTemp.Replace(_T("\002"),_T(""));
	sTemp.Replace(_T("\003"),_T(""));
	sTemp.Replace(_T("\017"),_T(""));
	sTemp.Replace(_T("\026"),_T(""));
	sTemp.Replace(_T("\037"),_T(""));
	return sTemp;
}

CString CIrcWnd::StripMessageOfColorCodes( CString sTemp )
{
	if( !sTemp.IsEmpty() )
	{
		CString sTemp1, sTemp2;
		int iTest = sTemp.Find( 3 );
		if( iTest != -1 )
		{
			int iTestLength = sTemp.GetLength() - iTest;
			if( iTestLength < 2 )
				return sTemp;
			sTemp1 = sTemp.Left( iTest );
			sTemp2 = sTemp.Mid( iTest + 2);
			if( iTestLength < 4 )
				return sTemp1+sTemp2;
			if( sTemp2[0] == 44 && sTemp2.GetLength() > 2)
			{
				sTemp2 = sTemp2.Mid(2);
				for( int iIndex = 48; iIndex < 58; iIndex++ )
				{
					if( sTemp2[0] == iIndex )
						sTemp2 = sTemp2.Mid(1);
				}
			}
			else
			{
				for( int iIndex = 48; iIndex < 58; iIndex++ )
				{
					if( sTemp2[0] == iIndex )
					{
						sTemp2 = sTemp2.Mid(1);
						if( sTemp2[0] == 44 && sTemp2.GetLength() > 2)
						{
							sTemp2 = sTemp2.Mid(2);
							for( int iIndex = 48; iIndex < 58; iIndex++ )
							{
								if( sTemp2[0] == iIndex )
									sTemp2 = sTemp2.Mid(1);
							}
						}
					}
				}
			}
			sTemp = sTemp1 + sTemp2;
			sTemp = StripMessageOfColorCodes(sTemp);
		}
	}
	return sTemp;
}

void CIrcWnd::SetTitle( CString sChannel, CString sTitle )
{
	Channel* pCurrChannel = m_tabctrlChannelSelect.FindChannelByName(sChannel);
	if(!pCurrChannel)
		return;
	pCurrChannel->m_sTitle = StripMessageOfFontCodes(sTitle);
	if( pCurrChannel == m_tabctrlChannelSelect.m_pCurrentChannel )
		m_editTitleWindow.SetWindowText( pCurrChannel->m_sTitle );
}

void CIrcWnd::OnBnClickedChatsend()
{
	CString sSend;
	GetDlgItem(IDC_INPUTWINDOW)->GetWindowText(sSend);
	GetDlgItem(IDC_INPUTWINDOW)->SetWindowText(_T(""));
	GetDlgItem(IDC_INPUTWINDOW)->SetFocus();
	m_tabctrlChannelSelect.Chatsend(sSend);
}

void CIrcWnd::SendString( CString sSend )
{
	if( this->m_bConnected )
		m_pIrcMain->SendString( sSend );
}

BOOL CIrcWnd::OnHelpInfo(HELPINFO*)
{
	theApp.ShowHelp(eMule_FAQ_IRC_Chat);
	return TRUE;
}

void CIrcWnd::OnChatTextChange()
{
	GetDlgItem(IDC_CHATSEND)->EnableWindow( GetDlgItem(IDC_INPUTWINDOW)->GetWindowTextLength()>0 );
}

void CIrcWnd::ParseChangeMode( CString sChannel, CString sChanger, CString sCommands, CString sParams )
{
	CString sCommandsOrig = sCommands;
	CString sParamsOrig = sParams;
	try
	{
		if( sCommands.GetLength() >= 2 )
		{
			CString sDir;
			int iParamIndex = 0;
			while( !sCommands.IsEmpty() )
			{
				if( sCommands.Left(1) == _T("+") || sCommands.Left(1) == _T("-") )
				{
					sDir = sCommands.Left(1);
					sCommands = sCommands.Right(sCommands.GetLength()-1);
				}
				if( !sCommands.IsEmpty() && !sDir.IsEmpty() )
				{
					CString sCommand = sCommands.Left(1);
					sCommands = sCommands.Right(sCommands.GetLength()-1);
					
					if(m_listctrlNickList.m_sUserModeSettings.Find(sCommand) != -1 )
					{
						//This is a user mode change and must have a param!
						CString sParam = sParams.Tokenize(_T(" "), iParamIndex);
						m_listctrlNickList.ChangeNickMode( sChannel, sParam, sDir + sCommand);
					}
					if(m_tabctrlChannelSelect.m_sChannelModeSettingsTypeA.Find(sCommand) != -1)
					{
						//We do not use these messages yet.. But we can display them for the user to see
						//These modes always have a param and will add or remove a user from some type of list.
						CString sParam = sParams.Tokenize(_T(" "), iParamIndex);
						m_tabctrlChannelSelect.ChangeChanMode( sChannel, sParam, sDir, sCommand);
					}
					if(m_tabctrlChannelSelect.m_sChannelModeSettingsTypeB.Find(sCommand) != -1)
					{
						//We do not use these messages yet.. But we can display them for the user to see
						//These modes will always have a param..
						CString sParam = sParams.Tokenize(_T(" "), iParamIndex);
						m_tabctrlChannelSelect.ChangeChanMode( sChannel, sParams, sDir, sCommand);
					}
					if(m_tabctrlChannelSelect.m_sChannelModeSettingsTypeC.Find(sCommand) != -1 )
					{
						//We do not use these messages yet.. But we can display them for the user to see
						//These modes will only have a param if your setting it!
						CString sParam = _T("");
						if( sDir == _T("+") )
							sParam = sParams.Tokenize(_T(" "), iParamIndex);

						m_tabctrlChannelSelect.ChangeChanMode( sChannel, sParam, sDir, sCommand);
					}
					if(m_tabctrlChannelSelect.m_sChannelModeSettingsTypeD.Find(sCommand) != -1 )
					{
						//We do not use these messages yet.. But we can display them for the user to see
						//These modes will never have a param for it!
						CString sParam = _T("");
						m_tabctrlChannelSelect.ChangeChanMode( sChannel, sParam, sDir, sCommand);

					}
				}
			}
			if( !thePrefs.GetIrcIgnoreMiscMessage() )
				AddInfoMessage( sChannel, GetResString(IDS_IRC_SETSMODE), sChanger, sCommandsOrig, sParamsOrig);
		}
	}
	catch(...)
	{
		AddInfoMessage( sChannel, GetResString(IDS_IRC_NOTSUPPORTED));
		ASSERT(0);
	}
}

LRESULT CIrcWnd::OnCloseTab(WPARAM wparam, LPARAM)
{

	OnBnClickedClosechat( (int)wparam );

	return TRUE;
}

LRESULT CIrcWnd::OnQueryTab(WPARAM wParam, LPARAM)
{
	int iItem = (int)wParam;

	TCITEM item;
	item.mask = TCIF_PARAM;
	m_tabctrlChannelSelect.GetItem(iItem, &item);
	Channel* pPartChannel = (Channel*)item.lParam;
	if (pPartChannel)
	{
		if (pPartChannel->m_uType == 4 && m_bConnected)
		{
			return 0;
		}
		else if (pPartChannel->m_uType == 5 || pPartChannel->m_uType == 4)
		{
			return 0;
		}
	}
	return 1;
}
bool CIrcWnd::GetLoggedIn()
{
	return m_bLoggedIn;
}
void CIrcWnd::SetLoggedIn( bool bFlag )
{
	m_bLoggedIn = bFlag;
}
void CIrcWnd::SetSendFileString( CString sInFile )
{
	m_sSendString = sInFile;
}
CString CIrcWnd::GetSendFileString()
{
	return m_sSendString;
}
bool CIrcWnd::IsConnected()
{
	return m_bConnected;
}
