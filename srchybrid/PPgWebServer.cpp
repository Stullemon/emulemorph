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
#include "stdafx.h"
#include "emule.h"
#include "PPgWebServer.h"
#include "otherfunctions.h"
#include "WebServer.h"
#include "MMServer.h"
#include "emuledlg.h"
#include "Preferences.h"
#include "ServerWnd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define HIDDEN_PASSWORD _T("*****")


IMPLEMENT_DYNAMIC(CPPgWebServer, CPropertyPage)
CPPgWebServer::CPPgWebServer()
	: CPropertyPage(CPPgWebServer::IDD)
{
	bCreated = false;
}

CPPgWebServer::~CPPgWebServer()
{
}


void CPPgWebServer::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPPgWebServer, CPropertyPage)
	ON_EN_CHANGE(IDC_WSPASS, OnDataChange)
	ON_EN_CHANGE(IDC_WSPASSLOW, OnDataChange)
	ON_EN_CHANGE(IDC_WSPORT, OnDataChange)
	ON_EN_CHANGE(IDC_MMPASSWORDFIELD, OnDataChange)
	ON_EN_CHANGE(IDC_TMPLPATH, OnDataChange)
	ON_EN_CHANGE(IDC_MMPORT_FIELD, OnDataChange)
	ON_BN_CLICKED(IDC_WSENABLED, OnEnChangeWSEnabled)
	ON_BN_CLICKED(IDC_WSENABLEDLOW, OnEnChangeWSEnabled)
	ON_BN_CLICKED(IDC_MMENABLED, OnEnChangeMMEnabled)
	ON_BN_CLICKED(IDC_WSRELOADTMPL, OnReloadTemplates)
	ON_BN_CLICKED(IDC_TMPLBROWSE, OnBnClickedTmplbrowse)
	ON_BN_CLICKED(IDC_WS_GZIP, OnDataChange)
END_MESSAGE_MAP()


BOOL CPPgWebServer::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	((CEdit*)GetDlgItem(IDC_WSPASS))->SetLimitText(12);
	((CEdit*)GetDlgItem(IDC_WSPORT))->SetLimitText(6);

	LoadSettings();
	Localize();

	OnEnChangeWSEnabled();

	// note: there are better classes to create a pure hyperlink, however since it is only needed here
	//		 we rather use an already existing class
	CRect rect;
	GetDlgItem(IDC_GUIDELINK)->GetWindowRect(rect);
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rect, 2);
	m_wndMobileLink.CreateEx(NULL,0,"MsgWnd",WS_BORDER | WS_VISIBLE | WS_CHILD | HTC_WORDWRAP | HTC_UNDERLINE_HOVER,rect.left,rect.top,rect.Width(),rect.Height(),m_hWnd,0);
	m_wndMobileLink.SetBkColor(::GetSysColor(COLOR_3DFACE)); // still not the right color, will fix this later (need to merge the .rc file before it changes ;) )
	m_wndMobileLink.SetFont(GetFont());
	if (!bCreated){
		bCreated = true;
		m_wndMobileLink.AppendText("Link: ");
		m_wndMobileLink.AppendHyperLink(GetResString(IDS_MMGUIDELINK),0,CString("http://mobil.emule-project.net"),0,0);
	}
	return TRUE;
}

void CPPgWebServer::LoadSettings(void)
{
	CString strBuffer;

	GetDlgItem(IDC_WSPASS)->SetWindowText(HIDDEN_PASSWORD);
	GetDlgItem(IDC_WSPASSLOW)->SetWindowText(HIDDEN_PASSWORD);
	GetDlgItem(IDC_MMPASSWORDFIELD)->SetWindowText(HIDDEN_PASSWORD);

	strBuffer.Format("%d", app_prefs->GetWSPort());
	GetDlgItem(IDC_WSPORT)->SetWindowText(strBuffer);

	strBuffer.Format("%d", app_prefs->GetMMPort());
	GetDlgItem(IDC_MMPORT_FIELD)->SetWindowText(strBuffer);

	GetDlgItem(IDC_TMPLPATH)->SetWindowText(app_prefs->GetTemplate());

	if(app_prefs->GetWSIsEnabled())
		CheckDlgButton(IDC_WSENABLED,1);
	else
		CheckDlgButton(IDC_WSENABLED,0);

	if(app_prefs->GetWSIsLowUserEnabled())
		CheckDlgButton(IDC_WSENABLEDLOW,1);
	else
		CheckDlgButton(IDC_WSENABLEDLOW,0);

	if(app_prefs->IsMMServerEnabled())
		CheckDlgButton(IDC_MMENABLED,1);
	else
		CheckDlgButton(IDC_MMENABLED,0);

	CheckDlgButton(IDC_WS_GZIP,(app_prefs->GetWebUseGzip())?1:0 );
	
	OnEnChangeMMEnabled();

	SetModified(FALSE);	// FoRcHa
}

BOOL CPPgWebServer::OnApply()
{	
	if(m_bModified)
	{
		CString sBuf;
		uint16 oldPort=app_prefs->GetWSPort();

		GetDlgItem(IDC_WSPASS)->GetWindowText(sBuf);
		if(sBuf != HIDDEN_PASSWORD)
			app_prefs->SetWSPass(sBuf);
		
		GetDlgItem(IDC_WSPASSLOW)->GetWindowText(sBuf);
		if(sBuf != HIDDEN_PASSWORD)
			app_prefs->SetWSLowPass(sBuf);

		GetDlgItem(IDC_WSPORT)->GetWindowText(sBuf);
		if (atoi(sBuf)!=oldPort) {
			app_prefs->SetWSPort(atoi(sBuf));
			theApp.webserver->RestartServer();
		}
		app_prefs->SetWSIsEnabled((int8)IsDlgButtonChecked(IDC_WSENABLED));
		app_prefs->SetWSIsLowUserEnabled((int8)IsDlgButtonChecked(IDC_WSENABLEDLOW));
		app_prefs->SetWebUseGzip( (int8)IsDlgButtonChecked(IDC_WS_GZIP));

		theApp.webserver->StartServer();

		GetDlgItem(IDC_TMPLPATH)->GetWindowText(sBuf);
		app_prefs->SetTemplate(sBuf);

		// mobilemule
		GetDlgItem(IDC_MMPORT_FIELD)->GetWindowText(sBuf);
		if (atoi(sBuf)!= theApp.glob_prefs->GetMMPort() ) {
			app_prefs->SetMMPort(atoi(sBuf));
			theApp.mmserver->StopServer();
			theApp.mmserver->Init();
		}
		app_prefs->SetMMIsEnabled((int8)IsDlgButtonChecked(IDC_MMENABLED));
		
		if (IsDlgButtonChecked(IDC_MMENABLED))
			theApp.mmserver->Init();
		else
			theApp.mmserver->StopServer();
		GetDlgItem(IDC_MMPASSWORDFIELD)->GetWindowText(sBuf);
		if(sBuf != HIDDEN_PASSWORD)
			app_prefs->SetMMPass(sBuf);

		theApp.emuledlg->serverwnd->UpdateMyInfo();
		SetModified(FALSE);
	}

	return CPropertyPage::OnApply();
}

void CPPgWebServer::Localize(void)
{
	if(m_hWnd){
		SetWindowText(GetResString(IDS_PW_WS));
		GetDlgItem(IDC_WSPASS_LBL)->SetWindowText(GetResString(IDS_WS_PASS));
		GetDlgItem(IDC_WSPORT_LBL)->SetWindowText(GetResString(IDS_PORT));
		GetDlgItem(IDC_WSENABLED)->SetWindowText(GetResString(IDS_ENABLED));
		GetDlgItem(IDC_WSRELOADTMPL)->SetWindowText(GetResString(IDS_SF_RELOAD));
		GetDlgItem(IDC_WSENABLED)->SetWindowText(GetResString(IDS_ENABLED));
		SetDlgItemText(IDC_WS_GZIP,GetResString(IDS_WEB_GZIP_COMPRESSION));

		GetDlgItem(IDC_WSPASS_LBL2)->SetWindowText(GetResString(IDS_WS_PASS));
		GetDlgItem(IDC_WSENABLEDLOW)->SetWindowText(GetResString(IDS_ENABLED));
		GetDlgItem(IDC_STATIC_GENERAL)->SetWindowText(GetResString(IDS_PW_GENERAL));

		GetDlgItem(IDC_STATIC_ADMIN)->SetWindowText(GetResString(IDS_ADMIN));
		GetDlgItem(IDC_STATIC_LOWUSER)->SetWindowText(GetResString(IDS_WEB_LOWUSER));
		GetDlgItem(IDC_WSENABLEDLOW)->SetWindowText(GetResString(IDS_ENABLED));

		GetDlgItem(IDC_TEMPLATE)->SetWindowText(GetResString(IDS_WS_RELOAD_TMPL));

		GetDlgItem(IDC_MMENABLED)->SetWindowText(GetResString(IDS_ENABLEMM));
		GetDlgItem(IDC_STATIC_MOBILEMULE)->SetWindowText(GetResString(IDS_MOBILEMULE));
		GetDlgItem(IDC_MMPASSWORD)->SetWindowText(GetResString(IDS_WS_PASS));
		GetDlgItem(IDC_MMPORT_LBL)->SetWindowText(GetResString(IDS_PORT));
	}
}

void CPPgWebServer::OnEnChangeWSEnabled()
{
	GetDlgItem(IDC_WSPASS)->EnableWindow(IsDlgButtonChecked(IDC_WSENABLED));	
	GetDlgItem(IDC_WSPORT)->EnableWindow(IsDlgButtonChecked(IDC_WSENABLED));	
	GetDlgItem(IDC_WSENABLEDLOW)->EnableWindow(IsDlgButtonChecked(IDC_WSENABLED));
	GetDlgItem(IDC_TMPLPATH)->EnableWindow(IsDlgButtonChecked(IDC_WSENABLED));
	GetDlgItem(IDC_TMPLBROWSE)->EnableWindow(IsDlgButtonChecked(IDC_WSENABLED));
	GetDlgItem(IDC_WSRELOADTMPL)->EnableWindow(IsDlgButtonChecked(IDC_WSENABLED));
	GetDlgItem(IDC_WS_GZIP)->EnableWindow(IsDlgButtonChecked(IDC_WSENABLED));

	GetDlgItem(IDC_WSPASSLOW)->EnableWindow(IsDlgButtonChecked(IDC_WSENABLED) && IsDlgButtonChecked(IDC_WSENABLEDLOW));

	SetModified();
}

void CPPgWebServer::OnEnChangeMMEnabled()
{
	GetDlgItem(IDC_MMPASSWORDFIELD)->EnableWindow(IsDlgButtonChecked(IDC_MMENABLED));	
	GetDlgItem(IDC_MMPORT_FIELD)->EnableWindow(IsDlgButtonChecked(IDC_MMENABLED));

	SetModified();
}

void CPPgWebServer::OnReloadTemplates()
{
	theApp.webserver->ReloadTemplates();
}

void CPPgWebServer::OnBnClickedTmplbrowse()
{
	CString strTempl;
	GetDlgItemText(IDC_TMPLPATH, strTempl);
	CString buffer;
	buffer=GetResString(IDS_WS_RELOAD_TMPL)+"(*.tmpl)|*.tmpl||";
    if (DialogBrowseFile(buffer, "Template "+buffer, strTempl)){
		GetDlgItem(IDC_TMPLPATH)->SetWindowText(buffer);
		SetModified();
	}
}
