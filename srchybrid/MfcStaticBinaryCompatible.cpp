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

// ****************************************************************************
// IMPORTANT NOTE !!!
//
// The reason for this file and the according compiler options is to solve the 
// binary compatibility problem with the (static) version of the MFC library 
// which shows up due to the defines which are specified in 'stdafx.h' of the
// eMule project.
// 
// The MFC library of VS .NET 2002 is compiled with the following SDK defines:
// 
// WINVER         = 0x0501
// _WIN32_WINNT   = 0x0501
// _WIN32_WINDOWS = 0x0410
// _WIN32_IE      = 0x0560
// 
// eMule is using different defines (see 'stdafx.h') to achieve backward 
// compatibility with older Windows systems.
// 
// Depending on the code which is taken from the MFC library or from eMule object 
// files at linking time, it may occur that we get different structure size, 
// structure member variable offsets and other ugly things than expected and used 
// within the MFC library or vice versa.
// 
// To solve this issue, some code parts (e.g. the MFC template for allocating the 
// AFX_MODULE_THREAD_STATE structure) HAVE to be compiled with the same defines 
// as used by the MFC library compilation.
// 
// This means, that this CPP file does NOT HAVE to be compiled with pre-compiled 
// headers and (more important) is NOT ALLOWED to include 'stdafx.h' from the 
// eMule project. It has to be ensured in every aspect, that this CPP file is 
// compiled with the very same compiler settings as used for the MFC library!
// ****************************************************************************
//#include "stdafx.h"	// please read the comment above!!

#include <afxwin.h>
#include <afxpriv.h>

// NOTE: Although the function is currently not used any longer, the source file 
// is kept because of the above comment. Hopefully we will never need that file
// again.
/*void Mfc_IdleUpdateCmdUiTopLevelFrameList(CWnd* pMainFrame)
{
#ifdef _AFXDLL
	// Can't link this in a Release build, see KB Q316312
	AFX_MODULE_THREAD_STATE* pState = AfxGetAppModuleState()->m_thread;
#else
	AFX_MODULE_THREAD_STATE* pState = _AFX_CMDTARGET_GETSTATE()->m_thread;
#endif
	CFrameWnd* pFrameWnd = pState->m_frameList;
	while (pFrameWnd != NULL)
	{
		if (pFrameWnd->m_hWnd != NULL && pFrameWnd != pMainFrame)
		{
			if (pFrameWnd->m_nShowDelay == SW_HIDE)
				pFrameWnd->ShowWindow(pFrameWnd->m_nShowDelay);
			if (pFrameWnd->IsWindowVisible() || pFrameWnd->m_nShowDelay >= 0)
			{
				AfxCallWndProc(pFrameWnd, pFrameWnd->m_hWnd, WM_IDLEUPDATECMDUI, (WPARAM)TRUE, 0);
				pFrameWnd->SendMessageToDescendants(WM_IDLEUPDATECMDUI, (WPARAM)TRUE, 0, TRUE, TRUE);
			}
			if (pFrameWnd->m_nShowDelay > SW_HIDE)
				pFrameWnd->ShowWindow(pFrameWnd->m_nShowDelay);
			pFrameWnd->m_nShowDelay = -1;
		}
		pFrameWnd = pFrameWnd->m_pNextFrameWnd;
	}
}*/