#pragma once

//this file is part of eMule
//Copyright (C)2002-2005 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "MuleListCtrl.h"
#include "ListCtrlItemWalk.h"

class CAbstractFile;
class CUpDownClient;

namespace Kademlia 
{
	class CEntry;
};

class CCommentListCtrl : public CMuleListCtrl, public CListCtrlItemWalk
{
	DECLARE_DYNAMIC(CCommentListCtrl)

public:
	CCommentListCtrl();
	virtual ~CCommentListCtrl();

	void Init();
	void Localize();

	void AddItem(CUpDownClient* client);
	void AddItem(Kademlia::CEntry* entry);

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};