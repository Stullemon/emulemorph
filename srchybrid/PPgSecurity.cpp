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
#include "PPgSecurity.h"
#include "OtherFunctions.h"
#include "IPFilter.h"
#include "Preferences.h"
#include "CustomAutoComplete.h"
#include "HttpDownloadDlg.h"
#include "emuledlg.h"
#include "HelpIDs.h"
#include "ZipFile.h"
#include "Log.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define	IPFILTERUPDATEURL_STRINGS_PROFILE	_T("AC_IPFilterUpdateURLs.dat")

IMPLEMENT_DYNAMIC(CPPgSecurity, CPropertyPage)
CPPgSecurity::CPPgSecurity()
	: CPropertyPage(CPPgSecurity::IDD)
{
	m_pacIPFilterURL = NULL;
}

CPPgSecurity::~CPPgSecurity()
{

}

void CPPgSecurity::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPPgSecurity, CPropertyPage)
	ON_BN_CLICKED(IDC_FILTERSERVERBYIPFILTER , OnSettingsChange)
	ON_BN_CLICKED(IDC_RELOADFILTER, OnReloadIPFilter)
	ON_BN_CLICKED(IDC_EDITFILTER, OnEditIPFilter)
	ON_EN_CHANGE(IDC_FILTERLEVEL, OnSettingsChange)
	ON_EN_CHANGE(IDC_FILTER, OnSettingsChange)
	ON_EN_CHANGE(IDC_COMMENTFILTER, OnSettingsChange)
	ON_BN_CLICKED(IDC_MSGONLYFRIENDS , OnSettingsChange) 
	ON_BN_CLICKED(IDC_MSGONLYSEC, OnSettingsChange)
	ON_BN_CLICKED(IDC_ADVSPAMFILTER , OnSettingsChange)
	ON_BN_CLICKED(IDC_USESECIDENT, OnSettingsChange)
	ON_BN_CLICKED(IDC_LOADURL, OnLoadIPFFromURL)
	ON_EN_CHANGE(IDC_UPDATEURL, OnEnChangeUpdateUrl)
	ON_BN_CLICKED(IDC_DD,OnDDClicked)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_RUNASUSER, OnBnClickedRunasuser)
END_MESSAGE_MAP()

void CPPgSecurity::LoadSettings(void)
{
	CString strBuffer;
	
	strBuffer.Format(_T("%i"),thePrefs.filterlevel);
	GetDlgItem(IDC_FILTERLEVEL)->SetWindowText(strBuffer);

	if(thePrefs.filterserverbyip)
		CheckDlgButton(IDC_FILTERSERVERBYIPFILTER,1);
	else
		CheckDlgButton(IDC_FILTERSERVERBYIPFILTER,0);

	if(thePrefs.msgonlyfriends)
		CheckDlgButton(IDC_MSGONLYFRIENDS,1);
	else
		CheckDlgButton(IDC_MSGONLYFRIENDS,0);

	if(thePrefs.msgsecure)
		CheckDlgButton(IDC_MSGONLYSEC,1);
	else
		CheckDlgButton(IDC_MSGONLYSEC,0);

	if(thePrefs.m_bAdvancedSpamfilter)
		CheckDlgButton(IDC_ADVSPAMFILTER,1);
	else
		CheckDlgButton(IDC_ADVSPAMFILTER,0);

	if(thePrefs.m_bUseSecureIdent)
		CheckDlgButton(IDC_USESECIDENT,1);
	else
		CheckDlgButton(IDC_USESECIDENT,0);

	if (thePrefs.GetWindowsVersion() == _WINVER_XP_ || thePrefs.GetWindowsVersion() == _WINVER_2K_)
		GetDlgItem(IDC_RUNASUSER)->EnableWindow(TRUE);
	else
		GetDlgItem(IDC_RUNASUSER)->EnableWindow(FALSE);

	if(thePrefs.IsRunAsUserEnabled())
		CheckDlgButton(IDC_RUNASUSER,1);
	else
		CheckDlgButton(IDC_RUNASUSER,0);

	GetDlgItem(IDC_FILTER)->SetWindowText(thePrefs.messageFilter);
	GetDlgItem(IDC_COMMENTFILTER)->SetWindowText(thePrefs.commentFilter);
}

BOOL CPPgSecurity::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);
	//MORPH START - Added by SiRoB, Allways use securedid
	GetDlgItem(IDC_USESECIDENT)->EnableWindow(0);
	//MORPH END   - Added by SiRoB, Allways use securedid
	LoadSettings();
	Localize();

	if (thePrefs.GetUseAutocompletion()){
		if (!m_pacIPFilterURL) {
			m_pacIPFilterURL = new CCustomAutoComplete();
			m_pacIPFilterURL->AddRef();
			if (m_pacIPFilterURL->Bind(::GetDlgItem(m_hWnd, IDC_UPDATEURL), ACO_UPDOWNKEYDROPSLIST | ACO_AUTOSUGGEST | ACO_FILTERPREFIXES ))
				m_pacIPFilterURL->LoadList(CString(thePrefs.GetConfigDir()) +  _T("\\") IPFILTERUPDATEURL_STRINGS_PROFILE);
		}
		SetDlgItemText(IDC_UPDATEURL,m_pacIPFilterURL->GetItem(0));
		if (theApp.m_fontSymbol.m_hObject){
			GetDlgItem(IDC_DD)->SetFont(&theApp.m_fontSymbol);
			GetDlgItem(IDC_DD)->SetWindowText(_T("6")); // show a down-arrow
		}
	}
	else
		GetDlgItem(IDC_DD)->ShowWindow(SW_HIDE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgSecurity::OnApply()
{
	TCHAR buffer[510];
	if(GetDlgItem(IDC_FILTERLEVEL)->GetWindowTextLength())
	{
		GetDlgItem(IDC_FILTERLEVEL)->GetWindowText(buffer,4);
		thePrefs.filterlevel=_tstoi(buffer);
	}

	thePrefs.filterserverbyip = (uint8)IsDlgButtonChecked(IDC_FILTERSERVERBYIPFILTER);
	thePrefs.msgonlyfriends = (uint8)IsDlgButtonChecked(IDC_MSGONLYFRIENDS);
	thePrefs.msgsecure = (uint8)IsDlgButtonChecked(IDC_MSGONLYSEC);
	thePrefs.m_bAdvancedSpamfilter = IsDlgButtonChecked(IDC_ADVSPAMFILTER);
	thePrefs.m_bUseSecureIdent = IsDlgButtonChecked(IDC_USESECIDENT);
	thePrefs.m_bRunAsUser = (uint8)IsDlgButtonChecked(IDC_RUNASUSER);

	GetDlgItem(IDC_FILTER)->GetWindowText(thePrefs.messageFilter,ARRSIZE(thePrefs.messageFilter));

	CString strCommentFilters;
	GetDlgItem(IDC_COMMENTFILTER)->GetWindowText(strCommentFilters);
	strCommentFilters.MakeLower();
	CString strNewCommentFilters;
	int curPos = 0;
	CString strFilter(strCommentFilters.Tokenize(_T("|"), curPos));
	while (!strFilter.IsEmpty())
	{
		strFilter.Trim();
		if (!strNewCommentFilters.IsEmpty())
			strNewCommentFilters += _T('|');
		strNewCommentFilters += strFilter;
		strFilter = strCommentFilters.Tokenize(_T("|"), curPos);
	}
	thePrefs.commentFilter = strNewCommentFilters;
	if (thePrefs.commentFilter != strCommentFilters)
		SetDlgItemText(IDC_COMMENTFILTER, thePrefs.commentFilter);

	LoadSettings();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgSecurity::Localize(void)
{
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_SECURITY));
		GetDlgItem(IDC_STATIC_IPFILTER)->SetWindowText(GetResString(IDS_IPFILTER));
		GetDlgItem(IDC_RELOADFILTER)->SetWindowText(GetResString(IDS_SF_RELOAD));
		GetDlgItem(IDC_EDITFILTER)->SetWindowText(GetResString(IDS_EDIT));
		GetDlgItem(IDC_STATIC_FILTERLEVEL)->SetWindowText(GetResString(IDS_FILTERLEVEL)+_T(":"));
		GetDlgItem(IDC_FILTERSERVERBYIPFILTER)->SetWindowText(GetResString(IDS_FILTERSERVERBYIPFILTER));

		GetDlgItem(IDC_FILTERCOMMENTSLABEL)->SetWindowText(GetResString(IDS_FILTERCOMMENTSLABEL));
		GetDlgItem(IDC_STATIC_COMMENTS)->SetWindowText(GetResString(IDS_COMMENT));

		GetDlgItem(IDC_FILTERLABEL)->SetWindowText(GetResString(IDS_FILTERLABEL));
		GetDlgItem(IDC_MSG)->SetWindowText(GetResString(IDS_CW_MESSAGES));

		GetDlgItem(IDC_MSGONLYFRIENDS)->SetWindowText(GetResString(IDS_MSGONLYFRIENDS));
		GetDlgItem(IDC_MSGONLYSEC)->SetWindowText(GetResString(IDS_MSGONLYSEC));

		GetDlgItem(IDC_ADVSPAMFILTER)->SetWindowText(GetResString(IDS_ADVSPAMFILTER));
		GetDlgItem(IDC_SEC_MISC)->SetWindowText(GetResString(IDS_PW_MISC));
		GetDlgItem(IDC_USESECIDENT)->SetWindowText(GetResString(IDS_USESECIDENT));

		SetDlgItemText(IDC_STATIC_UPDATEFROM,GetResString(IDS_UPDATEFROM));
		SetDlgItemText(IDC_LOADURL,GetResString(IDS_LOADURL));

		SetDlgItemText(IDC_RUNASUSER,GetResString(IDS_RUNASUSER));
	}
}

void CPPgSecurity::OnReloadIPFilter()
{
	theApp.ipfilter->LoadFromDefaultFile();
}

void CPPgSecurity::OnEditIPFilter()
{
	ShellExecute(NULL, _T("open"), thePrefs.GetTxtEditor(),
		_T("\"") + thePrefs.GetConfigDir() + DFLT_IPFILTER_FILENAME _T("\""), NULL, SW_SHOW);
}

void CPPgSecurity::OnLoadIPFFromURL() {
	CString url;
	GetDlgItemText(IDC_UPDATEURL,url);
	if (!url.IsEmpty())
	{
		// add entered URL to LRU list even if it's not yet known whether we can download from this URL (it's just more convenient this way)
		if (m_pacIPFilterURL && m_pacIPFilterURL->IsBound())
			m_pacIPFilterURL->AddItem(url, 0);

		TCHAR szTempFilePath[MAX_PATH];
		_tmakepath(szTempFilePath, NULL, thePrefs.GetConfigDir(), DFLT_IPFILTER_FILENAME, _T("tmp"));

		CHttpDownloadDlg dlgDownload;
		dlgDownload.m_strTitle = _T("Downloading IP filter file");
		dlgDownload.m_sURLToDownload = url;
		dlgDownload.m_sFileToDownloadInto = szTempFilePath;
		if (dlgDownload.DoModal() != IDOK)
		{
			_tremove(szTempFilePath);
			LogWarning(LOG_STATUSBAR, _T("IP Filter download failed"));
			return;
		}

		bool bIsZipFile = false;
		bool bUnzipped = false;
		CZIPFile zip;
		if (zip.Open(szTempFilePath))
		{
			bIsZipFile = true;

			CZIPFile::File* zfile = zip.GetFile(_T("guarding.p2p"));
			if (zfile)
			{
				TCHAR szTempUnzipFilePath[MAX_PATH];
				_tmakepath(szTempUnzipFilePath, NULL, thePrefs.GetConfigDir(), DFLT_IPFILTER_FILENAME, _T(".unzip.tmp"));
				if (zfile->Extract(szTempUnzipFilePath))
				{
					zip.Close();
					zfile = NULL;

					if (_tremove(theApp.ipfilter->GetDefaultFilePath()) != 0)
						TRACE("*** Error: Failed to remove default IP filter file \"%s\" - %s\n", theApp.ipfilter->GetDefaultFilePath(), _tcserror(errno));
					if (_trename(szTempUnzipFilePath, theApp.ipfilter->GetDefaultFilePath()) != 0)
						TRACE("*** Error: Failed to rename uncompressed IP filter file \"%s\" to default IP filter file \"%s\" - %s\n", szTempUnzipFilePath, theApp.ipfilter->GetDefaultFilePath(), _tcserror(errno));
					if (_tremove(szTempFilePath) != 0)
						TRACE("*** Error: Failed to remove temporary IP filter file \"%s\" - %s\n", szTempFilePath, _tcserror(errno));
					bUnzipped = true;
				}
				else
					LogError(LOG_STATUSBAR, _T("Failed to extract IP filter file from downloaded IP filter ZIP file \"%s\"."), szTempFilePath);
			}
			else
				LogError(LOG_STATUSBAR, _T("Downloaded IP filter file \"%s\" is a ZIP file with unexpected content."), szTempFilePath);

			zip.Close();
		}

		if (!bIsZipFile && !bUnzipped)
		{
			_tremove(theApp.ipfilter->GetDefaultFilePath());
			_trename(szTempFilePath, theApp.ipfilter->GetDefaultFilePath());
		}
	}
	OnReloadIPFilter();
}

void CPPgSecurity::DeleteDDB() {
	if (m_pacIPFilterURL){
		m_pacIPFilterURL->SaveList(CString(thePrefs.GetConfigDir()) + _T("\\") IPFILTERUPDATEURL_STRINGS_PROFILE);
		m_pacIPFilterURL->Unbind();
		m_pacIPFilterURL->Release();
	}
}

BOOL CPPgSecurity::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN){

		if (pMsg->wParam == VK_ESCAPE)
			return FALSE;

		if( m_pacIPFilterURL && m_pacIPFilterURL->IsBound() && ((pMsg->wParam == VK_DELETE) && (pMsg->hwnd == GetDlgItem(IDC_UPDATEURL)->m_hWnd) && (GetAsyncKeyState(VK_MENU)<0 || GetAsyncKeyState(VK_CONTROL)<0)) )
			m_pacIPFilterURL->Clear();

		if (pMsg->wParam == VK_RETURN){
			if (pMsg->hwnd == GetDlgItem(IDC_UPDATEURL)->m_hWnd){
				if (m_pacIPFilterURL && m_pacIPFilterURL->IsBound() ){
					CString strText;
					GetDlgItem(IDC_UPDATEURL)->GetWindowText(strText);
					if (!strText.IsEmpty()){
						GetDlgItem(IDC_UPDATEURL)->SetWindowText(_T("")); // this seems to be the only chance to let the dropdown list to disapear
						GetDlgItem(IDC_UPDATEURL)->SetWindowText(strText);
						((CEdit*)GetDlgItem(IDC_UPDATEURL))->SetSel(strText.GetLength(), strText.GetLength());
					}
				}
				return TRUE;
			}
		}
	}
   
	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CPPgSecurity::OnEnChangeUpdateUrl()
{
	CString strUrl;
	GetDlgItemText(IDC_UPDATEURL, strUrl);
	GetDlgItem(IDC_LOADURL)->EnableWindow(!strUrl.IsEmpty());
}

void CPPgSecurity::OnDDClicked() {
	
	CWnd* box=GetDlgItem(IDC_UPDATEURL);
	box->SetFocus();
	box->SetWindowText(_T(""));
	box->SendMessage(WM_KEYDOWN,VK_DOWN,0x00510001);
}

void CPPgSecurity::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Security);
}

BOOL CPPgSecurity::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}
	
BOOL CPPgSecurity::OnHelpInfo(HELPINFO* pHelpInfo)
{
	OnHelp();
	return TRUE;
}

void CPPgSecurity::OnBnClickedRunasuser()
{
	if ( ((CButton*)GetDlgItem(IDC_RUNASUSER))->GetCheck() == BST_CHECKED){
		if (AfxMessageBox(GetResString(IDS_RAU_WARNING),MB_OKCANCEL | MB_ICONINFORMATION,0) == IDCANCEL)
			((CButton*)GetDlgItem(IDC_RUNASUSER))->SetCheck(BST_UNCHECKED);
	}
	OnSettingsChange();
}
