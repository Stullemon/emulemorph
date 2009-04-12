//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "LayeredWindowHelperST.h"

#define	IDT_AUTO_CLOSE_TIMER	100

// CMiniMule dialog

class CMiniMule : public CDHtmlDialog
{
	DECLARE_DYNCREATE(CMiniMule)

public:
	CMiniMule(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMiniMule();

// Dialog Data
	enum { IDD = IDD_MINIMULE, IDH = IDR_HTML_MINIMULE };

	bool GetAutoClose() const { return m_bAutoClose; }
	void SetAutoClose(bool bAutoClose) { m_bAutoClose = bAutoClose; }
	bool IsInCallback() const { return m_iInCallback != 0; }
	void UpdateContent(UINT uUpDatarate = (UINT)-1, UINT uDownDatarate = (UINT)-1);
	void Localize();

protected:
	int m_iInCallback;
	bool m_bResolveImages;
	bool m_bRestoreMainWnd;
	UINT m_uWndTransparency;
	CLayeredWindowHelperST m_layeredWnd;

	// Auto-close
	bool m_bAutoClose;
	UINT m_uAutoCloseTimer;
	void CreateAutoCloseTimer();
	void KillAutoCloseTimer();

	void AutoSizeAndPosition(CSize sizClient);
	void RestoreMainWindow();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL CreateControlSite(COleControlContainer* pContainer, COleControlSite** ppSite, UINT nID, REFCLSID clsid);
	virtual void PostNcDestroy();
	
	virtual void OnBeforeNavigate(LPDISPATCH pDisp, LPCTSTR szUrl);
	virtual void OnNavigateComplete(LPDISPATCH pDisp, LPCTSTR szUrl);
	virtual void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl);

	STDMETHOD(GetOptionKeyPath)(LPOLESTR *pchKey, DWORD dw);
	STDMETHOD(TranslateUrl)(DWORD dwTranslate, OLECHAR* pchURLIn, OLECHAR** ppchURLOut);
	STDMETHOD(ShowContextMenu)(DWORD dwID, POINT* ppt, IUnknown* pcmdtReserved, IDispatch* pdispReserved);
	STDMETHOD(TranslateAccelerator)(LPMSG lpMsg, const GUID* pguidCmdGroup, DWORD nCmdID);

	DECLARE_EVENTSINK_MAP()
	void _OnBeforeNavigate2(LPDISPATCH pDisp, VARIANT* URL, VARIANT* Flags, VARIANT* TargetFrameName, VARIANT* PostData, VARIANT* Headers, BOOL* Cancel);

	DECLARE_DHTML_EVENT_MAP()
	HRESULT OnRestoreMainWindow(IHTMLElement* pElement);
	HRESULT OnOpenIncomingFolder(IHTMLElement* pElement);
	HRESULT OnOptions(IHTMLElement* pElement);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
};
