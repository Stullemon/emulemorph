#include "StdAfx.h"
#include "ircchannellistctrl.h"
#include "emuledlg.h"
#include "emule.h"
#include "OtherFunctions.h"
#include "MenuCmds.h"
#include "ircwnd.h"
#include "ircmain.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


struct ChannelList
{
	CString name;
	CString users;
	CString desc;
};

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

CIrcChannelListCtrl::~CIrcChannelListCtrl()
{
	//Remove and delete serverChannelList.
	ResetServerChannelList(true);
}

int CIrcChannelListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	ChannelList* item1 = (ChannelList*)lParam1;
	ChannelList* item2 = (ChannelList*)lParam2;
	switch(lParamSort)
	{
		case 0: 
			return item1->name.CompareNoCase(item2->name);
		case 10:
			return item2->name.CompareNoCase(item1->name);
		case 1: 
			return _tstoi(item1->users) - _tstoi(item2->users);
		case 11:
			return _tstoi(item2->users) - _tstoi(item1->users);
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
	ChanLMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this); 
	VERIFY( ChanLMenu.DestroyMenu() );
}

void CIrcChannelListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	JoinChannels();
	*pResult = 0;
}

void CIrcChannelListCtrl::ResetServerChannelList( bool b_shutdown )
{
	//Delete our ServerChannelList..
	POSITION pos1, pos2;
	for (pos1 = channelLPtrList.GetHeadPosition();( pos2 = pos1 ) != NULL;)
	{
		channelLPtrList.GetNext(pos1);
		ChannelList* cur_channel =	(ChannelList*)channelLPtrList.GetAt(pos2);
		channelLPtrList.RemoveAt(pos2);
		delete cur_channel;
	}
	if( !b_shutdown )
	{
		//Only do this if eMule is still running..
		DeleteAllItems();
	}
}

void CIrcChannelListCtrl::AddChannelToList( CString name, CString user, CString description )
{
	//Add a new channel to Server Channel List
	CString ntemp = name;
	CString dtemp = description;
	int usertest = _tstoi(user);
	if( thePrefs.GetIRCUseChanFilter() )
	{
		//We need to filter the channels..
		if( usertest < thePrefs.GetIRCChannelUserFilter() )
		{
			//There were not enough users in the channel.
			return;
		}
		if( dtemp.MakeLower().Find(thePrefs.GetIRCChanNameFilter().MakeLower()) == -1 && ntemp.MakeLower().Find(thePrefs.GetIRCChanNameFilter().MakeLower()) == -1)
		{
			//The word we wanted was not in the channel name or description..
			return;
		}
	}
	//Create new ChannelList object.
	ChannelList* toadd = new ChannelList;
	toadd->name = name;
	toadd->users = user;
	//Strip any color codes out of the description..
	toadd->desc = m_pParent->StripMessageOfFontCodes(description);
	//Add to tail and update list.
	channelLPtrList.AddTail( toadd);
	uint16 itemnr = GetItemCount();
	itemnr = InsertItem(LVIF_PARAM,itemnr,0,0,0,0,(LPARAM)toadd);
	SetItemText(itemnr,0,toadd->name);
	SetItemText(itemnr,1,toadd->users);
	SetItemText(itemnr,2,toadd->desc);
}

void CIrcChannelListCtrl::JoinChannels()
{
	if( !m_pParent->IsConnected() ) 
		return;
	int index = -1; 
	POSITION pos = GetFirstSelectedItemPosition(); 
	while(pos != NULL) 
	{ 
		index = GetNextSelectedItem(pos); 
		if(index > -1)
		{ 
			CString join;
			join = _T("JOIN ") + GetItemText(index, 0 );
			m_pParent->m_pIrcMain->SendString( join );
		}
	} 
}

void CIrcChannelListCtrl::Localize()
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

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

void CIrcChannelListCtrl::Init()
{
	InsertColumn(0, GetResString(IDS_IRC_NAME), LVCFMT_LEFT, 203 );
	InsertColumn(1, GetResString(IDS_UUSERS), LVCFMT_LEFT, 50 );
	InsertColumn(2, GetResString(IDS_DESCRIPTION), LVCFMT_LEFT, 350 );

	SortItems(SortProc, 11);
	SetSortArrow(1, false);
}

BOOL CIrcChannelListCtrl::OnCommand(WPARAM wParam,LPARAM lParam )
{
	switch( wParam )
	{
		case Irc_Join: 
		{
			//Pressed the join button.
			JoinChannels();
			return true;
		}
   }
   return true;
}
