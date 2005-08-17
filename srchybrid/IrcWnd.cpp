//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "HelpIDs.h"
#include "Opcodes.h"
#include "InputBox.h"
#include "UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define NICK_LV_PROFILE_NAME _T("IRCNicksLv")
#define CHAN_LV_PROFILE_NAME _T("IRCChannelsLv")

struct Nick
{
	CString nick;
	CString modes;
	int level;
};

struct Channel
{
	CString	name;
	CString modesA;
	CString modesB;
	CString modesC;
	CString modesD;
	CHTRichEditCtrl log;
	CString title;
	CPtrList nicks;
	uint8 type;
	CStringArray history;
	uint16 history_pos;
	// Type is mainly so that we can use this for IRC and the eMule Messages..
	// 1-Status, 2-Channel list, 4-Channel, 5-Private Channel, 6-eMule Message(Add later)
};

IMPLEMENT_DYNAMIC(CIrcWnd, CDialog)

BEGIN_MESSAGE_MAP(CIrcWnd, CDialog)
	// Tab control
	ON_WM_SIZE()
	ON_WM_CREATE()
    ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_HELPINFO()
	ON_MESSAGE(UM_CLOSETAB, OnCloseTab)
	ON_MESSAGE(UM_QUERYTAB, OnQueryTab)
END_MESSAGE_MAP()

CIrcWnd::CIrcWnd(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CIrcWnd::IDD, pParent)
{
	m_pIrcMain = NULL;
	m_bConnected = false;
	m_bLoggedIn = false;
	m_nicklist.m_pParent = this;
	m_serverChannelList.m_pParent = this;
	m_channelselect.m_bCloseable = true;
	m_channelselect.m_pParent = this;
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
	m_serverChannelList.Localize();
	m_channelselect.Localize();
	m_nicklist.Localize();
}

BOOL CIrcWnd::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
#ifdef _DEBUG
	CString strBuff;
	m_nicklist.GetWindowText(strBuff);
	ASSERT( strBuff == NICK_LV_PROFILE_NAME );

	strBuff.Empty();
	m_serverChannelList.GetWindowText(strBuff);
	ASSERT( strBuff == CHAN_LV_PROFILE_NAME );
#endif

	m_bConnected = false;
	m_bLoggedIn = false;
	Localize();
	m_pIrcMain = new CIrcMain();
	m_pIrcMain->SetIRCWnd(this);

	UpdateFonts(&theApp.m_fontHyperText);
	InitWindowStyles(this);

	((CEdit*)GetDlgItem(IDC_INPUTWINDOW))->SetLimitText(MAX_IRC_MSG_LEN);

	//MORPH START -Added by SiRoB, Splitting Bar [O²]
	CRect rc,rcSpl;

	GetDlgItem(IDC_NICKLIST)->GetWindowRect(rcSpl);
	ScreenToClient(rcSpl);
	
	GetWindowRect(rc);
	ScreenToClient(rc);

	rcSpl.bottom=rc.bottom-10; rcSpl.left=rcSpl.right +3; rcSpl.right=rcSpl.left+4;
	m_wndSplitterIRC.Create(WS_CHILD | WS_VISIBLE, rcSpl, this, IDC_SPLITTER_IRC);
	//MORPH END   - Added by SiRoB, Splitting Bar [O²]

	AddAnchor(IDC_BN_IRCCONNECT,BOTTOM_LEFT);
	AddAnchor(IDC_CLOSECHAT,BOTTOM_LEFT);
	AddAnchor(IDC_CHATSEND,BOTTOM_RIGHT);
	AddAnchor(IDC_INPUTWINDOW,BOTTOM_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_NICKLIST,TOP_LEFT,BOTTOM_LEFT);
	AddAnchor(IDC_TITLEWINDOW,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SERVERCHANNELLIST,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_TAB2,TOP_LEFT, TOP_RIGHT);
	//MORPH START - Added by SiRoB, Splitting Bar [O²]
	AddAnchor(m_wndSplitterIRC,TOP_LEFT, BOTTOM_LEFT);
	
	int PosStatinit = rcSpl.left;
	int PosStatnew = thePrefs.GetSplitterbarPositionIRC();
	if (thePrefs.GetSplitterbarPositionIRC() > 600) PosStatnew = 600;
	else if (thePrefs.GetSplitterbarPositionIRC() < 200) PosStatnew = 200;
	rcSpl.left = PosStatnew;
	rcSpl.right = PosStatnew+5;

	m_wndSplitterIRC.MoveWindow(rcSpl);
	DoResize(PosStatnew-PosStatinit);
	//MORPH END   - Added by SiRoB, Splitting Bar [O²]

	m_serverChannelList.Init();
	m_nicklist.Init();
	m_channelselect.Init();
	OnChatTextChange();

	return true;
}

//MORPH START - Added by SiRoB, Splitting Bar [O²]
void CIrcWnd::DoResize(int delta)
{

	CSplitterControl::ChangeWidth(GetDlgItem(IDC_NICKLIST), delta);
	CSplitterControl::ChangeWidth(GetDlgItem(IDC_INPUTWINDOW), -delta, CW_RIGHTALIGN);
	CSplitterControl::ChangeWidth(GetDlgItem(IDC_TITLEWINDOW), -delta, CW_RIGHTALIGN);
	CSplitterControl::ChangeWidth(GetDlgItem(IDC_SERVERCHANNELLIST), -delta, CW_RIGHTALIGN);
	CSplitterControl::ChangeWidth(GetDlgItem(IDC_STATUSWINDOW), -delta, CW_RIGHTALIGN);
	CSplitterControl::ChangeWidth(GetDlgItem(IDC_TAB2), -delta, CW_RIGHTALIGN);

	CRect rcChannel;
	m_serverChannelList.GetWindowRect(&rcChannel);
	ScreenToClient(&rcChannel);
	if (m_channelselect.m_pCurrentChannel) m_channelselect.m_pCurrentChannel->log.SetWindowPos(NULL, rcChannel.left, rcChannel.top, rcChannel.Width(), rcChannel.Height(), SWP_NOZORDER);

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
	//initCSize(thePrefs.GetSplitterbarPositionIRC());

	Invalidate();
	UpdateWindow();
}

LRESULT CIrcWnd::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
switch (message) {
 case WM_PAINT:
  if (m_wndSplitterIRC) {
  
   CRect rctree,rcSpl,rcW;
   CWnd* pWnd;

   GetWindowRect(rcW);
   ScreenToClient(rcW);

   pWnd = GetDlgItem(IDC_NICKLIST);
   pWnd->GetWindowRect(rctree);

   ScreenToClient(rctree);
  

   if (rcW.Width()>0) {

	rcSpl.left=rctree.right;
    rcSpl.right=rcSpl.left+5;
    rcSpl.top=rctree.top;
    rcSpl.bottom=rcW.bottom-40;
    
    m_wndSplitterIRC.MoveWindow(rcSpl,true);
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

   if (m_wndSplitterIRC && rcW.Width()>0) Invalidate();
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

return CResizableDialog::DefWindowProc(message, wParam, lParam);

}

//MORPH END - Added by SiRoB, Splitting Bar [O²]


void CIrcWnd::UpdateFonts(CFont* pFont)
{
	TCITEM tci;
	tci.mask = TCIF_PARAM;
	int i = 0;
	while (m_channelselect.GetItem(i++, &tci))
	{
		Channel* ch = (Channel*)tci.lParam;
		if (ch->log.m_hWnd != NULL)
			ch->log.SetFont(pFont);
	}
}

void CIrcWnd::OnSize(UINT nType, int cx, int cy) 
{
	CResizableDialog::OnSize(nType, cx, cy);

	if (m_channelselect.m_pCurrentChannel && m_channelselect.m_pCurrentChannel->log.m_hWnd)
	{
		CRect rcChannel;
		m_serverChannelList.GetWindowRect(&rcChannel);
		ScreenToClient(&rcChannel);
		m_channelselect.m_pCurrentChannel->log.SetWindowPos(NULL, rcChannel.left, rcChannel.top, rcChannel.Width(), rcChannel.Height(), SWP_NOZORDER);
	}
}

int CIrcWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	return CResizableDialog::OnCreate(lpCreateStruct);
}

void CIrcWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NICKLIST, m_nicklist);
	DDX_Control(pDX, IDC_INPUTWINDOW, inputWindow);
	DDX_Control(pDX, IDC_TITLEWINDOW, titleWindow);
	DDX_Control(pDX, IDC_SERVERCHANNELLIST, m_serverChannelList);
	DDX_Control(pDX, IDC_TAB2, m_channelselect);
}

BOOL CIrcWnd::OnCommand(WPARAM wParam,LPARAM lParam )
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
	if(pMsg->message == WM_KEYDOWN && (pMsg->hwnd == GetDlgItem(IDC_INPUTWINDOW)->m_hWnd)) {
		if (pMsg->wParam == VK_RETURN) 
		{
			//If we press the enter key, treat is as if we pressed the send button.
			OnBnClickedChatsend();
			return TRUE;
		}

		if (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN) 
		{
			//If we press page up/down scroll..
			m_channelselect.ScrollHistory(pMsg->wParam == VK_DOWN);
			return TRUE;
		}

		if (pMsg->wParam == VK_TAB )
		{
			AutoComplete();
			return true;
		}
	}
	OnChatTextChange();
	return CResizableDialog::PreTranslateMessage(pMsg);
}

void CIrcWnd::AutoComplete()
{
	CString send;
	CString name;
	GetDlgItem(IDC_INPUTWINDOW)->GetWindowText(send);
	if( send.ReverseFind(_T(' ')) == -1 )
	{
		if(!send.GetLength())
			return;
		name = send;
		send = _T("");
	}
	else
	{
		name = send.Mid(send.ReverseFind(_T(' '))+1);
		send = send.Mid(0, send.ReverseFind(_T(' '))+1);
	}

	POSITION pos1, pos2;
	for (pos1 = m_channelselect.m_pCurrentChannel->nicks.GetHeadPosition();( pos2 = pos1 ) != NULL;)
	{
		m_channelselect.m_pCurrentChannel->nicks.GetNext(pos1);
		Nick* cur_nick = (Nick*)(m_channelselect.m_pCurrentChannel)->nicks.GetAt(pos2);
		if (cur_nick->nick.Left(name.GetLength()) == name)
		{
			name = cur_nick->nick;
			GetDlgItem(IDC_INPUTWINDOW)->SetWindowText(send+name);
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
		if( thePrefs.GetIRCNick().MakeLower() == _T("emule") || thePrefs.GetIRCNick().MakeLower().Find(_T("emuleirc")) != -1 )
		{
			InputBox inputbox;
			inputbox.SetLabels(GetResString(IDS_IRC_NEWNICK), GetResString(IDS_IRC_NEWNICKDESC), _T("eMule"));
			if (inputbox.DoModal() == IDOK)
			{
				CString input = inputbox.GetInput();
				input.Trim();
				input = input.SpanExcluding(_T(" !@#$%^&*():;<>,.?{}~`+=-"));
				if( input != "" )
					thePrefs.SetIRCNick(input.GetBuffer());
			}
		}
		//if not connected, connect..
		m_pIrcMain->Connect();
	}
	else
	{
		//If connected, disconnect..
		m_pIrcMain->Disconnect();
	}
}

void CIrcWnd::OnBnClickedClosechat(int nItem)
{
	//Remove a channel..
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (nItem == -1)
	{
		//If no item was send, get our current channel..
		nItem = m_channelselect.GetCurSel();
	}

	if (nItem == -1)
	{
		//We have no channel, abort.
		return;
	}

	if (!m_channelselect.GetItem(nItem, &item))
	{
		//We had no valid item here.. Something isn't right..
		//TODO: this should never happen, so maybe we should remove this tab?
		return;
	}
	Channel* partChannel = (Channel*)item.lParam;
	if( partChannel->type == 4 &&  m_bConnected)
	{
		//If this was a channel and we were connected, do not just delete the channel!!
		//Send a part command and the server must respond with a successful part which will remove the channel!
		CString part;
		part = _T("PART ") + partChannel->name;
		m_pIrcMain->SendString( part );
		return;
	}
	else if (partChannel->type == 5 || partChannel->type == 4)
	{
		//If this is a private room, we just remove it as the server doesn't track this.
		//If this was a channel, but we are disconnected, remove the channel..
		m_channelselect.RemoveChannel(partChannel->name);
		return;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Messages
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void CIrcWnd::AddStatus( CString line,...)
{
	//Add entry to status window with arguments..
	va_list argptr;
	va_start(argptr, line);
	CString temp;
	temp.FormatV(line, argptr);
	va_end(argptr);
	CString timestamp;
	if( thePrefs.GetIRCAddTimestamp() )
	{
		//Append time stamp..
		timestamp = CTime::GetCurrentTime().Format(_T("%X: "));
	}
	Channel* update_channel = (Channel*)(m_channelselect.channelPtrList).GetHead();
	if( !update_channel )
	{
		//This should never happen!
		return;
	}
	//We do not support color codes..
	line = StripMessageOfFontCodes( temp );
	line += _T("\r\n");
	//Now that incoming arguments are finished, it's now safe to put back the % chars.
	line.Replace( _T("\004"), _T("%") );
	if (line == _T("\r\n") )
	{
		//This allows for us to add blank lines to the status..
		update_channel->log.AppendText(line);
	}
	else if (line.Mid(0,1) == _T("*"))
	{
		update_channel->log.AppendText(timestamp);
		update_channel->log.AppendKeyWord(line.Left(2),RGB(255,0,0));
		update_channel->log.AppendText(line.Mid(1) );
	}
	else
		update_channel->log.AppendText(timestamp + line);
	if( m_channelselect.m_pCurrentChannel == update_channel )
		return;
	m_channelselect.SetActivity( update_channel->name, true );
}

void CIrcWnd::AddInfoMessage( CString channelName, CString line,...)
{
	if(channelName.IsEmpty())
		return;
	va_list argptr;
	va_start(argptr, line);
	CString temp;
	temp.FormatV(line, argptr);
	va_end(argptr);
	CString timestamp = _T("");
	if( thePrefs.GetIRCAddTimestamp() )
		timestamp = CTime::GetCurrentTime().Format(_T("%X: "));
	Channel* update_channel = m_channelselect.FindChannelByName(channelName);
	if( !update_channel )
	{
		if( channelName.Left(1) == _T("#") )
			update_channel = m_channelselect.NewChannel( channelName, 4);
		else
			update_channel = m_channelselect.NewChannel( channelName, 5);
	}
	line = StripMessageOfFontCodes( temp );
	line += _T("\r\n");
	line.Replace( _T("\004"), _T("%") );
	if (line.Mid(0,1) == _T("*"))
	{
		update_channel->log.AppendText(timestamp);
		update_channel->log.AppendKeyWord(line.Left(2),RGB(255,0,0));
		update_channel->log.AppendText(line.Mid(1) );
	}
	else if (line.Mid(0,1) == _T("-") && line.Find( _T("-"), 1 ) != -1)
	{
		int index = line.Find( _T("-"), 1 );
		update_channel->log.AppendText(timestamp);
		update_channel->log.AppendKeyWord(line.Left(index),RGB(150,0,0));
		update_channel->log.AppendText(line.Mid(index) );
	}
	else
		update_channel->log.AppendText(timestamp + line);
	
	if( m_channelselect.m_pCurrentChannel == update_channel )
		return;
	m_channelselect.SetActivity( update_channel->name, true );
}

void CIrcWnd::AddMessage( CString channelName, CString targetname, CString line,...)
{
	if(channelName.IsEmpty() || targetname.IsEmpty())
		return;
	va_list argptr;
	va_start(argptr, line);
	CString temp;
	temp.FormatV(line, argptr);
	line = temp;
	va_end(argptr);
	CString timestamp = _T("");
	if( thePrefs.GetIRCAddTimestamp() )
		timestamp = CTime::GetCurrentTime().Format(_T("%X: "));
	Channel* update_channel = m_channelselect.FindChannelByName(channelName);
	if( !update_channel )
	{
		if( channelName.Left(1) == _T("#") )
			update_channel = m_channelselect.NewChannel( channelName, 4);
		else
			update_channel = m_channelselect.NewChannel( channelName, 5);
	}
	line = StripMessageOfFontCodes( line );
	line += _T("\r\n");
	line.Replace( _T("\004"), _T("%") );
	COLORREF color;
	if (m_pIrcMain->GetNick() == targetname)
		color = RGB(1,100,1);
	else
		color = RGB(1,20,130);	
	targetname = CString(_T("<"))+ targetname + CString(_T(">"));
	update_channel->log.AppendText(timestamp);
	update_channel->log.AppendKeyWord(targetname, color);
	update_channel->log.AppendText(CString(_T(" "))+line);
	if( m_channelselect.m_pCurrentChannel == update_channel )
		return;
	m_channelselect.SetActivity( update_channel->name, true );	
}

void CIrcWnd::SetConnectStatus( bool flag )
{
	if(flag)
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
		while( m_channelselect.channelPtrList.GetCount() > 2 )
		{
			Channel* todel = (Channel*)(m_channelselect.channelPtrList).GetTail();
			m_channelselect.RemoveChannel( todel->name );
		}
	}
}

void CIrcWnd::NoticeMessage( CString source, CString message )
{
	bool flag = false;
	Channel* curr_channel = m_channelselect.FindChannelByName( source );
	if( curr_channel )
	{
		AddInfoMessage( source, _T("-%s- %s"), source, message);
		flag = true;
	}
	POSITION pos1, pos2;
	for (pos1 = m_channelselect.channelPtrList.GetHeadPosition();( pos2 = pos1 ) != NULL;)
	{
		m_channelselect.channelPtrList.GetNext(pos1);
		curr_channel = (Channel*)(m_channelselect.channelPtrList).GetAt(pos2);
		Nick* curr_nick = m_nicklist.FindNickByName(curr_channel->name, source );
		if( curr_nick)
		{
			AddInfoMessage( curr_channel->name, _T("-%s- %s"), source, message);
			flag = true;			
		}
	}
	if( flag == false )
	{
		if( m_channelselect.m_pCurrentChannel->type == 4 )
			AddInfoMessage( m_channelselect.m_pCurrentChannel->name, _T("-%s- %s"), source, message);
		else
			AddStatus( _T("-%s- %s"), source, message );
	}
}

//We cannot support color within the text since HyperTextCtrl does not detect hyperlinks with color. So I will filter it.
CString CIrcWnd::StripMessageOfFontCodes( CString temp )
{
	temp = StripMessageOfColorCodes( temp );
	temp.Replace(_T("\002"),_T(""));
	temp.Replace(_T("\003"),_T(""));
	temp.Replace(_T("\017"),_T(""));
	temp.Replace(_T("\026"),_T(""));
	temp.Replace(_T("\037"),_T(""));
	return temp;
}

CString CIrcWnd::StripMessageOfColorCodes( CString temp )
{
	if( !temp.IsEmpty() )
	{
		CString temp1, temp2;
		int test = temp.Find( 3 );
		if( test != -1 )
		{
			int testlength = temp.GetLength() - test;
			if( testlength < 2 )
				return temp;
			temp1 = temp.Left( test );
			temp2 = temp.Mid( test + 2);
			if( testlength < 4 )
				return temp1+temp2;
			if( temp2[0] == 44 && temp2.GetLength() > 2)
			{
				temp2 = temp2.Mid(2);
				for( int I = 48; I < 58; I++ )
				{
					if( temp2[0] == I )
						temp2 = temp2.Mid(1);
				}
			}
			else
			{
				for( int I = 48; I < 58; I++ )
				{
					if( temp2[0] == I )
					{
						temp2 = temp2.Mid(1);
						if( temp2[0] == 44 && temp2.GetLength() > 2)
						{
							temp2 = temp2.Mid(2);
							for( int I = 48; I < 58; I++ )
							{
								if( temp2[0] == I )
									temp2 = temp2.Mid(1);
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

void CIrcWnd::SetTitle( CString channel, CString title )
{
	Channel* curr_channel = m_channelselect.FindChannelByName(channel);
	if(!curr_channel)
		return;
	curr_channel->title = StripMessageOfFontCodes(title);
	if( curr_channel == m_channelselect.m_pCurrentChannel )
		titleWindow.SetWindowText( curr_channel->title );
}

void CIrcWnd::OnBnClickedChatsend()
{
	CString send;
	GetDlgItem(IDC_INPUTWINDOW)->GetWindowText(send);
	GetDlgItem(IDC_INPUTWINDOW)->SetWindowText(_T(""));
	GetDlgItem(IDC_INPUTWINDOW)->SetFocus();
	m_channelselect.Chatsend(send);
}

void CIrcWnd::SendString( CString send )
{ 
	if( this->m_bConnected )
		m_pIrcMain->SendString( send );
}

BOOL CIrcWnd::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.ShowHelp(eMule_FAQ_IRC_Chat);
	return TRUE;
}

void CIrcWnd::OnChatTextChange()
{
	GetDlgItem(IDC_CHATSEND)->EnableWindow( GetDlgItem(IDC_INPUTWINDOW)->GetWindowTextLength()>0 );
}

void CIrcWnd::ParseChangeMode( CString channel, CString changer, CString commands, CString params )
{
	try
	{
		if( commands.GetLength() == 2 )
		{
			//Single mode change..
			if(m_nicklist.m_sUserModeSettings.Find(commands[1]) != -1 )
			{
				//This is a user mode change!
				m_nicklist.ChangeNickMode( channel, params, commands );
			}
			if(m_channelselect.m_sChannelModeSettingsTypeA.Find(commands[1]) != -1 )
			{
				//We do not use these messages yet.. But we can display them for the user to see
				//These modes always have a param and will add or remove a user from some type of list.
				m_channelselect.ChangeChanMode( channel, params, commands );
			}
			if(m_channelselect.m_sChannelModeSettingsTypeB.Find(commands[1]) != -1 )
			{
				//We do not use these messages yet.. But we can display them for the user to see
				//These modes will always have a param..
				m_channelselect.ChangeChanMode( channel, params, commands );
			}
			if(m_channelselect.m_sChannelModeSettingsTypeC.Find(commands[1]) != -1 )
			{
				//We do not use these messages yet.. But we can display them for the user to see
				//These modes will only have a param if your setting it!
				m_channelselect.ChangeChanMode( channel, params, commands );
			}
			if(m_channelselect.m_sChannelModeSettingsTypeD.Find(commands[1]) != -1 )
			{
				//We do not use these messages yet.. But we can display them for the user to see
				//These modes will never have a param for it!
				m_channelselect.ChangeChanMode( channel, params, commands );
			}
			if( !thePrefs.GetIrcIgnoreMiscMessage() )
				AddInfoMessage( channel, GetResString(IDS_IRC_SETSMODE), changer, commands, params);
			return;
		}
		else if ( commands.GetLength() > 2 )
		{
			//Multiple mode changes..
			CString dir;
			dir = commands[0];
			if( dir == _T("+") || dir == _T("-"))
			{
				//The mode must be either adding (+) or removing (-)
				int currModeIndex = 1;
				int currNameIndex = 0;
				int currNameBackIndex = params.Find( _T(" "), currNameIndex);
				CString currName = _T("");
				while( currModeIndex < commands.GetLength())
				{
					//There is another mode to process..
					if(m_nicklist.m_sUserModeSettings.Find(commands[1]) != -1 )
					{
						//This is a user mode change and must have a param!
                        if( currNameBackIndex > currNameIndex )
						{
							//There's a valid name to this mode change..
							currName = params.Mid(currNameIndex, currNameBackIndex-currNameIndex);
							currNameIndex = currNameBackIndex +1;
						}
						else
						{
							//This should not happen!
							ASSERT(0);
						}
						m_nicklist.ChangeNickMode( channel, currName, dir + commands[currModeIndex]);
						//Move to the next param.
						currNameBackIndex = params.Find(_T(" "), currNameIndex+1);
						if( currNameBackIndex == -1)
							currNameBackIndex = params.GetLength();
					}
					if(m_channelselect.m_sChannelModeSettingsTypeA.Find(commands[currModeIndex]) != -1)
					{
						//We do not use these messages yet.. But we can display them for the user to see
						//These modes always have a param and will add or remove a user from some type of list.
                        if( currNameBackIndex > currNameIndex )
						{
							//There's a valid name to this mode change..
							currName = params.Mid(currNameIndex, currNameBackIndex-currNameIndex);
							currNameIndex = currNameBackIndex +1;
						}
						else
						{
							//This should not happen!
							ASSERT(0);
						}

						m_channelselect.ChangeChanMode( channel, currName, dir + commands[currModeIndex]);

						//Move to the next param.
						currNameBackIndex = params.Find(_T(" "), currNameIndex+1);
						if( currNameBackIndex == -1)
							currNameBackIndex = params.GetLength();
					}
					if(m_channelselect.m_sChannelModeSettingsTypeB.Find(commands[currModeIndex]) != -1)
					{
						//We do not use these messages yet.. But we can display them for the user to see
						//These modes will always have a param..
                        if( currNameBackIndex > currNameIndex )
						{
							//There's a valid name to this mode change..
							currName = params.Mid(currNameIndex, currNameBackIndex-currNameIndex);
							currNameIndex = currNameBackIndex +1;
						}
						else
						{
							//This should not happen!
							ASSERT(0);
						}

						m_channelselect.ChangeChanMode( channel, currName, dir + commands[currModeIndex]);

						//Move to the next param.
						currNameBackIndex = params.Find(_T(" "), currNameIndex+1);
						if( currNameBackIndex == -1)
							currNameBackIndex = params.GetLength();
					}
					if(m_channelselect.m_sChannelModeSettingsTypeC.Find(commands[currModeIndex]) != -1 )
					{
						//We do not use these messages yet.. But we can display them for the user to see
						//These modes will only have a param if your setting it!
						if( dir == _T("+") )
						{
							//We are setting a mode, find param
	                        if( currNameBackIndex > currNameIndex )
							{
								//There's a valid name to this mode change..
								currName = params.Mid(currNameIndex, currNameBackIndex-currNameIndex);
								currNameIndex = currNameBackIndex +1;
							}
							else
							{
								//This should not happen!
								ASSERT(0);
							}
						}
						else
						{
							//We are removing a mode, no params
							currName = _T("");
						}

						m_channelselect.ChangeChanMode( channel, currName, dir + commands[currModeIndex]);

						if( dir == _T("+") )
						{
							//Set this mode, so move to the next param.
							currNameBackIndex = params.Find(_T(" "), currNameIndex+1);
							if( currNameBackIndex == -1)
								currNameBackIndex = params.GetLength();
						}
					}
					if(m_channelselect.m_sChannelModeSettingsTypeD.Find(commands[currModeIndex]) != -1 )
					{
						//We do not use these messages yet.. But we can display them for the user to see
						//These modes will never have a param for it!
						currName = _T("");

						m_channelselect.ChangeChanMode( channel, currName, dir + commands[currModeIndex]);

					}
					currModeIndex++;
				}
				if( !thePrefs.GetIrcIgnoreMiscMessage() )
					AddInfoMessage( channel, GetResString(IDS_IRC_SETSMODE), changer, commands, params);
			}
		}
	}
	catch(...)
	{
		AddInfoMessage( channel, GetResString(IDS_IRC_NOTSUPPORTED));
		ASSERT(0);
	}
}

LRESULT CIrcWnd::OnCloseTab(WPARAM wparam, LPARAM lparam) {

	OnBnClickedClosechat( (int)wparam );

	return TRUE;
}

LRESULT CIrcWnd::OnQueryTab(WPARAM wParam, LPARAM lParam)
{
	int nItem = (int)wParam;

	TCITEM item;
	item.mask = TCIF_PARAM;
	m_channelselect.GetItem(nItem, &item);
	Channel* partChannel = (Channel*)item.lParam;
	if (partChannel)
	{
		if (partChannel->type == 4 && m_bConnected)
		{
			return 0;
		}
		else if (partChannel->type == 5 || partChannel->type == 4)
		{
			return 0;
		}
	}
	return 1;
}
