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
#include "KademliaWnd.h"
#include "KadContactListCtrl.h"
#include "KadSearchListCtrl.h"
#include "Kademlia/Kademlia/kademlia.h"
#include "Kademlia/Kademlia/prefs.h"
#include "Kademlia/net/kademliaudplistener.h"
#include "Ini2.h"
#include "CustomAutoComplete.h"
#include "OtherFunctions.h"
#include "emuledlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define	ONBOOTSTRAP_STRINGS_PROFILE	_T("AC_BootstrapIPs.dat")

// KademliaWnd dialog

IMPLEMENT_DYNAMIC(CKademliaWnd, CDialog)
CKademliaWnd::CKademliaWnd(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CKademliaWnd::IDD, pParent)
{
	contactList = new CKadContactListCtrl;
	searchList = new CKadSearchListCtrl;
	m_pacONBSIPs = NULL;
}

CKademliaWnd::~CKademliaWnd()
{
	if (m_pacONBSIPs){
		m_pacONBSIPs->Unbind();
		m_pacONBSIPs->Release();
	}
	delete contactList;
	delete searchList;
}

BOOL CKademliaWnd::SaveAllSettings()
{
	if (m_pacONBSIPs)
		m_pacONBSIPs->SaveList(CString(thePrefs.GetConfigDir()) + _T("\\") ONBOOTSTRAP_STRINGS_PROFILE);

	CString strIniFile;
	strIniFile.Format(_T("%spreferences.ini"), thePrefs.GetConfigDir());
	CIni ini(strIniFile, "eMule");

	contactList->SaveAllSettings(&ini);
	searchList->SaveAllSettings(&ini);

	return TRUE;
}

BOOL CKademliaWnd::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);
	contactList->Init();
	searchList->Init();

	AddAnchor(IDC_CONTACTLIST,TOP_LEFT, CSize(100,50));
	AddAnchor(IDC_SEARCHLIST,CSize(0,50),CSize(100,100));

	AddAnchor(IDC_KADCONTACTLAB,TOP_LEFT);
	AddAnchor(IDC_FIREWALLCHECKBUTTON, TOP_RIGHT);
	AddAnchor(IDC_KADCONNECT, TOP_RIGHT);
	AddAnchor(IDC_KADSEARCHLAB,CSize(0,50));
	AddAnchor(IDC_BSSTATIC, TOP_RIGHT);
	AddAnchor(IDC_BOOTSTRAPBUTTON, TOP_RIGHT);
	AddAnchor(IDC_BOOTSTRAPPORT, TOP_RIGHT);
	AddAnchor(IDC_BOOTSTRAPIP, TOP_RIGHT);
	AddAnchor(IDC_SSTATIC4, TOP_RIGHT);
	AddAnchor(IDC_SSTATIC7, TOP_RIGHT);
	AddAnchor(IDC_RADCLIENTS, TOP_RIGHT);
	AddAnchor(IDC_RADIP, TOP_RIGHT);

	SetAllIcons();
	Localize();

	if (thePrefs.GetUseAutocompletion()){
		m_pacONBSIPs = new CCustomAutoComplete();
		m_pacONBSIPs->AddRef();
		if (m_pacONBSIPs->Bind(::GetDlgItem(m_hWnd, IDC_BOOTSTRAPIP), ACO_UPDOWNKEYDROPSLIST | ACO_AUTOSUGGEST | ACO_FILTERPREFIXES ))
			m_pacONBSIPs->LoadList(CString(thePrefs.GetConfigDir()) +  _T("\\") ONBOOTSTRAP_STRINGS_PROFILE);
	}

	CheckDlgButton(IDC_RADCLIENTS,1);

	return true;
}

void CKademliaWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CONTACTLIST, *contactList);
	DDX_Control(pDX, IDC_SEARCHLIST, *searchList);
	DDX_Control(pDX, IDC_KADCONTACTLAB, kadContactLab);
	DDX_Control(pDX, IDC_KADSEARCHLAB, kadSearchLab);
}


BEGIN_MESSAGE_MAP(CKademliaWnd, CResizableDialog)
	ON_BN_CLICKED(IDC_BOOTSTRAPBUTTON, OnBnClickedBootstrapbutton)
	ON_BN_CLICKED(IDC_FIREWALLCHECKBUTTON, OnBnClickedFirewallcheckbutton)
	ON_BN_CLICKED(IDC_KADCONNECT, OnBnConnect)
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()


///////////////////////////////////////////////////////////////////////////////
//

void CKademliaWnd::OnBnClickedBootstrapbutton()
{
	Kademlia::CKademlia::start();

	uint16 tempVal=0;
	CString strBuffer;
	GetDlgItem(IDC_BOOTSTRAPIP)->GetWindowText(strBuffer);

	// auto-handle ip:port
	if (strBuffer.Find(':')!=-1) {
		int pos=strBuffer.Find(':');
		GetDlgItem(IDC_BOOTSTRAPPORT)->SetWindowText( strBuffer.Right( strBuffer.GetLength()-pos-1));
		GetDlgItem(IDC_BOOTSTRAPIP)->SetWindowText( strBuffer.Left(pos));
		strBuffer=strBuffer.Right( strBuffer.GetLength()-pos-1);
	}

	char buffer[7];
	GetDlgItem(IDC_BOOTSTRAPPORT)->GetWindowText(buffer,6);
	tempVal= atoi(buffer);

	// boot by kad-client
	if ( IsDlgButtonChecked(IDC_RADCLIENTS) )
	{
		Kademlia::CKademlia::bootstrap(strBuffer,tempVal);
		return;
	}

	// invalid IP/Port
	if (strBuffer.GetLength()<7 || tempVal<1)
		return;

	if (m_pacONBSIPs && m_pacONBSIPs->IsBound())
		m_pacONBSIPs->AddItem(strBuffer +":"+CString(buffer) ,0);

	Kademlia::CKademlia::bootstrap(strBuffer,tempVal);
}

void CKademliaWnd::OnBnClickedFirewallcheckbutton()
{
	if(Kademlia::CKademlia::isRunning())
	{
		Kademlia::CKademlia::getPrefs()->setRecheckIP();
	}
}

void CKademliaWnd::OnBnConnect()
{
	if (Kademlia::CKademlia::isConnected())
		Kademlia::CKademlia::stop();
	else if (Kademlia::CKademlia::isRunning())
		Kademlia::CKademlia::stop();
	else
		Kademlia::CKademlia::start();
	theApp.emuledlg->ShowConnectionState();
}

void CKademliaWnd::OnSysColorChange()
{
	CResizableDialog::OnSysColorChange();
	SetAllIcons();
}

void CKademliaWnd::SetAllIcons()
{
}

void CKademliaWnd::Localize() {
	GetDlgItem(IDC_BSSTATIC)->SetWindowText(GetResString(IDS_BOOTSTRAP));
	GetDlgItem(IDC_BOOTSTRAPBUTTON)->SetWindowText(GetResString(IDS_BOOTSTRAP));
	GetDlgItem(IDC_SSTATIC4)->SetWindowText(GetResString(IDS_SV_ADDRESS) + _T(":"));
	GetDlgItem(IDC_SSTATIC7)->SetWindowText(GetResString(IDS_SV_PORT) + _T(":"));
	GetDlgItem(IDC_FIREWALLCHECKBUTTON)->SetWindowText(GetResString(IDS_KAD_RECHECKFW));
	
	SetDlgItemText(IDC_KADCONTACTLAB,GetResString(IDS_KADCONTACTLAB));
	SetDlgItemText(IDC_KADSEARCHLAB,GetResString(IDS_KADSEARCHLAB));

	SetDlgItemText(IDC_RADCLIENTS,GetResString(IDS_RADCLIENTS));

	UpdateControlsState();
	contactList->Localize();
	searchList->Localize();
}

void CKademliaWnd::UpdateControlsState()
{
	CString strLabel;
	if (Kademlia::CKademlia::isConnected())
		strLabel = GetResString(IDS_MAIN_BTN_DISCONNECT);
	else if (Kademlia::CKademlia::isRunning())
		strLabel = GetResString(IDS_MAIN_BTN_CANCEL);
	else
		strLabel = GetResString(IDS_MAIN_BTN_CONNECT);
	strLabel.Remove(_T('&'));
	GetDlgItem(IDC_KADCONNECT)->SetWindowText(strLabel);
	GetDlgItem(IDC_BOOTSTRAPBUTTON)->EnableWindow(!Kademlia::CKademlia::isConnected());
}
