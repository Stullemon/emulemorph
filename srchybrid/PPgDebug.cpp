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
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include "emule.h"
#include "PPgDebug.h"
#include "Preferences.h"
#include "OtherFunctions.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


///////////////////////////////////////////////////////////////////////////////
// CPPgDebug dialog

IMPLEMENT_DYNAMIC(CPPgDebug, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgDebug, CPropertyPage)
	ON_WM_DESTROY()
	ON_MESSAGE(WM_TREEOPTSCTRL_NOTIFY, OnTreeOptsCtrlNotify)
END_MESSAGE_MAP()

CPPgDebug::CPPgDebug()
	: CPropertyPage(CPPgDebug::IDD)
	, m_ctrlTreeOptions(theApp.m_iDfltImageListColorFlags)
{
	ClearAllMembers();
}

CPPgDebug::~CPPgDebug()
{
}

void CPPgDebug::ClearAllMembers()
{
	m_bInitializedTreeOpts = false;
	m_htiServer = NULL;
	m_htiClient = NULL;
	memset(m_cb, 0, sizeof m_cb);
	memset(m_lv, 0, sizeof m_lv);
	memset(m_checks, 0, sizeof m_checks);
	memset(m_levels, 0, sizeof m_levels);
}

void CPPgDebug::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DEBUG_OPTS, m_ctrlTreeOptions);
	if (!m_bInitializedTreeOpts)
	{
		int iImgServer = 8; // default icon
		int iImgClient = 8; // default icon
		CImageList* piml = m_ctrlTreeOptions.GetImageList(TVSIL_NORMAL);
		if (piml){
			HICON hIcon = theApp.LoadIcon("Server");
			if (hIcon){
				iImgServer = piml->Add(hIcon);
				VERIFY( ::DestroyIcon(hIcon) );
			}

			hIcon = theApp.LoadIcon("StatsClients");
			if (hIcon){
				iImgClient = piml->Add(hIcon);
				VERIFY( ::DestroyIcon(hIcon) );
			}
		}

#define	ADD_DETAIL_ITEM(idx, label, group) \
		m_cb[idx] = m_ctrlTreeOptions.InsertCheckBox(label, group); \
		m_lv[idx] = m_ctrlTreeOptions.InsertItem("Level", m_cb[idx]); \
		m_ctrlTreeOptions.AddEditBox(m_lv[idx], RUNTIME_CLASS(CNumTreeOptionsEdit));

		m_htiServer = m_ctrlTreeOptions.InsertCheckBox("Server", TVI_ROOT, FALSE);
		ADD_DETAIL_ITEM(0, "TCP", m_htiServer);
		ADD_DETAIL_ITEM(1, "UDP", m_htiServer);
		ADD_DETAIL_ITEM(2, "Sources", m_htiServer);
		ADD_DETAIL_ITEM(3, "Searches", m_htiServer);

		m_htiClient = m_ctrlTreeOptions.InsertCheckBox("Client", TVI_ROOT, FALSE);
		ADD_DETAIL_ITEM(4, "TCP", m_htiClient);
		ADD_DETAIL_ITEM(5, "UDP", m_htiClient);

		m_ctrlTreeOptions.Expand(m_htiServer, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiClient, TVE_EXPAND);
		m_ctrlTreeOptions.SendMessage(WM_VSCROLL, SB_TOP);
		m_bInitializedTreeOpts = true;
	}

	for (int i = 0; i < ARRSIZE(m_cb); i++)
		DDX_TreeCheck(pDX, IDC_DEBUG_OPTS, m_cb[i], m_checks[i]);
	m_ctrlTreeOptions.UpdateCheckBoxGroup(m_htiServer);
	m_ctrlTreeOptions.UpdateCheckBoxGroup(m_htiClient);

	for (int i = 0; i < ARRSIZE(m_lv); i++)
		DDX_TreeEdit(pDX, IDC_DEBUG_OPTS, m_lv[i], m_levels[i]);
}

BOOL CPPgDebug::OnInitDialog()
{
#define	SET_DETAIL_OPT(idx, var) \
	m_checks[idx] = ((var) > 0); \
	m_levels[idx] = ((var) > 0) ? (var) : -(var);

	SET_DETAIL_OPT(0, app_prefs->prefs->m_iDebugServerTCPLevel);
	SET_DETAIL_OPT(1, app_prefs->prefs->m_iDebugServerUDPLevel);
	SET_DETAIL_OPT(2, app_prefs->prefs->m_iDebugServerSourcesLevel);
	SET_DETAIL_OPT(3, app_prefs->prefs->m_iDebugServerSearchesLevel);
	SET_DETAIL_OPT(4, app_prefs->prefs->m_iDebugClientTCPLevel);
	SET_DETAIL_OPT(5, app_prefs->prefs->m_iDebugClientUDPLevel);
#undef SET_OPT

	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgDebug::OnKillActive()
{
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();
	return CPropertyPage::OnKillActive();
}

BOOL CPPgDebug::OnApply()
{
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();

	if (!UpdateData())
		return FALSE;

#define	GET_DETAIL_OPT(idx, opt) \
	if (m_checks[idx]) \
		opt = (m_levels[idx] > 0) ? m_levels[idx] : 1; \
	else \
		opt = -m_levels[idx];

	GET_DETAIL_OPT(0, app_prefs->prefs->m_iDebugServerTCPLevel);
	GET_DETAIL_OPT(1, app_prefs->prefs->m_iDebugServerUDPLevel);
	GET_DETAIL_OPT(2, app_prefs->prefs->m_iDebugServerSourcesLevel);
	GET_DETAIL_OPT(3, app_prefs->prefs->m_iDebugServerSearchesLevel);
	GET_DETAIL_OPT(4, app_prefs->prefs->m_iDebugClientTCPLevel);
	GET_DETAIL_OPT(5, app_prefs->prefs->m_iDebugClientUDPLevel);
#undef GET_DETAIL_OPT

	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgDebug::OnDestroy()
{
	m_ctrlTreeOptions.DeleteAllItems();
	m_ctrlTreeOptions.DestroyWindow();
	ClearAllMembers();
	CPropertyPage::OnDestroy();
}

LRESULT CPPgDebug::OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == IDC_DEBUG_OPTS){
		TREEOPTSCTRLNOTIFY* pton = (TREEOPTSCTRLNOTIFY*)lParam;
		SetModified();
	}
	return 0;
}
