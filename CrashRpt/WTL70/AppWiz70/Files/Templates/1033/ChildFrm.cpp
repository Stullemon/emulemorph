// [!output WTL_CHILD_FRAME_FILE].cpp : implementation of the [!output WTL_CHILD_FRAME_CLASS] class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

[!if WTL_USE_VIEW]
#include "[!output WTL_VIEW_FILE].h"
[!endif]
#include "[!output WTL_CHILD_FRAME_FILE].h"

void [!output WTL_CHILD_FRAME_CLASS]::OnFinalMessage(HWND /*hWnd*/)
{
	delete this;
}

[!if WTL_USE_VIEW]
LRESULT [!output WTL_CHILD_FRAME_CLASS]::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
[!if WTL_VIEWTYPE_FORM]
	m_hWndClient = m_view.Create(m_hWnd);
[!else]
[!if WTL_VIEWTYPE_HTML]
	//TODO: Replace with a URL of your choice
	m_hWndClient = m_view.Create(m_hWnd, rcDefault, _T("http://www.microsoft.com"), [!output WTL_VIEW_STYLES], [!output WTL_VIEW_EX_STYLES]);
[!else]
	m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, [!output WTL_VIEW_STYLES], [!output WTL_VIEW_EX_STYLES]);
[!endif]
[!endif]

	bHandled = FALSE;
	return 1;
}

[!endif]
LRESULT [!output WTL_CHILD_FRAME_CLASS]::OnForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	LPMSG pMsg = (LPMSG)lParam;

[!if WTL_USE_VIEW]
	if([!output WTL_CHILD_FRAME_BASE_CLASS]<[!output WTL_CHILD_FRAME_CLASS]>::PreTranslateMessage(pMsg))
		return TRUE;

	return m_view.PreTranslateMessage(pMsg);
[!else]
	return [!output WTL_CHILD_FRAME_BASE_CLASS]<[!output WTL_CHILD_FRAME_CLASS]>::PreTranslateMessage(pMsg);
[!endif]
}
