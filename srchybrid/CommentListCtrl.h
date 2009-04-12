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
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once
#include "MuleListCtrl.h"

class CUpDownClient;
namespace Kademlia
{
	class CEntry;
};

class CCommentListCtrl : public CMuleListCtrl
{
	DECLARE_DYNAMIC(CCommentListCtrl)

public:
	CCommentListCtrl();
	virtual ~CCommentListCtrl();

	void Init();
	void AddItem(const CUpDownClient* client);
	void AddItem(const Kademlia::CEntry* entry);

protected:
	struct SComment
	{
		SComment(const void* pClientCookie, int iRating, const CString& strComment, 
			     const CString& strFileName, const CString& strUserName, int iOrigin,
				 const CString& strClientSoftware,  //Commander - Added: ClientSoftware Column
				 const CString& strClientCountry) //Commander - Added: ClientCountry Column
			: m_pClientCookie(pClientCookie), m_iRating(iRating), 
			  m_strComment(strComment), m_strFileName(strFileName), 
			  m_strUserName(strUserName), m_iOrigin(iOrigin),
			  m_strClientSoft(strClientSoftware),  //Commander - Added: ClientSoftware Column
			  m_strClientCountry(strClientCountry) //Commander - Added: ClientCountry Column
		{ }

		const void* m_pClientCookie;
		int m_iRating;
		CString m_strComment;
		CString m_strFileName;
		CString m_strUserName;
		int m_iOrigin;	// 0=eD2K, 1=Kad
		CString m_strClientSoft; //Commander - Added: ClientSoftware Column
		CString m_strClientCountry; //Commander - Added: ClientCountry Column
	};
	void AddComment(const SComment* pComment);
	int FindClientComment(const void* pCookie);

	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLvnColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnDeleteItem(NMHDR *pNMHDR, LRESULT *pResult);
};
