// KademliaWnd.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "KademliaWnd.h"
#include "KadContactListCtrl.h"
#include "KadSearchListCtrl.h"
#include "Kademlia/routing/timer.h"
#include "Kademlia/Kademlia/kademlia.h"
#include "Kademlia/net/kademliaudplistener.h"
#include "Ini2.h"

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
		m_pacONBSIPs->SaveList(CString(theApp.glob_prefs->GetConfigDir()) + _T("\\") ONBOOTSTRAP_STRINGS_PROFILE);

	CString strIniFile;
	strIniFile.Format(_T("%spreferences.ini"), theApp.glob_prefs->GetConfigDir());
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

	Localize();

	if (theApp.glob_prefs->GetUseAutocompletion()){
		m_pacONBSIPs = new CCustomAutoComplete();
		m_pacONBSIPs->AddRef();
		if (m_pacONBSIPs->Bind(::GetDlgItem(m_hWnd, IDC_BOOTSTRAPIP), ACO_UPDOWNKEYDROPSLIST | ACO_AUTOSUGGEST | ACO_FILTERPREFIXES ))
			m_pacONBSIPs->LoadList(CString(theApp.glob_prefs->GetConfigDir()) +  _T("\\") ONBOOTSTRAP_STRINGS_PROFILE);
	}

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
	ON_STN_CLICKED(IDC_KADCONTACTLAB, OnStnClickedKadcontactlab)
END_MESSAGE_MAP()


///////////////////////////////////////////////////////////////////////////////
//

void CKademliaWnd::OnBnClickedBootstrapbutton()
{
	if(!Kademlia::CTimer::getThreadID())
		return;

	uint16 tempVal=0;
	CString strBuffer;
	char buffer[510];

	GetDlgItem(IDC_BOOTSTRAPIP)->GetWindowText(strBuffer);

	// handle ip:port
	if (strBuffer.Find(':')!=-1) {
		int pos=strBuffer.Find(':');

		GetDlgItem(IDC_BOOTSTRAPPORT)->SetWindowText( strBuffer.Right( strBuffer.GetLength()-pos-1));
		GetDlgItem(IDC_BOOTSTRAPIP)->SetWindowText( strBuffer.Left(pos));
		strBuffer=strBuffer.Right( strBuffer.GetLength()-pos-1);
	}

	GetDlgItem(IDC_BOOTSTRAPPORT)->GetWindowText(buffer,10);
	tempVal= atoi(buffer);

	if (strBuffer.GetLength()<7 || tempVal<1) return;

	if (m_pacONBSIPs && m_pacONBSIPs->IsBound())
		m_pacONBSIPs->AddItem(strBuffer +":"+CString(buffer) ,0);

	theApp.kademlia->Bootstrap(strBuffer,tempVal);
}

void CKademliaWnd::OnBnClickedFirewallcheckbutton()
{
	if(Kademlia::CTimer::getThreadID() && Kademlia::CKademlia::getPrefs()->getFirewalled()){
		Kademlia::CKademlia::getPrefs()->setRecheckIP();
	}
}

void CKademliaWnd::OnBnConnect() {
	if (theApp.kademlia->isConnected() )
		theApp.kademlia->DisConnect();
	else if( Kademlia::CTimer::getThreadID() )
		theApp.kademlia->DisConnect();
	else
		theApp.kademlia->Connect();
}

void CKademliaWnd::OnSysColorChange()
{
	Localize();
	CResizableDialog::OnSysColorChange();
}

void CKademliaWnd::Localize() {
	GetDlgItem(IDC_BSSTATIC)->SetWindowText(GetResString(IDS_BOOTSTRAP));
	GetDlgItem(IDC_BOOTSTRAPBUTTON)->SetWindowText(GetResString(IDS_BOOTSTRAP));
	GetDlgItem(IDC_SSTATIC4)->SetWindowText(GetResString(IDS_SV_ADDRESS));
	GetDlgItem(IDC_SSTATIC7)->SetWindowText(GetResString(IDS_SV_PORT));
	GetDlgItem(IDC_FIREWALLCHECKBUTTON)->SetWindowText(GetResString(IDS_KAD_RECHECKFW));
	
	SetDlgItemText(IDC_KADCONTACTLAB,GetResString(IDS_KADCONTACTLAB));
	SetDlgItemText(IDC_KADSEARCHLAB,GetResString(IDS_KADSEARCHLAB));

	UpdateControlsState();
	contactList->Localize();
	searchList->Localize();
}

void CKademliaWnd::UpdateControlsState() {
	if( theApp.kademlia->isConnected() )
	{
		GetDlgItem(IDC_KADCONNECT)->SetWindowText( GetResString(IDS_MAIN_BTN_DISCONNECT ) );
		GetDlgItem(IDC_BOOTSTRAPBUTTON)->EnableWindow(false);
	}
	else if( Kademlia::CTimer::getThreadID() )
	{
		GetDlgItem(IDC_KADCONNECT)->SetWindowText( GetResString(IDS_MAIN_BTN_CANCEL));
		GetDlgItem(IDC_BOOTSTRAPBUTTON)->EnableWindow(true);
	}
	else
	{
		GetDlgItem(IDC_KADCONNECT)->SetWindowText( GetResString(IDS_MAIN_BTN_CONNECT ) );
		GetDlgItem(IDC_BOOTSTRAPBUTTON)->EnableWindow(false);
	}
	if (!theApp.glob_prefs->GetNetworkKademlia())
		GetDlgItem(IDC_KADCONNECT)->EnableWindow(FALSE);
}

void CKademliaWnd::OnStnClickedKadcontactlab()
{
	// TODO: Add your control notification handler code here
}
