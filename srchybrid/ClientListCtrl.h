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

#pragma once
#include <sys/timeb.h>
#include "types.h"
#include "titlemenu.h"
#include "MuleListCtrl.h"


// CClientListCtrl
class CUpDownClient;

class CClientListCtrl : public CMuleListCtrl
{
	DECLARE_DYNAMIC(CClientListCtrl)

public:
	CClientListCtrl();
	virtual ~CClientListCtrl();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	void	Init();
	void	AddClient(CUpDownClient* client);
	void	RemoveClient(CUpDownClient* client);
	void	RefreshClient(CUpDownClient* client);
	void	Hide() {ShowWindow(SW_HIDE);}
	void	Visable() {ShowWindow(SW_SHOW);}
	void	Localize();
	void	ShowSelectedUserDetails();
	void	ShowKnownClients();
	virtual BOOL OnCommand(WPARAM wParam,LPARAM lParam );
protected:
	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	afx_msg	void OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
private:
	CImageList imagelist;
	CTitleMenu	   m_ClientMenu;
};
