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
#include "PreferencesDlg.h"
#include "Wizard.h"
#include "emuledlg.h"
#include "StatisticsDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// Wizard dialog

IMPLEMENT_DYNAMIC(Wizard, CDialog)
Wizard::Wizard(CWnd* pParent /*=NULL*/)
	: CDialog(Wizard::IDD, pParent)
{
	m_iBitByte = 0;
	m_iOS = 0;
	m_iTotalDownload = 0;
}

Wizard::~Wizard()
{
}

void Wizard::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROVIDERS, m_provider);
	DDX_Radio(pDX, IDC_WIZ_XP_RADIO, m_iOS);
	DDX_Radio(pDX, IDC_WIZ_LOWDOWN_RADIO, m_iTotalDownload);
	DDX_Radio(pDX, IDC_KBITS, m_iBitByte);
}

BEGIN_MESSAGE_MAP(Wizard, CDialog)
	ON_BN_CLICKED(IDC_WIZ_APPLY_BUTTON, OnBnClickedApply)
	ON_BN_CLICKED(IDC_WIZ_CANCEL_BUTTON, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_WIZ_XP_RADIO, OnBnClickedWizRadioOsNtxp)
	ON_BN_CLICKED(IDC_WIZ_ME_RADIO, OnBnClickedWizRadioUs98me)
	ON_BN_CLICKED(IDC_WIZ_LOWDOWN_RADIO, OnBnClickedWizLowdownloadRadio)
	ON_BN_CLICKED(IDC_WIZ_MEDIUMDOWN_RADIO, OnBnClickedWizMediumdownloadRadio)
	ON_BN_CLICKED(IDC_WIZ_HIGHDOWN_RADIO, OnBnClickedWizHighdownloadRadio)
	ON_NOTIFY(NM_CLICK, IDC_PROVIDERS, OnNMClickProviders)
END_MESSAGE_MAP()


// Wizard message handlers

void Wizard::OnBnClickedApply()
{
	char buffer[510];
	int upload, download;
	if(GetDlgItem(IDC_WIZ_TRUEDOWNLOAD_BOX)->GetWindowTextLength())
	{ 
		GetDlgItem(IDC_WIZ_TRUEDOWNLOAD_BOX)->GetWindowText(buffer,20);
		download = atoi(buffer);
	}
	else
	{
		download = 0;
 	}
	if(GetDlgItem(IDC_WIZ_TRUEUPLOAD_BOX)->GetWindowTextLength())
	{ 
		GetDlgItem(IDC_WIZ_TRUEUPLOAD_BOX)->GetWindowText(buffer,20);
		upload = atoi(buffer);
	}
	else
	{
		upload = 0;
	}

	if(IsDlgButtonChecked(IDC_KBITS)==1) {upload/=8;download/=8;}

	app_prefs->prefs->maxGraphDownloadRate = download;
	app_prefs->prefs->maxGraphUploadRate = upload;

	if( upload > 0 && download > 0 ){
		// Elandal: typesafe, integer math only
		// removes warning regarding implicit cast
		app_prefs->prefs->maxupload = (uint16)((upload * 4L) / 5);
		if( upload < 4 && download > upload*3 ){
			app_prefs->prefs->maxdownload = app_prefs->prefs->maxupload * 3;
			download = upload*3;
		}
		if( upload < 10 && download > upload*4 ){
			app_prefs->prefs->maxdownload = app_prefs->prefs->maxupload * 4;
			download = upload*4;
		}
		else
			// Elandal: typesafe, integer math only
			// removes warning regarding implicit cast
			app_prefs->prefs->maxdownload = (uint16)((download * 9L) / 10);

		theApp.emuledlg->statisticswnd->SetARange(false,app_prefs->prefs->maxGraphUploadRate);
		theApp.emuledlg->statisticswnd->SetARange(true,app_prefs->prefs->maxGraphDownloadRate);

		if( m_iOS == 1 )
			app_prefs->prefs->maxconnections = 50;
		else{
		if( upload <= 7 )	
			app_prefs->prefs->maxconnections = 80;
		else if( upload < 12 )
			app_prefs->prefs->maxconnections = 200;	
		else if( upload < 25 )
			app_prefs->prefs->maxconnections = 400;
		else if( upload < 37 )
			app_prefs->prefs->maxconnections = 600;
		else
			app_prefs->prefs->maxconnections = 800;	

		}
		if( m_iOS == 1 )
			download = download/2;

		if( download <= 7 ){
			switch( m_iTotalDownload ){
				case 0:
					app_prefs->prefs->maxsourceperfile = 100;
				break;
				case 1:
					app_prefs->prefs->maxsourceperfile = 60;
				break;
				case 2:
					app_prefs->prefs->maxsourceperfile = 40;
				break;
			}
		}
		else if( download < 62 ){
			switch( m_iTotalDownload ){
				case 0:
					app_prefs->prefs->maxsourceperfile = 300;
				break;
				case 1:
					app_prefs->prefs->maxsourceperfile = 200;
				break;
				case 2:
					app_prefs->prefs->maxsourceperfile = 100;
				break;
			}
		}
		else if( download < 187 ){
			switch( m_iTotalDownload ){
				case 0:
					app_prefs->prefs->maxsourceperfile = 500;
				break;
				case 1:
					app_prefs->prefs->maxsourceperfile = 400;
				break;
				case 2:
					app_prefs->prefs->maxsourceperfile = 350;
				break;
			}
		}
		else if( download <= 312 ){
			switch( m_iTotalDownload ){
				case 0:
					app_prefs->prefs->maxsourceperfile = 800;
				break;
				case 1:
					app_prefs->prefs->maxsourceperfile = 600;
				break;
				case 2:
					app_prefs->prefs->maxsourceperfile = 400;
				break;
			}
		}
		else {
			switch( m_iTotalDownload ){
			case 0:
				app_prefs->prefs->maxsourceperfile = 1000;
				break;
			case 1:
				app_prefs->prefs->maxsourceperfile = 750;
				break;
			case 2:
				app_prefs->prefs->maxsourceperfile = 500;
				break;
			}
		}
	}
	theApp.emuledlg->preferenceswnd->m_wndConnection.LoadSettings();
	CDialog::OnOK();
}

void Wizard::OnBnClickedCancel()
{
	CDialog::OnCancel();
}

void Wizard::OnBnClickedWizRadioOsNtxp()
{
	m_iOS = 0;
}

void Wizard::OnBnClickedWizRadioUs98me()
{
	m_iOS = 1;
}

void Wizard::OnBnClickedWizLowdownloadRadio()
{
	m_iTotalDownload = 0;
}

void Wizard::OnBnClickedWizMediumdownloadRadio()
{
	m_iTotalDownload = 1;
}

void Wizard::OnBnClickedWizHighdownloadRadio()
{
	m_iTotalDownload = 2;
}

void Wizard::OnBnClickedWizResetButton()
{
	CString strBuffer;
	strBuffer.Format("%i", 0);
	GetDlgItem(IDC_WIZ_TRUEDOWNLOAD_BOX)->SetWindowText(strBuffer); 
	GetDlgItem(IDC_WIZ_TRUEUPLOAD_BOX)->SetWindowText(strBuffer); 
}

BOOL Wizard::OnInitDialog(){
	CDialog::OnInitDialog();
	InitWindowStyles(this);

	if (::DetectWinVersion()== _WINVER_95_ || ::DetectWinVersion()==_WINVER_98_ || ::DetectWinVersion()==_WINVER_ME_){
		this->CheckDlgButton(IDC_WIZ_XP_RADIO,0);
		this->CheckDlgButton(IDC_WIZ_ME_RADIO,1);
		m_iOS = 1;
	}
	else{
		this->CheckDlgButton(IDC_WIZ_ME_RADIO,0);
		this->CheckDlgButton(IDC_WIZ_XP_RADIO,1);
		m_iOS = 0;
	}
	this->CheckDlgButton(IDC_WIZ_LOWDOWN_RADIO,1);
	this->CheckDlgButton(IDC_KBITS,1);
	this->CheckDlgButton(IDC_KBYTES,0);

	CString temp;
	temp.Format("%u",app_prefs->prefs->maxGraphDownloadRate *8);	GetDlgItem(IDC_WIZ_TRUEDOWNLOAD_BOX)->SetWindowText(temp); 
	temp.Format("%u",app_prefs->prefs->maxGraphUploadRate*8);GetDlgItem(IDC_WIZ_TRUEUPLOAD_BOX)->SetWindowText(temp); 

	m_provider.InsertColumn(0,GetResString(IDS_PW_CONNECTION),LVCFMT_LEFT, 160);
	m_provider.InsertColumn(1,GetResString(IDS_WIZ_DOWN),LVCFMT_LEFT, 85);
	m_provider.InsertColumn(2,GetResString(IDS_WIZ_UP),LVCFMT_LEFT, 85);

	m_provider.InsertItem(0,GetResString(IDS_WIZARD_CUSTOM) );m_provider.SetItemText(0,1,GetResString(IDS_WIZARD_ENTERBELOW));m_provider.SetItemText(0,2,GetResString(IDS_WIZARD_ENTERBELOW));
	m_provider.InsertItem(1,"56-k Modem");m_provider.SetItemText(1,1,"56");m_provider.SetItemText(1,2,"56");
	m_provider.InsertItem(2,"ISDN");m_provider.SetItemText(2,1,"64");m_provider.SetItemText(2,2,"64");
	m_provider.InsertItem(3,"ISDN 2x");m_provider.SetItemText(3,1,"128");m_provider.SetItemText(3,2,"128");
	m_provider.InsertItem(4 ,"DSL");m_provider.SetItemText(4,1,"256");m_provider.SetItemText(4,2,"128");
	m_provider.InsertItem(5,"DSL");m_provider.SetItemText(5,1,"384");m_provider.SetItemText(5,2,"91");
	m_provider.InsertItem(6,"DSL");m_provider.SetItemText(6,1,"512");m_provider.SetItemText(6,2,"91");
	m_provider.InsertItem(7 ,"DSL");m_provider.SetItemText(7,1,"512");m_provider.SetItemText(7,2,"128");
	m_provider.InsertItem(8,"DSL");m_provider.SetItemText(8,1,"640");m_provider.SetItemText(8,2,"90");
	m_provider.InsertItem(9,"DSL (T-DSL, newDSL, 1&1-DSL");m_provider.SetItemText(9,1,"768");m_provider.SetItemText(9,2,"128");
	m_provider.InsertItem(10,"DSL (QDSL, NGI-DSL");m_provider.SetItemText(10,1,"1024");m_provider.SetItemText(10,2,"256");
	m_provider.InsertItem(11,"DSL 1500 ('TDSL 1500')");m_provider.SetItemText(11,1,"1500");m_provider.SetItemText(11,2,"192");
	m_provider.InsertItem(12,"DSL 1600");m_provider.SetItemText(12,1,"1600");m_provider.SetItemText(12,2,"90");
	m_provider.InsertItem(13,"DSL 2000");m_provider.SetItemText(13,1,"2000");m_provider.SetItemText(13,2,"300");
	m_provider.InsertItem(14,"Cable");m_provider.SetItemText(14,1,"187");m_provider.SetItemText(14,2,"32");
	m_provider.InsertItem(15,"Cable");m_provider.SetItemText(15,1,"187");m_provider.SetItemText(15,2,"64");
	m_provider.InsertItem(16,"T1");m_provider.SetItemText(16,1,"1500");m_provider.SetItemText(16,2,"1500");
	m_provider.InsertItem(17,"T3+");m_provider.SetItemText(17,1,"44 Mbps");m_provider.SetItemText(17,2,"44 Mbps");


	m_provider.SetSelectionMark(0);

	Localize();

	return TRUE;
}

void Wizard::Localize(void){
	GetDlgItem(IDC_WIZ_OS_FRAME)->SetWindowText(GetResString(IDS_WIZ_OS_FRAME));
	GetDlgItem(IDC_WIZ_TRUEUPLOAD_TEXT)->SetWindowText(GetResString(IDS_WIZ_TRUEUPLOAD_TEXT));
	GetDlgItem(IDC_WIZ_TRUEDOWNLOAD_TEXT)->SetWindowText(GetResString(IDS_WIZ_TRUEDOWNLOAD_TEXT));
	GetDlgItem(IDC_WIZ_APPLY_BUTTON)->SetWindowText(GetResString(IDS_PW_APPLY));
	GetDlgItem(IDC_WIZ_CANCEL_BUTTON)->SetWindowText(GetResString(IDS_CANCEL));
	GetDlgItem(IDC_WIZ_HOTBUTTON_FRAME)->SetWindowText(GetResString(IDS_WIZ_CTFRAME));
	GetDlgItem(IDC_CTINFO)->SetWindowText(GetResString(IDS_CTINFO));

	GetDlgItem(IDC_CTINFO)->SetWindowText(GetResString(IDS_CTINFO));
	GetDlgItem(IDC_CTINFO)->SetWindowText(GetResString(IDS_CTINFO));

	GetDlgItem(IDC_KBITS)->SetWindowText(GetResString(IDS_KBITSSEC));
	GetDlgItem(IDC_KBYTES)->SetWindowText(GetResString(IDS_KBYTESSEC));

	GetDlgItem(IDC_WIZ_CONCURENTDOWN_FRAME)->SetWindowText(GetResString(IDS_CONCURDWL));
	GetDlgItem(IDC_UNIT)->SetWindowText(GetResString(IDS_UNIT));

	SetWindowText(GetResString(IDS_WIZARD));
}

void Wizard::SetCustomItemsActivation() {
	BOOL active=(m_provider.GetSelectionMark()<1);

	GetDlgItem(IDC_WIZ_TRUEUPLOAD_BOX)->EnableWindow(active);
	GetDlgItem(IDC_WIZ_TRUEDOWNLOAD_BOX )->EnableWindow(active);
	GetDlgItem(IDC_KBITS )->EnableWindow(active);
	GetDlgItem(IDC_KBYTES )->EnableWindow(active);
}

void Wizard::OnNMClickProviders(NMHDR *pNMHDR, LRESULT *pResult)
{
	SetCustomItemsActivation();
	uint16 up,down;
	switch (m_provider.GetSelectionMark()) {
		case 1 : down=56;up=33; break;
		case 2 : down=64;up=64; break;
		case 3 : down=128;up=128; break;
		case 4 : down=256;up=128; break;
		case 5 : down=384;up=91; break;
		case 6 : down=512;up=91; break;
		case 7 : down=512;up=128; break;
		case 8 : down=640;up=90; break;
		case 9 : down=768;up=128; break;
		case 10 : down=1024;up=256; break;
		case 11 : down=1500;up=192; break;
		case 12: down=1600;up=90; break;
		case 13: down=2000;up=300; break;
		case 14: down=187;up=32; break;
		case 15: down=187;up=64; break;
		case 16: down=1500;up=1500; break;
		case 17: down=44000;up=44000; break;
		
		default: return;
	}
	CString temp;
	temp.Format("%u",down);	GetDlgItem(IDC_WIZ_TRUEDOWNLOAD_BOX)->SetWindowText(temp); 
	temp.Format("%u",up);GetDlgItem(IDC_WIZ_TRUEUPLOAD_BOX)->SetWindowText(temp); 
	this->CheckDlgButton(IDC_KBITS,1);
	this->CheckDlgButton(IDC_KBYTES,0);

	*pResult = 0;
}
