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
#include "ClientDetailDialog.h"
#include "UpDownClient.h"
#include "PartFile.h"
#include "ClientCredits.h"
#include "otherfunctions.h"
#include "Server.h"
#include "ServerList.h"
#include "SharedFileList.h"
#include "IP2Country.h" //EastShare - added by AndCycle, IP to Country

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CClientDetailDialog dialog

IMPLEMENT_DYNAMIC(CClientDetailDialog, CDialog)
CClientDetailDialog::CClientDetailDialog(const CUpDownClient* client)
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
		GetDlgItem(IDC_DNAME)->SetWindowText(_T("?"));
	
	//EastShare Start - added by AndCycle, IP to Country
	// Superlexx
	bool longCountryName = true;
	GetDlgItem(IDC_DLOC)->SetWindowText(m_client->GetCountryName(longCountryName));
	//MORPH START - Added by Commander, CountryFlag
	if (theApp.ip2country->ShowCountryFlag()){
		countryflag = theApp.ip2country->GetFlagImageList()->ExtractIcon(m_client->GetCountryFlagIndex());
		((CStatic*)GetDlgItem(IDC_COUNTRYFLAG))->SetIcon(countryflag);
		((CStatic*)GetDlgItem(IDC_COUNTRYFLAG))->ShowWindow(SW_SHOW);
	}
		
	//MORPH END - Added by Commander, CountryFlag
	//EastShare End - added by AndCycle, IP to Country

	if (m_client->HasValidHash()){
		buffer = _T("");
		CString buffer2;
		for (uint16 i = 0;i != 16;i++){
			buffer2.Format(_T("%02X"),m_client->GetUserHash()[i]);
			buffer+=buffer2;
		}
		GetDlgItem(IDC_DHASH)->SetWindowText(buffer);
	}
	else
		GetDlgItem(IDC_DHASH)->SetWindowText(_T("?"));
	
	GetDlgItem(IDC_DSOFT)->SetWindowText(m_client->GetClientSoftVer());

	buffer.Format(_T("%s"),(m_client->HasLowID() ? GetResString(IDS_IDLOW):GetResString(IDS_IDHIGH)));
	GetDlgItem(IDC_DID)->SetWindowText(buffer);
	
	if (m_client->GetServerIP()){
		CString strServerIP = ipstr(m_client->GetServerIP());
		GetDlgItem(IDC_DSIP)->SetWindowText(strServerIP);
		
		CServer* cserver = theApp.serverlist->GetServerByAddress(strServerIP, m_client->GetServerPort()); 
		if (cserver)
			GetDlgItem(IDC_DSNAME)->SetWindowText(cserver->GetListName());
		else
			GetDlgItem(IDC_DSNAME)->SetWindowText(_T("?"));
	}
	else{
		GetDlgItem(IDC_DSIP)->SetWindowText(_T("?"));
		GetDlgItem(IDC_DSNAME)->SetWindowText(_T("?"));
	}

	CKnownFile* file = theApp.sharedfiles->GetFileByID(m_client->GetUploadFileID());
	if (file)
		GetDlgItem(IDC_DDOWNLOADING)->SetWindowText(file->GetFileName() );
	else
		GetDlgItem(IDC_DDOWNLOADING)->SetWindowText(_T("-"));

	if (m_client->GetRequestFile())
		GetDlgItem(IDC_UPLOADING)->SetWindowText( m_client->GetRequestFile()->GetFileName()  );
	else 
		GetDlgItem(IDC_UPLOADING)->SetWindowText(_T("-"));

	GetDlgItem(IDC_DDUP)->SetWindowText(CastItoXBytes(m_client->GetTransferedDown(), false, false));

	GetDlgItem(IDC_DDOWN)->SetWindowText(CastItoXBytes(m_client->GetTransferedUp(), false, false));

	//MORPH START - Changed by SiRoB, Average display [wistily]
	/*
	buffer.Format(_T("%s"), CastItoXBytes(m_client->GetDownloadDatarate(), false, true));
	*/
	buffer.Format(_T("%s"), CastItoXBytes(m_client->GetAvDownDatarate(), false, true));
	//MORPH END   - Changed by SiRoB, Average display [wistily]
	GetDlgItem(IDC_DAVUR)->SetWindowText(buffer);

	//MORPH START - Changed by SiRoB, Average display [wistily]
	/*
	buffer.Format(_T("%s"),CastItoXBytes(m_client->GetDatarate(), false, true));
	*/
	buffer.Format(_T("%s"),CastItoXBytes(m_client->GetAvUpDatarate(), false, true));
	//MORPH END   - Changed by SiRoB, Average display [wistily]
	GetDlgItem(IDC_DAVDR)->SetWindowText(buffer);
	
	if (m_client->Credits()){
		GetDlgItem(IDC_DUPTOTAL)->SetWindowText(CastItoXBytes(m_client->Credits()->GetDownloadedTotal(), false, false));
		GetDlgItem(IDC_DDOWNTOTAL)->SetWindowText(CastItoXBytes(m_client->Credits()->GetUploadedTotal(), false, false));
		//MORPH START - Changed by IceCream, VQB: ownCredits
		/*
		buffer.Format(_T("%.1f"),(float)m_client->Credits()->GetScoreRatio(m_client->GetIP()));
		*/
		buffer.Format(_T("%.1f  [%.1f]"),(float)m_client->Credits()->GetScoreRatio(m_client->GetIP()),(float)m_client->Credits()->GetMyScoreRatio(m_client->GetIP()));
		//MORPH END   - Changed by IceCream, VQB: ownCredits
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
		GetDlgItem(IDC_DDOWNTOTAL)->SetWindowText(_T("?"));
		GetDlgItem(IDC_DUPTOTAL)->SetWindowText(_T("?"));
		GetDlgItem(IDC_DRATIO)->SetWindowText(_T("?"));
		GetDlgItem(IDC_CDIDENT)->SetWindowText(_T("?"));
	}

	if (m_client->GetUserName()){
		buffer.Format(_T("%.1f"),(float)m_client->GetScore(false,m_client->IsDownloading(),true));
		GetDlgItem(IDC_DRATING)->SetWindowText(buffer);
	}
	else
		GetDlgItem(IDC_DRATING)->SetWindowText(_T("?"));

	if (m_client->GetUploadState() != US_NONE){
		if (!m_client->GetFriendSlot()){
			buffer.Format(_T("%u"),m_client->GetScore(false,m_client->IsDownloading(),false));
			GetDlgItem(IDC_DSCORE)->SetWindowText(buffer);
		}
		else
			GetDlgItem(IDC_DSCORE)->SetWindowText(GetResString(IDS_FRIENDDETAIL));
	}
	else
		GetDlgItem(IDC_DSCORE)->SetWindowText(_T("-"));

	if (m_client->GetKadPort() )
		buffer.Format( _T("%s"), GetResString(IDS_CONNECTED));
	else
		buffer.Format( _T("%s"), GetResString(IDS_DISCONNECTED));
	GetDlgItem(IDC_CLIENTDETAIL_KADCON)->SetWindowText(buffer);
	
	// [MightyKnife] Private modification
	#ifdef MIGHTY_TWEAKS
	CString AddInfo;
	uint32 ClientIP = m_client->GetIP ();
	AddInfo.Format (_T("User-ID: %u   IP: %d.%d.%d.%d:%d"),
					m_client->GetUserIDHybrid (),
					(ClientIP >> 24) & 0xFF, (ClientIP >> 16) & 0xFF,
					(ClientIP >> 8) & 0xFF, ClientIP & 0xFF, 
					m_client->GetUserPort ());
	CRect R (19,450,300,464);
	m_sAdditionalInfo.Create (AddInfo,WS_CHILD|WS_VISIBLE,R,this);
	VERIFY(m_fStdFont.CreateFont(
					12,                        // nHeight
					0,                         // nWidth
					0,                         // nEscapement
					0,                         // nOrientation
					FW_NORMAL,                 // nWeight
					FALSE,                     // bItalic
					FALSE,                     // bUnderline
					0,                         // cStrikeOut
					ANSI_CHARSET,              // nCharSet
					OUT_DEFAULT_PRECIS,        // nOutPrecision
					CLIP_DEFAULT_PRECIS,       // nClipPrecision
					DEFAULT_QUALITY,           // nQuality
					DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
					_T("MS Shell Dlg")));      // lpszFacename
	m_sAdditionalInfo.SetFont (&m_fStdFont);
	#endif
	// [MightyKnife] end: Private Modifications
	//MORPH START - Added by SiRoB, Webcache 1.2f

	//MORPH START - Modified by Commander, WebCacheName
	if(m_client->SupportsWebCache() && m_client->GetWebCacheName() == "")
		GetDlgItem(IDC_Webcache)->SetWindowText(_T("no proxy set"));
	if(m_client->SupportsWebCache() && m_client->GetWebCacheName() != "")
	GetDlgItem(IDC_Webcache)->SetWindowText(m_client->GetWebCacheName()); // Superlexx - webcache //JP changed to new GetWebcacheName-function
    if(!m_client->SupportsWebCache())
		GetDlgItem(IDC_Webcache)->SetWindowText(GetResString(IDS_WEBCACHE_NOSUPPORT));
    //MORPH END - Modified by Commander, WebCacheName

	double percentSessions = 0;
	if (m_client->WebCachedBlockRequests != 0)
		percentSessions = (double) 100 * m_client->SuccessfulWebCachedBlockDownloads / m_client->WebCachedBlockRequests;
	buffer.Format( _T("%u/%u (%1.1f%%)"), m_client->SuccessfulWebCachedBlockDownloads, m_client->WebCachedBlockRequests, percentSessions );
	GetDlgItem(IDC_WCSTATISTICS)->SetWindowText(buffer); //JP Client WC-Statistics
	if (m_client->IsTrustedOHCBSender())
		buffer.Format(_T("Yes"));
	else
		buffer.Format(_T("No"));
	GetDlgItem(IDC_TRUSTEDOHCBSENDER)->SetWindowText(buffer); //JP Is trusted OHCB sender
	//MORPH END   - Added by SiRoB, Webcache 1.2f
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
	CString buffer;
	buffer.Format(_T("%s:"), GetResString(IDS_KADEMLIA)); 
	GetDlgItem(IDC_CLIENTDETAIL_KAD)->SetWindowText(buffer);

	GetDlgItem(IDOK)->SetWindowText(GetResString(IDS_FD_CLOSE));

	SetWindowText(GetResString(IDS_CD_TITLE));

}