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
#include "PPGProxy.h"
#include "opcodes.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "HelpIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CPPgProxy, CPropertyPage)
CPPgProxy::CPPgProxy()
	: CPropertyPage(CPPgProxy::IDD)
{
}

CPPgProxy::~CPPgProxy()
{
}

void CPPgProxy::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPPgProxy, CPropertyPage)
	ON_BN_CLICKED(IDC_ENABLEPROXY, OnBnClickedEnableproxy)
	ON_BN_CLICKED(IDC_ENABLEAUTH, OnBnClickedEnableauth)
	ON_CBN_SELCHANGE(IDC_PROXYTYPE, OnCbnSelchangeProxytype)
	ON_EN_CHANGE(IDC_PROXYNAME, OnEnChangeProxyname)
	ON_EN_CHANGE(IDC_PROXYPORT, OnEnChangeProxyport)
	ON_EN_CHANGE(IDC_USERNAME_A, OnEnChangeUsername)
	ON_EN_CHANGE(IDC_PASSWORD, OnEnChangePassword)
	ON_BN_CLICKED(IDC_ASCWOP, OnBnClickedAscwop)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()


BOOL CPPgProxy::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	proxy = thePrefs.GetProxy();
	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgProxy::OnApply()
{
	USES_CONVERSION;
	thePrefs.SetProxyASCWOP(IsDlgButtonChecked(IDC_ASCWOP)!=0);
	proxy.UseProxy = (IsDlgButtonChecked(IDC_ENABLEPROXY)!=0);
	proxy.EnablePassword = ((CButton*)GetDlgItem(IDC_ENABLEAUTH))->GetCheck()!=0;
	proxy.type = ((CComboBox*)GetDlgItem(IDC_PROXYTYPE))->GetCurSel();

	if(GetDlgItem(IDC_PROXYNAME)->GetWindowTextLength())
	{ 
		GetDlgItem(IDC_PROXYNAME)->GetWindowText(proxy.name, ARRSIZE(proxy.name));
	}
	else
	{
		proxy.name[0] = _T('\0');
        proxy.UseProxy = false;
	}

	if(GetDlgItem(IDC_PROXYPORT)->GetWindowTextLength())
	{ 
		TCHAR buffer[6];
		GetDlgItem(IDC_PROXYPORT)->GetWindowText(buffer,ARRSIZE(buffer));
		proxy.port = (_tstoi(buffer)) ? _tstoi(buffer) : 1080;
	}
	else
		proxy.port = 1080;

	if(GetDlgItem(IDC_USERNAME_A)->GetWindowTextLength())
	{ 
		CString strUser;
		GetDlgItem(IDC_USERNAME_A)->GetWindowText(strUser);
		_snprintf(proxy.user, ARRSIZE(proxy.user), "%s", T2CA(strUser));
	}
	else
	{
		proxy.user[0] = '\0';
		proxy.EnablePassword = false;
	}

	if(GetDlgItem(IDC_PASSWORD)->GetWindowTextLength())
	{ 
		CString strPasswd;
		GetDlgItem(IDC_PASSWORD)->GetWindowText(strPasswd);
		_snprintf(proxy.password, ARRSIZE(proxy.password), "%s", T2CA(strPasswd));
	}
	else
	{
		proxy.password[0] = '\0';
		proxy.EnablePassword = false;
	}
	thePrefs.SetProxySettings(proxy);
	LoadSettings();
	return TRUE;
}
// CPPGProxy message handlers

void CPPgProxy::OnBnClickedEnableproxy()
{
	SetModified(true);
	CButton* btn = (CButton*) GetDlgItem(IDC_ENABLEPROXY);

	GetDlgItem(IDC_ENABLEAUTH)->EnableWindow(btn->GetCheck() != 0);
	GetDlgItem(IDC_PROXYTYPE)->EnableWindow(btn->GetCheck() != 0);
	GetDlgItem(IDC_PROXYNAME)->EnableWindow(btn->GetCheck() != 0);
	GetDlgItem(IDC_PROXYPORT)->EnableWindow(btn->GetCheck() != 0);
	GetDlgItem(IDC_USERNAME_A)->EnableWindow(btn->GetCheck() != 0);
	GetDlgItem(IDC_PASSWORD)->EnableWindow(btn->GetCheck() != 0);
	if (btn->GetCheck() != 0) OnBnClickedEnableauth();
	if (btn->GetCheck() != 0) OnCbnSelchangeProxytype();
	if (btn->GetCheck() != 0) OnBnClickedAscwop();
}

void CPPgProxy::OnBnClickedEnableauth()
{
	SetModified(true);
	CButton* btn = (CButton*) GetDlgItem(IDC_ENABLEAUTH);

	GetDlgItem(IDC_USERNAME_A)->EnableWindow(btn->GetCheck() != 0);
	GetDlgItem(IDC_PASSWORD)->EnableWindow(btn->GetCheck() != 0);
}

void CPPgProxy::OnCbnSelchangeProxytype()
{
	SetModified(true);
	CComboBox* cbbox = (CComboBox*)GetDlgItem(IDC_PROXYTYPE);
	if (!(cbbox->GetCurSel() == PROXYTYPE_SOCKS5 || cbbox->GetCurSel() == PROXYTYPE_HTTP11))
	{
		((CButton*)GetDlgItem(IDC_ENABLEAUTH))->SetCheck(false);
		OnBnClickedEnableauth();
		GetDlgItem(IDC_ENABLEAUTH)->EnableWindow(false);
	} 
	else GetDlgItem(IDC_ENABLEAUTH)->EnableWindow(true);


}


void CPPgProxy::LoadSettings()
{
	USES_CONVERSION;
	CheckDlgButton(IDC_ASCWOP,(thePrefs.IsProxyASCWOP() ));
	((CButton*)GetDlgItem(IDC_ENABLEPROXY))->SetCheck(proxy.UseProxy);
	((CButton*)GetDlgItem(IDC_ENABLEAUTH))->SetCheck(proxy.EnablePassword);
	((CComboBox*)GetDlgItem(IDC_PROXYTYPE))->SetCurSel(proxy.type);
	GetDlgItem(IDC_PROXYNAME)->SetWindowText(proxy.name);
	CString buffer;
	buffer.Format(_T("%ld"),proxy.port);
	GetDlgItem(IDC_PROXYPORT)->SetWindowText(buffer);
	GetDlgItem(IDC_USERNAME_A)->SetWindowText(A2T(proxy.user));
	GetDlgItem(IDC_PASSWORD)->SetWindowText(A2T(proxy.password));
	OnBnClickedEnableproxy();
}

void CPPgProxy::Localize(void)
{
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_PROXY));
		GetDlgItem(IDC_ENABLEPROXY)->SetWindowText(GetResString(IDS_PROXY_ENABLE));	
		GetDlgItem(IDC_PROXYTYPE_LBL)->SetWindowText(GetResString(IDS_PROXY_TYPE));	
		GetDlgItem(IDC_PROXYNAME_LBL)->SetWindowText(GetResString(IDS_PROXY_HOST));	
		GetDlgItem(IDC_PROXYPORT_LBL)->SetWindowText(GetResString(IDS_PROXY_PORT));	
		GetDlgItem(IDC_ENABLEAUTH)->SetWindowText(GetResString(IDS_PROXY_AUTH));	
		GetDlgItem(IDC_USERNAME_LBL)->SetWindowText(GetResString(IDS_CD_UNAME));	
		GetDlgItem(IDC_PASSWORD_LBL)->SetWindowText(GetResString(IDS_WS_PASS)+_T(":"));	
		GetDlgItem(IDC_AUTH_LBL)->SetWindowText(GetResString(IDS_AUTH));	
		GetDlgItem(IDC_AUTH_LBL2)->SetWindowText(GetResString(IDS_PW_GENERAL));	
		GetDlgItem(IDC_ASCWOP)->SetWindowText(GetResString(IDS_ASCWOP_PROXYSUPPORT));
	}
}

void CPPgProxy::OnBnClickedAscwop()
{
	SetModified(true);
}

void CPPgProxy::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Proxy);
}

BOOL CPPgProxy::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgProxy::OnHelpInfo(HELPINFO* pHelpInfo)
{
	OnHelp();
	return TRUE;
}
