#include "StdAfx.h"
#include <Mmsystem.h>
#include "ircchanneltabctrl.h"
#include "ircwnd.h"
#include "ircmain.h"
#include "emuledlg.h"
#include "OtherFunctions.h"
#include "MenuCmds.h"
#include "emule.h"
#include "HTRichEditCtrl.h"

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

IMPLEMENT_DYNAMIC(CIrcChannelTabCtrl, CClosableTabCtrl)

BEGIN_MESSAGE_MAP(CIrcChannelTabCtrl, CClosableTabCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONUP()
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnTcnSelchangeTab2)
END_MESSAGE_MAP()

CIrcChannelTabCtrl::CIrcChannelTabCtrl()
{
	m_pCurrentChannel = NULL;
	m_pParent = NULL;
	m_bCloseable = true;
	memset(&m_iiCloseButton, 0, sizeof m_iiCloseButton);
	m_ptCtxMenu.SetPoint(-1, -1);
}

CIrcChannelTabCtrl::~CIrcChannelTabCtrl()
{
	//Remove and delete all our open channels.
	DeleteAllChannel();
}

void CIrcChannelTabCtrl::Init()
{
	//This adds the two static windows, Status and ChanneList
	NewChannel( GetResString(IDS_STATUS), 1 );
	NewChannel( GetResString(IDS_IRC_CHANNELLIST), 2);
	//Initialize the IRC window to be in the ChannelList
	m_pCurrentChannel = (Channel*)channelPtrList.GetTail();
	SetCurSel(0);
	OnTcnSelchangeTab2( NULL, NULL );
	SetAllIcons();
}

void CIrcChannelTabCtrl::OnSysColorChange()
{ 
	CClosableTabCtrl::OnSysColorChange();
	SetAllIcons();
}

void CIrcChannelTabCtrl::SetAllIcons()
{
	CImageList iml;
	iml.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
	iml.Add(CTempIconLoader(_T("Chat")));
	iml.Add(CTempIconLoader(_T("Message")));
	iml.Add(CTempIconLoader(_T("MessagePending")));
	SetImageList(&iml);
	m_imlIRC.DeleteImageList();
	m_imlIRC.Attach(iml.Detach());
	SetPadding(CSize(10, 0));
}

void CIrcChannelTabCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{ 
	int iCurTab = GetTabUnderMouse(point);
	if (iCurTab < 2)
	{
		return;
	}
	TCITEM item;
	item.mask = TCIF_PARAM;
	GetItem(iCurTab, &item);

	Channel* chan = FindChannelByName(((Channel*)item.lParam)->name);
	if( !chan )
	{
		return;
	}

	CTitleMenu ChatMenu;
	ChatMenu.CreatePopupMenu();
	ChatMenu.AddMenuTitle(GetResString(IDS_IRC)+_T(" : ")+chan->name);
	ChatMenu.AppendMenu(MF_STRING, Irc_Close, GetResString(IDS_FD_CLOSE));
	if (iCurTab < 2) // no 'Close' for status log and channel list
		ChatMenu.EnableMenuItem(Irc_Close, MF_GRAYED);

	int totallength = m_sChannelModeSettingsTypeA.GetLength()+m_sChannelModeSettingsTypeB.GetLength()+m_sChannelModeSettingsTypeC.GetLength()+m_sChannelModeSettingsTypeD.GetLength();
	int index = 0;
	if( totallength > index )
	{
		int length = m_sChannelModeSettingsTypeA.GetLength();
		for( int i = 0; i < length; i++)
		{
			CString mode = m_sChannelModeSettingsTypeA.Mid(i,1);
			if( chan->modesA.Find(mode[0]) == -1 )
			{
				ChatMenu.AppendMenu(MF_STRING, Irc_ChanCommands+index, _T("Set +") + mode + _T(" ( Not Supported Yet )") );
				ChatMenu.EnableMenuItem(Irc_ChanCommands+index, MF_GRAYED);
			}
			else
			{
				ChatMenu.AppendMenu(MF_STRING, Irc_ChanCommands+index+50, _T("Set -") + mode + _T(" ( Not Supported Yet )") );
				ChatMenu.EnableMenuItem(Irc_ChanCommands+index+50, MF_GRAYED);
			}
			index++;
		}
		length = m_sChannelModeSettingsTypeB.GetLength();
		for( int i = 0; i < length; i++)
		{
			CString mode = m_sChannelModeSettingsTypeB.Mid(i,1);
			if( chan->modesB.Find(mode[0]) == -1 )
			{
				ChatMenu.AppendMenu(MF_STRING, Irc_ChanCommands+index, _T("Set +") + mode + _T(" ( Not Supported Yet )") );
				ChatMenu.EnableMenuItem(Irc_ChanCommands+index, MF_GRAYED);
			}
			else
			{
				ChatMenu.AppendMenu(MF_STRING, Irc_ChanCommands+index+50, _T("Set -") + mode + _T(" ( Not Supported Yet )") );
				ChatMenu.EnableMenuItem(Irc_ChanCommands+index+50, MF_GRAYED);
			}
			index++;
		}
		length = m_sChannelModeSettingsTypeC.GetLength();
		for( int i = 0; i < length; i++)
		{
			CString mode = m_sChannelModeSettingsTypeC.Mid(i,1);
			if( chan->modesC.Find(mode[0]) == -1 )
			{
				ChatMenu.AppendMenu(MF_STRING, Irc_ChanCommands+index, _T("Set +") + mode + _T(" ( Not Supported Yet )") );
				ChatMenu.EnableMenuItem(Irc_ChanCommands+index, MF_GRAYED);
			}
			else
			{
				ChatMenu.AppendMenu(MF_STRING, Irc_ChanCommands+index+50, _T("Set -") + mode + _T(" ( Remove ") + mode + _T(" )") );
			}
			index++;
		}
		length = m_sChannelModeSettingsTypeD.GetLength();
		for( int i = 0; i < length; i++)
		{
			CString mode = m_sChannelModeSettingsTypeD.Mid(i,1);
			if( chan->modesD.Find(mode[0]) == -1 )
			{
				ChatMenu.AppendMenu(MF_STRING, Irc_ChanCommands+index, _T("Set +") + mode + _T(" ( Add ") + mode + _T(" )") );
			}
			else
			{
				ChatMenu.AppendMenu(MF_STRING, Irc_ChanCommands+index+50, _T("Set -") + mode + _T(" ( Remove ") + mode + _T(" )") );
			}
			index++;
		}
	}

	ChatMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	VERIFY( ChatMenu.DestroyMenu() );
}

int CIrcChannelTabCtrl::GetTabUnderMouse(CPoint point) 
{
	TCHITTESTINFO hitinfo;
	CRect rect;
	GetWindowRect(&rect);
	point.Offset(0-rect.left,0-rect.top);
	hitinfo.pt = point;

	if( GetItemRect( 0, &rect ) )
		if (hitinfo.pt.y< rect.top+30 && hitinfo.pt.y >rect.top-30)
			hitinfo.pt.y = rect.top;

	// Find the destination tab...
	unsigned int nTab = HitTest( &hitinfo );
	if( hitinfo.flags != TCHT_NOWHERE )
		return nTab;
	else
		return -1;
}

Channel* CIrcChannelTabCtrl::FindChannelByName(CString name)
{
	POSITION pos1, pos2;
	for (pos1 = channelPtrList.GetHeadPosition();( pos2 = pos1 ) != NULL;)
	{
		channelPtrList.GetNext(pos1);
		Channel* cur_channel = (Channel*)channelPtrList.GetAt(pos2);
		if (cur_channel->name.CompareNoCase(name.Trim()) == 0 && (cur_channel->type == 4 || cur_channel->type == 5))
			return cur_channel;
	}
	return 0;
}

Channel* CIrcChannelTabCtrl::NewChannel( CString name, uint8 type )
{
	Channel* toadd = new Channel;
	toadd->name = name;
	toadd->title = name;
	toadd->type = type;
	toadd->history_pos = 0;
	if (type != 2)
	{
		CRect rcChannel;
		m_pParent->m_serverChannelList.GetWindowRect(&rcChannel);
		toadd->log.Create(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | ES_MULTILINE | ES_READONLY, rcChannel, m_pParent, (UINT)-1);
		toadd->log.ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_FRAMECHANGED);
		toadd->log.SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
		toadd->log.SetEventMask(toadd->log.GetEventMask() | ENM_LINK);
		toadd->log.SetFont(&theApp.emuledlg->m_fontHyperText);
		toadd->log.SetTitle(name);
		toadd->log.SetProfileSkinKey(_T("IRCChannel"));
		toadd->log.ApplySkin();
	}
	channelPtrList.AddTail(toadd);
	
	TCITEM newitem;
	newitem.mask = TCIF_PARAM|TCIF_TEXT|TCIF_IMAGE;

	newitem.lParam = (LPARAM)toadd;
	newitem.pszText = name.GetBuffer();
	newitem.iImage = 1; // 'Message'
	uint32 pos = GetItemCount();
	InsertItem(pos,&newitem);
	if(type == 4)
	{
		SetCurSel(pos);
		SetCurFocus(pos);
		OnTcnSelchangeTab2( NULL, NULL );
	}
	return toadd;
}

void CIrcChannelTabCtrl::RemoveChannel( CString channel )
{
	Channel* todel = FindChannelByName( channel );
	if( !todel )
		return;
	TCITEM item;
	item.mask = TCIF_PARAM;
	item.lParam = -1;
	int i;
	for (i = 0; i < GetItemCount();i++)
	{
		GetItem(i,&item);
		if (((Channel*)item.lParam) == todel)
			break;
	}
	if (((Channel*)item.lParam) != todel)
		return;
	DeleteItem(i);

	if( todel == m_pCurrentChannel )
	{
		m_pParent->m_nicklist.DeleteAllItems();
		if( GetItemCount() > 2 && i > 1 ) 
		{
			if ( i == 2 )
				i++;
			SetCurSel(i-1);
			SetCurFocus(i-1);
			OnTcnSelchangeTab2( NULL, NULL );
		}
		else 
		{
			SetCurSel(0);
			SetCurFocus(0);
			OnTcnSelchangeTab2( NULL, NULL );
		}
	}
	m_pParent->m_nicklist.DeleteAllNick(todel->name);
	channelPtrList.RemoveAt(channelPtrList.Find(todel));
	delete todel;
}

void CIrcChannelTabCtrl::DeleteAllChannel()
{
	POSITION pos1, pos2;
	for (pos1 = channelPtrList.GetHeadPosition();( pos2 = pos1 ) != NULL;)
	{
		channelPtrList.GetNext(pos1);
		Channel* cur_channel =	(Channel*)channelPtrList.GetAt(pos2);
		m_pParent->m_nicklist.DeleteAllNick(cur_channel->name);
		channelPtrList.RemoveAt(pos2);
		delete cur_channel;
	}
}


bool CIrcChannelTabCtrl::ChangeChanMode( CString channel, CString nick, CString mode )
{
	if( channel.Left(1) != _T("#") )
	{
		//Not a channel, this shouldn't happen
		return true;
	}
	//We are changing a mode to something..
	Channel* update = FindChannelByName( channel );
	if( !update )
	{
		//No channel found, this shouldn't happen.
		return false;
	}
	//Update modes.
	int modeLevel = m_sChannelModeSettingsTypeA.Find(mode[1]);
	if( modeLevel != -1 )
	{
		//Remove the setting. This takes care of "-" and makes sure we don't add the same symbol twice.
		update->modesA.Remove(mode[1]);
		if( mode.Left(1) == _T("+") )
		{
			if( update->modesA == _T("") )
			{
				//Channel has no modes, just set it.. 
				update->modesA = mode[1];
			}
			else
			{
				//The chan does have other modes.. Lets make sure we put things in order..
				int index = 0;
				//This will pad the mode string..
				while( index < m_sChannelModeSettingsTypeA.GetLength() )
				{
					if( update->modesA.Find(m_sChannelModeSettingsTypeA[index]) == -1 )
						update->modesA.Insert(index, _T(" "));
					index++;
				}
				//Insert the new mode
				update->modesA.Insert(modeLevel, mode[1]);
				//Remove pads
				update->modesA.Remove(_T(' '));
			}
		}
		return true;
	}
	modeLevel = m_sChannelModeSettingsTypeB.Find(mode[1]);
	if( modeLevel != -1 )
	{
		//Remove the setting. This takes care of "-" and makes sure we don't add the same symbol twice.
		update->modesB.Remove(mode[1]);
		if( mode.Left(1) == _T("+") )
		{
			if( update->modesB == _T("") )
			{
				//Channel has no modes, just set it.. 
				update->modesB = mode[1];
			}
			else
			{
				//The chan does have other modes.. Lets make sure we put things in order..
				int index = 0;
				//This will pad the mode string..
				while( index < m_sChannelModeSettingsTypeB.GetLength() )
				{
					if( update->modesB.Find(m_sChannelModeSettingsTypeB[index]) == -1 )
						update->modesB.Insert(index, _T(" "));
					index++;
				}
				//Insert the new mode
				update->modesB.Insert(modeLevel, mode[1]);
				//Remove pads
				update->modesB.Remove(_T(' '));
			}
		}
		return true;
	}
	modeLevel = m_sChannelModeSettingsTypeC.Find(mode[1]);
	if( modeLevel != -1 )
	{
		//Remove the setting. This takes care of "-" and makes sure we don't add the same symbol twice.
		update->modesC.Remove(mode[1]);
		if( mode.Left(1) == _T("+") )
		{
			if( update->modesC == _T("") )
			{
				//Channel has no modes, just set it.. 
				update->modesC = mode[1];
			}
			else
			{
				//The chan does have other modes.. Lets make sure we put things in order..
				int index = 0;
				//This will pad the mode string..
				while( index < m_sChannelModeSettingsTypeC.GetLength() )
				{
					if( update->modesC.Find(m_sChannelModeSettingsTypeC[index]) == -1 )
						update->modesC.Insert(index, _T(" "));
					index++;
				}
				//Insert the new mode
				update->modesC.Insert(modeLevel, mode[1]);
				//Remove pads
				update->modesC.Remove(_T(' '));
			}
		}
		return true;
	}
	modeLevel = m_sChannelModeSettingsTypeD.Find(mode[1]);
	if( modeLevel != -1 )
	{
		//Remove the setting. This takes care of "-" and makes sure we don't add the same symbol twice.
		update->modesD.Remove(mode[1]);
		if( mode.Left(1) == _T("+") )
		{
			if( update->modesD == _T("") )
			{
				//Channel has no modes, just set it.. 
				update->modesD = mode[1];
			}
			else
			{
				//The chan does have other modes.. Lets make sure we put things in order..
				int index = 0;
				//This will pad the mode string..
				while( index < m_sChannelModeSettingsTypeD.GetLength() )
				{
					if( update->modesD.Find(m_sChannelModeSettingsTypeD[index]) == -1 )
						update->modesD.Insert(index, _T(" "));
					index++;
				}
				//Insert the new mode
				update->modesD.Insert(modeLevel, mode[1]);
				//Remove pads
				update->modesD.Remove(_T(' '));
			}
		}
		return true;
	}
	return false;
}

void CIrcChannelTabCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bCloseable)
	{
		int iTabs = GetItemCount();
		for (int i = 0; i < iTabs; i++)
		{
			CRect rcItem;
			GetItemRect(i, rcItem);
			CRect rcCloseButton;
			GetCloseButtonRect(rcItem, rcCloseButton);
			rcCloseButton.top -= 2;
			rcCloseButton.left -= 4;
			rcCloseButton.right += 2;
			rcCloseButton.bottom += 4;
			if (rcCloseButton.PtInRect(point))
			{
				m_pParent->OnBnClickedClosechat( i );
				return; 
			}
		}
	}
	
	CTabCtrl::OnLButtonDown(nFlags, point);
}

void CIrcChannelTabCtrl::OnTcnSelchangeTab2(NMHDR *pNMHDR, LRESULT *pResult)
{
	TCITEM item;
	item.mask = TCIF_PARAM;
	//What channel did we select?
	int cur_sel = GetCurSel();
	if (cur_sel == -1)
	{
		//No channel, abort..
		return;
	}
	if (!GetItem(cur_sel, &item))
	{
		//We had no valid item here.. Something isn't right..
		//TODO: this should never happen, so maybe we should remove this tab?
		return;
	}
	Channel* update = (Channel*)item.lParam;

	//Set our current channel to the new one for quick reference..
	m_pCurrentChannel = update;

	if (m_pCurrentChannel->type == 1)
		m_pParent->titleWindow.SetWindowText(GetResString(IDS_STATUS));
	if (m_pCurrentChannel->type == 2)
	{
		//Since some channels can have a LOT of nicks, hide the window then remove them to speed it up..
		m_pParent->m_nicklist.ShowWindow(SW_HIDE);
		m_pParent->m_nicklist.DeleteAllItems();
		m_pParent->m_nicklist.UpdateNickCount();
		m_pParent->m_nicklist.ShowWindow(SW_SHOW);
		//Set title to ChanList
		m_pParent->titleWindow.SetWindowText(GetResString(IDS_IRC_CHANNELLIST));
		//Show our ChanList..
		m_pParent->m_serverChannelList.ShowWindow(SW_SHOW);
		TCITEM tci;
		tci.mask = TCIF_PARAM;
		//Go through the channel tabs and hide the channels..
		//Maybe overkill? Maybe just remember our previous channel and hide it?
		int i = 0;
		while (GetItem(i++, &tci))
		{
			Channel* ch2 = (Channel*)tci.lParam;
			if (ch2 != m_pCurrentChannel && ch2->log.m_hWnd != NULL)
				ch2->log.ShowWindow(SW_HIDE);
		}
		return;
	}
	//We entered the channel, set activity flag off.
	SetActivity( m_pCurrentChannel->name, false );
	CRect rcChannel;
	m_pParent->m_serverChannelList.GetWindowRect(&rcChannel);
	m_pParent->ScreenToClient(&rcChannel);
	//Show new current channel..
	m_pCurrentChannel->log.SetWindowPos(NULL, rcChannel.left, rcChannel.top, rcChannel.Width(), rcChannel.Height(), SWP_NOZORDER);
	m_pCurrentChannel->log.ShowWindow(SW_SHOW);
	TCITEM tci;
	tci.mask = TCIF_PARAM;
	//Hide all channels not in focus..
	//Maybe overkill? Maybe remember previous channel and hide?
	int i = 0;
	while (GetItem(i++, &tci))
	{
		Channel* ch2 = (Channel*)tci.lParam;
		if (ch2 != m_pCurrentChannel && ch2->log.m_hWnd != NULL)
			ch2->log.ShowWindow(SW_HIDE);
	}
	//Make sure channelList is hidden.
	m_pParent->m_serverChannelList.ShowWindow(SW_HIDE);
	//Update nicklist to the new channel..
	m_pParent->m_nicklist.RefreshNickList( update->name );
	//Set title to the new channel..
	m_pParent->SetTitle( update->name, update->title );
	//Push focus back to the input box..
	m_pParent->GetDlgItem(IDC_INPUTWINDOW)->SetFocus();
	if( pResult )
		*pResult = 0;
}

void CIrcChannelTabCtrl::ScrollHistory(bool down) 
{
	CString buffer;

	if ( (m_pCurrentChannel->history_pos==0 && !down) || (m_pCurrentChannel->history_pos==m_pCurrentChannel->history.GetCount() && down))
		return;
	
	if (down)
		++m_pCurrentChannel->history_pos;
	else
		--m_pCurrentChannel->history_pos;

	buffer= (m_pCurrentChannel->history_pos==m_pCurrentChannel->history.GetCount()) ? _T("") : m_pCurrentChannel->history.GetAt(m_pCurrentChannel->history_pos);

	m_pParent->GetDlgItem(IDC_INPUTWINDOW)->SetWindowText(buffer);
	m_pParent->inputWindow.SetSel(buffer.GetLength(),buffer.GetLength());
}

void CIrcChannelTabCtrl::SetActivity( CString channel, bool flag)
{
	Channel* refresh = FindChannelByName( channel );
	if( !refresh )
	{
		refresh = (Channel*)channelPtrList.GetHead();
		if( !refresh )
			return;
	}
	TCITEM item;
	item.mask = TCIF_PARAM;
	item.lParam = -1;
	int i;
	for (i = 0; i < GetItemCount();i++)
	{
		GetItem(i,&item);
		if (((Channel*)item.lParam) == refresh)
			break;
	}
	if (((Channel*)item.lParam) != refresh)
		return;
    if( flag )
	{
		item.mask = TCIF_IMAGE;
		item.iImage = 2; // 'MessagePending'
		SetItem( i, &item );
		HighlightItem(i, TRUE);
    }
	else
	{
		item.mask = TCIF_IMAGE;
 		item.iImage = 1; // 'Message'
		SetItem( i, &item );
		HighlightItem(i, FALSE);
    }
}


void CIrcChannelTabCtrl::Chatsend( CString send )
{
	if (m_pCurrentChannel->history.GetCount()==thePrefs.GetMaxChatHistoryLines()) 
		m_pCurrentChannel->history.RemoveAt(0);
	m_pCurrentChannel->history.Add(send);
	m_pCurrentChannel->history_pos=m_pCurrentChannel->history.GetCount();

	if( send.IsEmpty() )
		return;

	if( !m_pParent->IsConnected() )
		return;
	if( send.Left(4) == _T("/hop") )
	{
		if( m_pCurrentChannel->name.Left(1) == _T("#") )
		{
			CString channel = m_pCurrentChannel->name;
			m_pParent->m_pIrcMain->SendString( _T("PART ") + channel );
			m_pParent->m_pIrcMain->SendString( _T("JOIN ") + channel );
			return;
		}
		return;
	}
	if( send.Left(1) == _T("/") && send.Left(3).MakeLower() != _T("/me") && send.Left(6).MakeLower() != _T("/sound") )
	{
		if (send.Left(4) == _T("/msg"))
		{
			if( m_pCurrentChannel->type == 4 || m_pCurrentChannel->type == 5)
			{
				send.Replace( _T("%"), _T("\004") );
				m_pParent->AddInfoMessage( m_pCurrentChannel->name ,CString(_T("* >> "))+send.Mid(5));
				send.Replace( _T("\004"), _T("%") );
			}
			else
			{
				send.Replace( _T("%"), _T("\004") );
				m_pParent->AddStatus( CString(_T("* >> "))+send.Mid(5));
				send.Replace( _T("\004"), _T("%") );
			}
			send = CString(_T("/PRIVMSG")) + send.Mid(4);
		}
		if( ((CString)send.Left(17)).CompareNoCase( _T("/PRIVMSG nickserv")  )== 0)
		{
			send = CString(_T("/ns")) + send.Mid(17);
		}
		else if( ((CString)send.Left(17)).CompareNoCase( _T("/PRIVMSG chanserv") )== 0)
		{
			send = CString(_T("/cs")) + send.Mid(17);
		}
		else if( ((CString)send.Left(8)).CompareNoCase( _T("/PRIVMSG") )== 0)
		{
			int index = send.Find(_T(" "), send.Find(_T(" "))+1);
			send.Insert(index+1, _T(":"));
		}
		else if( ((CString)send.Left(6)).CompareNoCase( _T("/TOPIC") )== 0)
		{
			int index = send.Find(_T(" "), send.Find(_T(" "))+1);
			send.Insert(index+1, _T(":"));
		}
		m_pParent->m_pIrcMain->SendString(send.Mid(1));
		return;
	}
	if( m_pCurrentChannel->type < 4 )
	{
		m_pParent->m_pIrcMain->SendString(send);
		return;
	}
	if( send.Left(3) == _T("/me") )
	{
		CString build;
		build.Format( _T("PRIVMSG %s :\001ACTION %s\001"), m_pCurrentChannel->name, send.Mid(4) );
		send.Replace( _T("%"), _T("\004") );
		m_pParent->AddInfoMessage( m_pCurrentChannel->name, _T("* %s %s"), m_pParent->m_pIrcMain->GetNick(), send.Mid(4));
		m_pParent->m_pIrcMain->SendString(build);
		return;
	}
	if( send.Left(6) == _T("/sound") )
	{
	   CString build, sound;
	   build.Format( _T("PRIVMSG %s :\001SOUND %s\001"), m_pCurrentChannel->name, send.Mid(7) );
	   m_pParent->m_pIrcMain->SendString(build);
	   send = send.Mid(7);
	   int soundlen = send.Find( _T(" ") );
	   send.Replace( _T("%"), _T("\004") );
	   if( soundlen != -1 )
	   {
		   build = send.Left(soundlen);
		   build.Replace(_T("\\"),_T(""));
		   send = send.Left(soundlen);
	   }
	   else
	   {
		   build = send;
		   send = _T("[SOUND]");
	   }
   	   sound.Format(_T("%sSounds\\IRC\\%s"), thePrefs.GetAppDir(), build);
	   m_pParent->AddInfoMessage( m_pCurrentChannel->name, _T("* %s %s"), m_pParent->m_pIrcMain->GetNick(), send);
	   PlaySound(sound, NULL, SND_FILENAME | SND_NOSTOP | SND_NOWAIT | SND_ASYNC);
	   return;
	}
	CString build = _T("PRIVMSG ") + m_pCurrentChannel->name + _T(" :") + send;
	m_pParent->m_pIrcMain->SendString(build);
	send.Replace( _T("%"), _T("\004") );
	m_pParent->AddMessage( m_pCurrentChannel->name, m_pParent->m_pIrcMain->GetNick(), send );
	send.Replace( _T("\004"), _T("%") );
}

void CIrcChannelTabCtrl::Localize()
{
	for (int i = 0; i < GetItemCount();i++)
	{
		TCITEM item;
		item.mask = TCIF_PARAM;
		item.lParam = -1;
		GetItem(i,&item);
		Channel* cur_chan = (Channel*)item.lParam;
		if (cur_chan != NULL)
		{
			if( cur_chan->type == 1 )
			{
				cur_chan->title = GetResString(IDS_STATUS);
				item.mask = TCIF_TEXT;
				item.pszText = cur_chan->title.GetBuffer();
				SetItem(i,&item);
				cur_chan->title.ReleaseBuffer();
			}
			if( cur_chan->type == 2 )
			{
				cur_chan->title = GetResString(IDS_IRC_CHANNELLIST);
				item.mask = TCIF_TEXT;
				item.pszText = cur_chan->title.GetBuffer();
				SetItem(i,&item);
				cur_chan->title.ReleaseBuffer();
			}
		}
	}
	if (m_pCurrentChannel)
	{
		if( m_pCurrentChannel->type == 1 )
			m_pParent->titleWindow.SetWindowText(GetResString(IDS_STATUS));
		if( m_pCurrentChannel->type == 2 )
			m_pParent->titleWindow.SetWindowText(GetResString(IDS_IRC_CHANNELLIST));
	}
	SetAllIcons();
}

BOOL CIrcChannelTabCtrl::OnCommand(WPARAM wParam,LPARAM lParam )
{
	int chanItem = m_pParent->m_channelselect.GetCurSel(); 
	switch( wParam )
	{
		case Irc_Close:
		{
			//Pressed the close button
			m_pParent->OnBnClickedClosechat();
			return true;
		}
	}
	uint32 test = Irc_ChanCommands;
	if( wParam >= test && wParam < test+50)
	{
		int index = wParam - test;
		if( index < m_sChannelModeSettingsTypeA.GetLength() )
		{
			CString mode = m_sChannelModeSettingsTypeA.Mid(index,1);
			TCITEM item;
			item.mask = TCIF_PARAM;
			GetItem(chanItem,&item);
			Channel* chan = (Channel*)item.lParam;
			if( chan )
			{
				//We have a chan, send the command.
				CString send;
				send.Format( _T("MODE %s +%s %s"), chan->name, mode );
				m_pParent->m_pIrcMain->SendString(send);
			}
			return true;
		}
		if( index < m_sChannelModeSettingsTypeA.GetLength()+m_sChannelModeSettingsTypeB.GetLength() )
		{
			CString mode = m_sChannelModeSettingsTypeB.Mid(index-m_sChannelModeSettingsTypeA.GetLength(),1);
			TCITEM item;
			item.mask = TCIF_PARAM;
			GetItem(chanItem,&item);
			Channel* chan = (Channel*)item.lParam;
			if( chan )
			{
				//We have a chan, send the command.
				CString send;
				send.Format( _T("MODE %s +%s %s"), chan->name, mode );
				m_pParent->m_pIrcMain->SendString(send);
			}
			return true;
		}
		if( index < m_sChannelModeSettingsTypeA.GetLength()+m_sChannelModeSettingsTypeB.GetLength()+m_sChannelModeSettingsTypeC.GetLength() )
		{
			CString mode = m_sChannelModeSettingsTypeC.Mid(index-m_sChannelModeSettingsTypeA.GetLength()-m_sChannelModeSettingsTypeB.GetLength(),1);
			TCITEM item;
			item.mask = TCIF_PARAM;
			GetItem(chanItem,&item);
			Channel* chan = (Channel*)item.lParam;
			if( chan )
			{
				//We have a chan, send the command.
				CString send;
				send.Format( _T("MODE %s +%s %s"), chan->name, mode );
				m_pParent->m_pIrcMain->SendString(send);
			}
			return true;
		}
		if( index < m_sChannelModeSettingsTypeA.GetLength()+m_sChannelModeSettingsTypeB.GetLength()+m_sChannelModeSettingsTypeC.GetLength()+m_sChannelModeSettingsTypeD.GetLength() )
		{
			CString mode = m_sChannelModeSettingsTypeD.Mid(index-m_sChannelModeSettingsTypeA.GetLength()-m_sChannelModeSettingsTypeB.GetLength()-m_sChannelModeSettingsTypeC.GetLength(),1);
			TCITEM item;
			item.mask = TCIF_PARAM;
			GetItem(chanItem,&item);
			Channel* chan = (Channel*)item.lParam;
			if( chan )
			{
				//We have a chan, send the command.
				CString send;
				send.Format( _T("MODE %s +%s %s"), chan->name, mode );
				m_pParent->m_pIrcMain->SendString(send);
			}
			return true;
		}
		return true;
	}
	if( wParam >= test+50 && wParam < test+100)
	{
		int index = wParam - test - 50;
		if( index < m_sChannelModeSettingsTypeA.GetLength() )
		{
			CString mode = m_sChannelModeSettingsTypeA.Mid(index,1);
			TCITEM item;
			item.mask = TCIF_PARAM;
			GetItem(chanItem,&item);
			Channel* chan = (Channel*)item.lParam;
			if( chan )
			{
				//We have a chan, send the command.
				CString send;
				send.Format( _T("MODE %s -%s %s"), chan->name, mode );
				m_pParent->m_pIrcMain->SendString(send);
			}
			return true;
		}
		if( index < m_sChannelModeSettingsTypeA.GetLength()+m_sChannelModeSettingsTypeB.GetLength() )
		{
			CString mode = m_sChannelModeSettingsTypeB.Mid(index-m_sChannelModeSettingsTypeA.GetLength(),1);
			TCITEM item;
			item.mask = TCIF_PARAM;
			GetItem(chanItem,&item);
			Channel* chan = (Channel*)item.lParam;
			if( chan )
			{
				//We have a chan, send the command.
				CString send;
				send.Format( _T("MODE %s -%s %s"), chan->name, mode );
				m_pParent->m_pIrcMain->SendString(send);
			}
			return true;
		}
		if( index < m_sChannelModeSettingsTypeA.GetLength()+m_sChannelModeSettingsTypeB.GetLength()+m_sChannelModeSettingsTypeC.GetLength() )
		{
			CString mode = m_sChannelModeSettingsTypeC.Mid(index-m_sChannelModeSettingsTypeA.GetLength()-m_sChannelModeSettingsTypeB.GetLength(),1);
			TCITEM item;
			item.mask = TCIF_PARAM;
			GetItem(chanItem,&item);
			Channel* chan = (Channel*)item.lParam;
			if( chan )
			{
				//We have a chan, send the command.
				CString send;
				send.Format( _T("MODE %s -%s %s"), chan->name, mode );
				m_pParent->m_pIrcMain->SendString(send);
			}
			return true;
		}
		if( index < m_sChannelModeSettingsTypeA.GetLength()+m_sChannelModeSettingsTypeB.GetLength()+m_sChannelModeSettingsTypeC.GetLength()+m_sChannelModeSettingsTypeD.GetLength() )
		{
			CString mode = m_sChannelModeSettingsTypeD.Mid(index-m_sChannelModeSettingsTypeA.GetLength()-m_sChannelModeSettingsTypeB.GetLength()-m_sChannelModeSettingsTypeC.GetLength(),1);
			TCITEM item;
			item.mask = TCIF_PARAM;
			GetItem(chanItem,&item);
			Channel* chan = (Channel*)item.lParam;
			if( chan )
			{
				//We have a chan, send the command.
				CString send;
				send.Format( _T("MODE %s -%s %s"), chan->name, mode );
				m_pParent->m_pIrcMain->SendString(send);
			}
			return true;
		}
		return true;
	}
	return true;
}