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

// ClientDetailDialog.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "ClientDetailDialog.h"
#include "otherfunctions.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CClientDetailDialog dialog

IMPLEMENT_DYNAMIC(CClientDetailDialog, CDialog)
CClientDetailDialog::CClientDetailDialog(CUpDownClient* client)
	: CDialog(CClientDetailDialog::IDD, 0)
{
	m_client = client;
}

CClientDetailDialog::~CClientDetailDialog()
{
}

void CClientDetailDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CClientDetailDialog::OnInitDialog(){
	CDialog::OnInitDialog();
	InitWindowStyles(this);
	Localize();
	CString buffer;
	if (m_client->GetUserName())
		GetDlgItem(IDC_DNAME)->SetWindowText(m_client->GetUserName());
	else
		GetDlgItem(IDC_DNAME)->SetWindowText("?");
	
	if (m_client->HasValidHash()){
		buffer ="";
		CString buffer2;
		for (uint16 i = 0;i != 16;i++){
			buffer2.Format("%02X",m_client->GetUserHash()[i]);
			buffer+=buffer2;
		}
		GetDlgItem(IDC_DHASH)->SetWindowText(buffer);
	}
	else
		GetDlgItem(IDC_DHASH)->SetWindowText("?");
	
	switch(m_client->GetClientSoft()){
		case SO_UNKNOWN:
			GetDlgItem(IDC_DSOFT)->SetWindowText("?");
			GetDlgItem(IDC_DVERSION)->SetWindowText("?");
			break;
		case SO_EMULE:
		case SO_OLDEMULE:
			GetDlgItem(IDC_DSOFT)->SetWindowText("eMule");
			if(m_client->GetMuleVersion() == 0x99)
			{
				buffer.Format("v%u.%u%c", m_client->GetMajVersion(), m_client->GetMinVersion(), _T('a') + m_client->GetUpVersion());
			}
			else
			{
				buffer.Format("v%u.%u", m_client->GetMajVersion(), m_client->GetMinVersion());
			}
			//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
			if(m_client->GetClientModString().IsEmpty() == false){
				buffer += _T(" [");
				buffer += m_client->GetClientModString();
				buffer += _T("]");
			}
			//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
			GetDlgItem(IDC_DVERSION)->SetWindowText(buffer);
			break;
		case SO_CDONKEY:
			GetDlgItem(IDC_DSOFT)->SetWindowText("cDonkey");
			if(m_client->GetMuleVersion() == 0x99)
			{
				buffer.Format("v%u.%u%c", m_client->GetMajVersion(), m_client->GetMinVersion(), _T('a') + m_client->GetUpVersion());
			}
			else
			{
				buffer.Format("v%u.%u", m_client->GetMajVersion(), m_client->GetMinVersion());
			}
			GetDlgItem(IDC_DVERSION)->SetWindowText(buffer);
			break;
		case SO_SHAREAZA:
			GetDlgItem(IDC_DSOFT)->SetWindowText("Shareaza");
			if(m_client->GetMuleVersion() == 0x99)
			{
				buffer.Format("v%u.%u%c", m_client->GetMajVersion(), m_client->GetMinVersion(), _T('a') + m_client->GetUpVersion());
			}
			else
			{
				buffer.Format("v%u.%u", m_client->GetMajVersion(), m_client->GetMinVersion());
			}
			GetDlgItem(IDC_DVERSION)->SetWindowText(buffer);
			break;
		case SO_XMULE:
			GetDlgItem(IDC_DSOFT)->SetWindowText("xMule");
			if(m_client->GetMuleVersion() == 0x99)
			{
				buffer.Format("v%u.%u%c", m_client->GetMajVersion(), m_client->GetMinVersion(), _T('a') + m_client->GetUpVersion());
			}
			else
			{
				buffer.Format("v%u.%u", m_client->GetMajVersion(), m_client->GetMinVersion());
			}
 			//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
			if(m_client->GetClientModString().IsEmpty() == false){
				buffer += _T(" [");
				buffer += m_client->GetClientModString();
				buffer += _T("]");
			}
			//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
			GetDlgItem(IDC_DVERSION)->SetWindowText(buffer);
			break;
		case SO_EDONKEY:
			GetDlgItem(IDC_DSOFT)->SetWindowText("eDonkey");
			buffer.Format("v%u",m_client->GetVersion());
			GetDlgItem(IDC_DVERSION)->SetWindowText(buffer);
			break;
		case SO_EDONKEYHYBRID:
			GetDlgItem(IDC_DSOFT)->SetWindowText("eDonkeyHybrid");
			buffer.Format("v%u",m_client->GetVersion());
			GetDlgItem(IDC_DVERSION)->SetWindowText(buffer);
			break;
		case SO_MLDONKEY:
			GetDlgItem(IDC_DSOFT)->SetWindowText("MLdonkey");
			if(m_client->GetMuleVersion() == 0x99)
			{
				buffer.Format("v%u.%u%c", m_client->GetMajVersion(), m_client->GetMinVersion(), _T('a') + m_client->GetUpVersion());
			}
			else
			{
				buffer.Format("v%u.%u", m_client->GetMajVersion(), m_client->GetMinVersion());
			}
			GetDlgItem(IDC_DVERSION)->SetWindowText(buffer);
	}

	buffer.Format("%s",(m_client->HasLowID() ? GetResString(IDS_IDLOW):GetResString(IDS_IDHIGH)));
	GetDlgItem(IDC_DID)->SetWindowText(buffer);
	
	if (m_client->GetServerIP()){
		in_addr server;
		server.S_un.S_addr = m_client->GetServerIP();
		GetDlgItem(IDC_DSIP)->SetWindowText(inet_ntoa(server));
		
		CServer* cserver = theApp.serverlist->GetServerByAddress(inet_ntoa(server), m_client->GetServerPort()); 
		if (cserver)
			GetDlgItem(IDC_DSNAME)->SetWindowText(cserver->GetListName());
		else
			GetDlgItem(IDC_DSNAME)->SetWindowText("?");
	}
	else{
		GetDlgItem(IDC_DSIP)->SetWindowText("?");
		GetDlgItem(IDC_DSNAME)->SetWindowText("?");
	}

	CKnownFile* file = theApp.sharedfiles->GetFileByID(m_client->GetUploadFileID());
	if (file)
		GetDlgItem(IDC_DDOWNLOADING)->SetWindowText(MakeStringEscaped(file->GetFileName()));
	else
		GetDlgItem(IDC_DDOWNLOADING)->SetWindowText("-");

	if (m_client->reqfile)
		GetDlgItem(IDC_UPLOADING)->SetWindowText( m_client->reqfile->GetFileName()  );
	else 
		GetDlgItem(IDC_UPLOADING)->SetWindowText("-");

	GetDlgItem(IDC_DDUP)->SetWindowText(CastItoXBytes(m_client->GetTransferedDown()));

	GetDlgItem(IDC_DDOWN)->SetWindowText(CastItoXBytes(m_client->GetTransferedUp()));

	//wistily
	/*
	buffer.Format("%.1f %s",(float)m_client->GetDownloadDatarate()/1024,GetResString(IDS_KBYTESEC));
	GetDlgItem(IDC_DAVUR)->SetWindowText(buffer);
	*/
	buffer.Format("%.1f %s",(float)m_client->GetAvDownDatarate()/1024,GetResString(IDS_KBYTESEC));
	GetDlgItem(IDC_DAVUR)->SetWindowText(buffer);

	/*
	buffer.Format("%.1f %s",(float)m_client->GetDatarate()/1024,GetResString(IDS_KBYTESEC));
	GetDlgItem(IDC_DAVDR)->SetWindowText(buffer);
	*/
	buffer.Format("%.1f %s",(float)m_client->GetAvUpDatarate()/1024,GetResString(IDS_KBYTESEC));
	GetDlgItem(IDC_DAVDR)->SetWindowText(buffer);
	//wistily stop
	
	if (m_client->Credits()){
		GetDlgItem(IDC_DUPTOTAL)->SetWindowText(CastItoXBytes(m_client->Credits()->GetDownloadedTotal()));
		GetDlgItem(IDC_DDOWNTOTAL)->SetWindowText(CastItoXBytes(m_client->Credits()->GetUploadedTotal()));
		buffer.Format("%.1f  [%.1f]",(float)m_client->Credits()->GetScoreRatio(m_client->GetIP()),(float)m_client->Credits()->GetMyScoreRatio(m_client->GetIP()));	// MORPH - Added by IceCream, VQB: ownCredits
		GetDlgItem(IDC_DRATIO)->SetWindowText(buffer);
		
		if (theApp.clientcredits->CryptoAvailable()){
			switch(m_client->Credits()->GetCurrentIdentState(m_client->GetIP())){
				case IS_NOTAVAILABLE:
					GetDlgItem(IDC_CDIDENT)->SetWindowText(GetResString(IDS_IDENTNOSUPPORT));
					break;
				case IS_IDFAILED:
				case IS_IDNEEDED:
				case IS_IDBADGUY:
					GetDlgItem(IDC_CDIDENT)->SetWindowText(GetResString(IDS_IDENTFAILED));
					break;
				case IS_IDENTIFIED:
					GetDlgItem(IDC_CDIDENT)->SetWindowText(GetResString(IDS_IDENTOK));
					break;
			}
		}
		else
			GetDlgItem(IDC_CDIDENT)->SetWindowText(GetResString(IDS_IDENTNOSUPPORT));
	}	
	else{
		GetDlgItem(IDC_DDOWNTOTAL)->SetWindowText("?");
		GetDlgItem(IDC_DUPTOTAL)->SetWindowText("?");
		GetDlgItem(IDC_DRATIO)->SetWindowText("?");
		GetDlgItem(IDC_CDIDENT)->SetWindowText("?");
	}

	if (m_client->GetUserName()){
		buffer.Format("%.1f",(float)m_client->GetScore(false,m_client->IsDownloading(),true));
		GetDlgItem(IDC_DRATING)->SetWindowText(buffer);
	}
	else
		GetDlgItem(IDC_DRATING)->SetWindowText("?");;

	if (m_client->GetUploadState() != US_NONE){
		if (!m_client->GetFriendSlot()){
			buffer.Format("%u",m_client->GetScore(false,m_client->IsDownloading(),false));
			GetDlgItem(IDC_DSCORE)->SetWindowText(buffer);
		}
		else
			GetDlgItem(IDC_DSCORE)->SetWindowText(GetResString(IDS_FRIENDDETAIL));
	}
	else
		GetDlgItem(IDC_DSCORE)->SetWindowText("-");
	return true;
}

BEGIN_MESSAGE_MAP(CClientDetailDialog, CDialog)
END_MESSAGE_MAP()


// CClientDetailDialog message handlers
void CClientDetailDialog::Localize(){
	GetDlgItem(IDC_STATIC30)->SetWindowText(GetResString(IDS_CD_GENERAL));
	GetDlgItem(IDC_STATIC31)->SetWindowText(GetResString(IDS_CD_UNAME));
	GetDlgItem(IDC_STATIC32)->SetWindowText(GetResString(IDS_CD_UHASH));
	GetDlgItem(IDC_STATIC33)->SetWindowText(GetResString(IDS_CD_CSOFT));
	GetDlgItem(IDC_STATIC35)->SetWindowText(GetResString(IDS_CD_SIP));
	GetDlgItem(IDC_STATIC36)->SetWindowText(GetResString(IDS_CD_VERSION));
	GetDlgItem(IDC_STATIC38)->SetWindowText(GetResString(IDS_CD_SNAME));

	GetDlgItem(IDC_STATIC40)->SetWindowText(GetResString(IDS_CD_TRANS));
	GetDlgItem(IDC_STATIC41)->SetWindowText(GetResString(IDS_CD_CDOWN));
	GetDlgItem(IDC_STATIC42)->SetWindowText(GetResString(IDS_CD_DOWN));
	GetDlgItem(IDC_STATIC43)->SetWindowText(GetResString(IDS_CD_ADOWN));
	GetDlgItem(IDC_STATIC44)->SetWindowText(GetResString(IDS_CD_TDOWN));
	GetDlgItem(IDC_STATIC45)->SetWindowText(GetResString(IDS_CD_UP));
	GetDlgItem(IDC_STATIC46)->SetWindowText(GetResString(IDS_CD_AUP));
	GetDlgItem(IDC_STATIC47)->SetWindowText(GetResString(IDS_CD_TUP));
	GetDlgItem(IDC_STATIC48)->SetWindowText(GetResString(IDS_CD_UPLOADREQ));

	GetDlgItem(IDC_STATIC50)->SetWindowText(GetResString(IDS_CD_SCORES));
	GetDlgItem(IDC_STATIC51)->SetWindowText(GetResString(IDS_CD_MOD));
	GetDlgItem(IDC_STATIC52)->SetWindowText(GetResString(IDS_CD_RATING));
	GetDlgItem(IDC_STATIC53)->SetWindowText(GetResString(IDS_CD_USCORE));
	GetDlgItem(IDC_STATIC133x)->SetWindowText(GetResString(IDS_CD_IDENT));

	GetDlgItem(IDOK)->SetWindowText(GetResString(IDS_FD_CLOSE));

	SetWindowText(GetResString(IDS_CD_TITLE));

}