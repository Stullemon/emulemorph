//this file is part of eMule morphXT
//Copyright (C)2006 leuk_he ( strEmail.Format("%s@%s", "leukhe", "gmail.com") / http://emulemorph.sf.net )
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
#include <string.h>
#include "emule.h"
#include "emuledlg.h"
#include "otherfunctions.h"
#include "PPGtooltipped.h" //MORPH leuk_he addded tooltipped
#include "WebServer.h"
#include "PPgIonixWebServer.h"
#include "NTService.h"
#include "PPgNTServer.h"
#include "PreferencesDlg.h"
#include "resource.h"
                 
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define HIDDEN_PASSWORD _T("*****")

IMPLEMENT_DYNAMIC(CPPgNTService, CPropertyPage)
CPPgNTService::CPPgNTService()
	:  CPPgtooltipped(CPPgNTService::IDD)
{
	m_bIsInit = false;
	// MORPH start tabbed option [leuk_he]
	m_imageList.DeleteImageList();
	m_imageList.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 14+1, 0);
	m_imageList.Add(CTempIconLoader(_T("CLIENTSONQUEUE")));
	// MORPH end tabbed option [leuk_he]
}

CPPgNTService::~CPPgNTService()
{
}


void CPPgNTService::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

  // MORPH start tabbed options [leuk_he]
   DDX_Control(pDX, IDC_TAB_SERVICE3 , m_tabCtr);
  // MORPH end tabbed options [leuk_he]

}

BEGIN_MESSAGE_MAP(CPPgNTService, CPropertyPage)
	ON_BN_CLICKED(IDC_SVC_INSTALLSERVICE, OnBnClickedInstall)	
	ON_BN_CLICKED(IDC_SVC_SERVERUNINSTALL, OnBnClickedUnInstall)	

    ON_BN_CLICKED(IDC_SVC_STARTWITHSYSTEM, OnBnStartSystem)	
	ON_BN_CLICKED(IDC_SVC_MANUALSTART, OnBnManualStart)	
	ON_BN_CLICKED(IDC_SVC_SETTINGS ,   OnBnAllSettings)	
	ON_BN_CLICKED(IDC_SVC_RUNBROWSER , OnBnRunBRowser)	
	ON_BN_CLICKED(IDC_SVC_REPLACESERVICE , OnBnReplaceStart)	
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_SERVICE3, OnTcnSelchangeTab)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()


BOOL CPPgNTService::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);
 
    // MORPH start tabbed options [leuk_he]
	InitTab(true,1);
	m_tabCtr.SetCurSel(theApp.emuledlg->preferenceswnd->StartPageWebServer);
    // MORPH end tabbed options [leuk_he]

	LoadSettings();
    InitTooltips(); //MORPH leuk_he tolltipped

	Localize();

	return TRUE;
}

void CPPgNTService::LoadSettings(void)
{
	if(m_hWnd)
	{
		if(thePrefs.GetServiceStartupMode()==2) 
		{
				CheckDlgButton(IDC_SVC_RUNBROWSER   ,BST_UNCHECKED );
				CheckDlgButton(IDC_SVC_REPLACESERVICE, BST_CHECKED);
		}
		else
		{
				CheckDlgButton(IDC_SVC_RUNBROWSER   ,BST_CHECKED );
                CheckDlgButton(IDC_SVC_REPLACESERVICE, BST_UNCHECKED);
		}


    	FillStatus();
		m_bIsInit = true;
	}
}



BOOL CPPgNTService::OnApply()
{	

    int b_installed;
	int  i_startupmode;
	int rights;
	// Startup with system, store in service.
	NTServiceGet(b_installed,i_startupmode,	rights);
    if (b_installed==1 && 
		  (i_startupmode ==0 && (IsDlgButtonChecked(IDC_SVC_STARTWITHSYSTEM)==BST_CHECKED))||
		  (i_startupmode ==1 && (IsDlgButtonChecked(IDC_SVC_MANUALSTART)==BST_CHECKED)))
			NTServiceSetStartupMode(IsDlgButtonChecked(IDC_SVC_STARTWITHSYSTEM)==BST_CHECKED);
   // TODO: Appy setting 
	if (IsDlgButtonChecked(IDC_SVC_SETTINGS )==BST_CHECKED) 	{
		thePrefs.startupsound=false;
		thePrefs.SetAutoConnect(true);
		thePrefs.SetWSIsEnabled(true); 
		RemAutoStart(); // remove from windows startup. 
		thePrefs.m_bEnableMiniMule=false;
		thePrefs.m_bSelCatOnAdd=false;
		thePrefs.notifierSoundType = ntfstNoSound;
		thePrefs.notifierOnDownloadFinished =false;
		thePrefs.splashscreen=false;
		thePrefs.startMinimized=true;
		thePrefs.beepOnError=false;
		thePrefs.bringtoforeground=false;
	}
	if ( IsDlgButtonChecked(IDC_SVC_RUNBROWSER)==BST_CHECKED)
	   thePrefs.m_iServiceStartupMode=1;
	else 
	   thePrefs.m_iServiceStartupMode=2;

	SetModified(FALSE);
	LoadSettings();
	return CPropertyPage::OnApply();
}

void CPPgNTService::Localize(void)
{
	if(m_hWnd){
		SetWindowText(_T("NT Service")  );
	GetDlgItem(IDC_SVC_INSTALLSERVICE)->SetWindowText(GetResString(IDS_SVC_INSTALLSERVICE));
GetDlgItem(IDC_SVC_SERVERUNINSTALL)->SetWindowText(GetResString(IDS_SVC_SERVERUNINSTALL));
GetDlgItem(IDC_SVC_STARTWITHSYSTEM)->SetWindowText(GetResString(IDS_SVC_STARTWITHSYSTEM));
GetDlgItem(IDC_SVC_MANUALSTART)->SetWindowText(GetResString(IDS_SVC_MANUALSTART));
GetDlgItem(IDC_SVC_SETTINGS)->SetWindowText(GetResString(IDS_SVC_SETTINGS));
GetDlgItem(IDC_SVC_RUNBROWSER)->SetWindowText(GetResString(IDS_SVC_RUNBROWSER));
GetDlgItem(IDC_SVC_REPLACESERVICE )->SetWindowText(GetResString(IDS_SVC_REPLACESERVICE));
GetDlgItem(IDC_SVC_ONSTARTBOX)->SetWindowText(GetResString(IDS_SVC_ONSTARTBOX));
GetDlgItem(IDS_SVC_STARTUPBOX)->SetWindowText(GetResString(IDS_SVC_STARTUPBOX));
SetTool(IDC_SVC_INSTALLSERVICE,IDS_SVC_INSTALLSERVICE_TIP);
SetTool(IDC_SVC_SERVERUNINSTALL,IDS_SVC_SERVERUNINSTALL_TIP);
SetTool(IDC_SVC_STARTWITHSYSTEM,IDS_SVC_STARTWITHSYSTEM_TIP);
SetTool(IDC_SVC_MANUALSTART,IDS_SVC_MANUALSTART_TIP);
SetTool(IDC_SVC_SETTINGS,IDS_SVC_SETTINGS_TIP);
SetTool(IDC_SVC_RUNBROWSER,IDS_SVC_RUNBROWSER_TIP);
SetTool(IDC_SVC_REPLACESERVICE ,IDS_SVC_REPLACESERVICE_TIP);
SetTool(IDC_SVC_CURRENT_STATUS,IDS_SVC_CURRENT_STATUS_TIP);

	}
}

void CPPgNTService::OnHelp()
{
//heApp.ShowHelp();
}

BOOL CPPgNTService::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgNTService::OnHelpInfo(HELPINFO* /*pHelpInfo */)
{
	OnHelp();
	return TRUE;
}



int  CPPgNTService::FillStatus(){
	int b_installed;
	int  i_startupmode;
	int rights;

	if (afxData.bWin95) {
		GetDlgItem( IDC_SVC_CURRENT_STATUS)->SetWindowText(GetResString(IDS_SVC_OSTOOOLD));
		GetDlgItem( IDC_SVC_INSTALLSERVICE)->EnableWindow(false); 
		GetDlgItem( IDC_SVC_SERVERUNINSTALL)->EnableWindow(false); 
        return -1;
	} 
		NTServiceGet(b_installed,i_startupmode,	rights);

		if (RunningAsService())		{ 
			GetDlgItem( IDC_SVC_CURRENT_STATUS)->SetWindowText(GetResString(IDS_SVC_RUNNINGASSERVICE));
			GetDlgItem( IDC_SVC_INSTALLSERVICE)->EnableWindow(false); // installing makes no sense when already running
			GetDlgItem( IDC_SVC_SERVERUNINSTALL)->EnableWindow(false); // cannot uninstall self
			if ( i_startupmode==1){
				CheckDlgButton(IDC_SVC_STARTWITHSYSTEM, BST_CHECKED);
				CheckDlgButton(IDC_SVC_MANUALSTART,     BST_UNCHECKED);
			}
			else {
				CheckDlgButton(IDC_SVC_STARTWITHSYSTEM, BST_UNCHECKED);
				CheckDlgButton(IDC_SVC_MANUALSTART,     BST_CHECKED);
			}
		}
		else {//This instance is not the running process
			if (b_installed==-1)// undetermined
			{
				GetDlgItem( IDC_SVC_INSTALLSERVICE)->EnableWindow(true); // probably fails but let user try
				GetDlgItem( IDC_SVC_SERVERUNINSTALL)->EnableWindow(true); // probably fails but let user try
				GetDlgItem( IDC_SVC_CURRENT_STATUS)->SetWindowText(GetResString(IDS_SVC_ACCESSDENIED)); 
			}
			else if (b_installed==0){
				GetDlgItem( IDC_SVC_CURRENT_STATUS)->SetWindowText(GetResString(IDS_SVC_NOTINSTALLED)); 
				GetDlgItem( IDC_SVC_INSTALLSERVICE)->EnableWindow(true); 
				GetDlgItem( IDC_SVC_SERVERUNINSTALL)->EnableWindow(false);
			}	else if (b_installed==1 && i_startupmode ==4 ){
				GetDlgItem( IDC_SVC_CURRENT_STATUS)->SetWindowText(GetResString(IDS_SVC_INSTALLED_DISABLED)); 
				GetDlgItem( IDC_SVC_INSTALLSERVICE)->EnableWindow(false); 
				GetDlgItem( IDC_SVC_SERVERUNINSTALL)->EnableWindow(true);
			}	else if (b_installed==1){
				GetDlgItem( IDC_SVC_CURRENT_STATUS)->SetWindowText(GetResString(IDS_SVC_INSTALLED)); 
				GetDlgItem( IDC_SVC_INSTALLSERVICE)->EnableWindow(false); 
 				GetDlgItem( IDC_SVC_SERVERUNINSTALL)->EnableWindow(true);
			}
			if(i_startupmode==1){
				CheckDlgButton(IDC_SVC_STARTWITHSYSTEM, BST_CHECKED);
				CheckDlgButton(IDC_SVC_MANUALSTART,     BST_UNCHECKED);
			}
			else{
				CheckDlgButton(IDC_SVC_STARTWITHSYSTEM, BST_UNCHECKED);
				CheckDlgButton(IDC_SVC_MANUALSTART,     BST_CHECKED);
			}
		}
		return 0;
}


void CPPgNTService::OnBnClickedInstall()
{
	if (CmdInstallService((IsDlgButtonChecked(IDC_SVC_STARTWITHSYSTEM))==BST_CHECKED )==0) {
	  FillStatus();
      CheckDlgButton(IDC_SVC_SETTINGS, BST_CHECKED);
 	  SetModified();
   }
 else
	 SetDlgItemText(IDC_SVC_CURRENT_STATUS,GetResString(IDS_SVC_INSTALLFAILED)); 
}


void CPPgNTService::OnBnClickedUnInstall()
{
	if (CmdRemoveService()==0) {
		FillStatus();
		SetModified();
	}
	else
		SetDlgItemText(IDC_SVC_CURRENT_STATUS,GetResString(IDS_SVC_UNINSTALLFAILED)); 

}

void CPPgNTService::OnBnStartSystem(){
	if(IsDlgButtonChecked(IDC_SVC_MANUALSTART)==BST_CHECKED )
		SetModified();
	CheckDlgButton(IDC_SVC_STARTWITHSYSTEM, BST_CHECKED);
	CheckDlgButton(IDC_SVC_MANUALSTART,     BST_UNCHECKED);
};	
void CPPgNTService::OnBnManualStart(){
	if(IsDlgButtonChecked(IDC_SVC_STARTWITHSYSTEM)==BST_CHECKED )
		SetModified();
	CheckDlgButton(IDC_SVC_STARTWITHSYSTEM,BST_UNCHECKED );
	CheckDlgButton(IDC_SVC_MANUALSTART,    BST_CHECKED);
};	

void CPPgNTService::OnBnAllSettings(){
	SetModified();
};	

void CPPgNTService::OnBnReplaceStart(){
	CheckDlgButton(IDC_SVC_RUNBROWSER   ,BST_UNCHECKED );
	CheckDlgButton(IDC_SVC_REPLACESERVICE, BST_CHECKED);
	SetModified();
};	

void CPPgNTService::OnBnRunBRowser(){
	CheckDlgButton(IDC_SVC_RUNBROWSER   ,BST_CHECKED );
	CheckDlgButton(IDC_SVC_REPLACESERVICE,    BST_UNCHECKED);
	SetModified();
};	


// MORPH start tabbed option [leuk_he]
void CPPgNTService::InitTab(bool firstinit, int Page)
{
	if (m_tabCtr.GetSafeHwnd() != NULL  && firstinit ) {
		m_tabCtr.DeleteAllItems();
		m_tabCtr.SetImageList(&m_imageList);
		m_tabCtr.InsertItem(TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM, WEBSERVER, _T("Web server"), 0, (LPARAM)WEBSERVER); 
		m_tabCtr.InsertItem(TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM, MULTIWEBSERVER, _T("Multi user"), 0, (LPARAM)MULTIWEBSERVER); // note that the string Multi user is REAL HARD coded 
		m_tabCtr.InsertItem(TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM, NTSERVICE   , _T("NT Service"), 0, (LPARAM)NTSERVICE); // note that the string Multi user is REAL HARD coded 
	}
	if (m_tabCtr.GetSafeHwnd() != NULL     )
		m_tabCtr.SetCurSel(Page);
}
void CPPgNTService::OnTcnSelchangeTab(NMHDR * /*pNMHDR */, LRESULT *pResult)
{
	int cur_sel = m_tabCtr.GetCurSel();
	theApp.emuledlg->preferenceswnd->SwitchTab(cur_sel);
	*pResult = 0;
}
// MORPH end tabbed option [leuk_he]

