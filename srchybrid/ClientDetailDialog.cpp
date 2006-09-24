//this file is part of eMule
//Copyright (C)2002-2006 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "HighColorTab.hpp"
#include "UserMsgs.h"
#include "ListenSocket.h"
#include "preferences.h"
#include "IP2Country.h" //EastShare - added by AndCycle, IP to Country

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


///////////////////////////////////////////////////////////////////////////////
// CClientDetailPage

IMPLEMENT_DYNAMIC(CClientDetailPage, CResizablePage)

BEGIN_MESSAGE_MAP(CClientDetailPage, CResizablePage)
	ON_MESSAGE(UM_DATA_CHANGED, OnDataChanged)
END_MESSAGE_MAP()

CClientDetailPage::CClientDetailPage()
	: CResizablePage(CClientDetailPage::IDD,0)
{
	m_paClients = NULL;
	m_bDataChanged = false;
	m_strCaption	= GetResString(IDS_CD_TITLE);
	m_psp.pszTitle	= m_strCaption;
	m_psp.dwFlags  |= PSP_USETITLE;
}

CClientDetailPage::~CClientDetailPage()
{
}

void CClientDetailPage::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
}

BOOL CClientDetailPage::OnInitDialog()
{
	CResizablePage::OnInitDialog();
	InitWindowStyles(this);

	AddAnchor(IDC_STATIC30, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATIC40, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATIC50, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_DDOWNLOADING, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_UPLOADING, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_OBFUSCATION_STAT, TOP_LEFT, TOP_RIGHT);

	//MORPH - Added by SiRoB, WebCache
	AddAnchor(IDC_STATIC57, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_WCSTATISTICS, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_Webcache, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_TRUSTEDOHCBSENDER, TOP_LEFT, TOP_RIGHT);
	//MORPH - Added by SiRoB, WebCache

	Localize();
	return TRUE;
}

BOOL CClientDetailPage::OnSetActive()
{
	if (!CResizablePage::OnSetActive())
		return FALSE;

	if (m_bDataChanged)
	{
		CUpDownClient* client = STATIC_DOWNCAST(CUpDownClient, (*m_paClients)[0]);

		CString buffer;
		if (client->GetUserName())
			GetDlgItem(IDC_DNAME)->SetWindowText(client->GetUserName());
		else
			GetDlgItem(IDC_DNAME)->SetWindowText(_T("?"));
	
		//EastShare Start - added by AndCycle, IP to Country
		// Superlexx
		bool longCountryName = true;
		GetDlgItem(IDC_DLOC)->SetWindowText(client->GetCountryName(longCountryName));
		//MORPH START - Added by Commander, CountryFlag
		if (theApp.ip2country->ShowCountryFlag()){
			countryflag = theApp.ip2country->GetFlagImageList()->ExtractIcon(client->GetCountryFlagIndex());
			((CStatic*)GetDlgItem(IDC_COUNTRYFLAG))->SetIcon(countryflag);
			((CStatic*)GetDlgItem(IDC_COUNTRYFLAG))->ShowWindow(SW_SHOW);
		}
		//MORPH END - Added by Commander, CountryFlag
		//EastShare End - added by AndCycle, IP to Country

		if (client->HasValidHash())
			GetDlgItem(IDC_DHASH)->SetWindowText(md4str(client->GetUserHash()));
		else
			GetDlgItem(IDC_DHASH)->SetWindowText(_T("?"));
	
		GetDlgItem(IDC_DSOFT)->SetWindowText(client->GetClientSoftVer());

		if (client->SupportsCryptLayer() && thePrefs.IsClientCryptLayerSupported() && (client->RequestsCryptLayer() || thePrefs.IsClientCryptLayerRequested()) 
			&& (client->IsObfuscatedConnectionEstablished() || !(client->socket != NULL && client->socket->IsConnected())))
		{
			buffer = GetResString(IDS_ENABLED);
		}
		else if (client->SupportsCryptLayer())
			buffer = GetResString(IDS_SUPPORTED);
		else
			buffer = GetResString(IDS_IDENTNOSUPPORT);
#if defined(_DEBUG) || defined(_BETA)
		if (client->IsObfuscatedConnectionEstablished())
			buffer += _T("(In Use)");
#endif
		GetDlgItem(IDC_OBFUSCATION_STAT)->SetWindowText(buffer);

		buffer.Format(_T("%s"),(client->HasLowID() ? GetResString(IDS_IDLOW):GetResString(IDS_IDHIGH)));
		GetDlgItem(IDC_DID)->SetWindowText(buffer);
	
		if (client->GetServerIP()){
			GetDlgItem(IDC_DSIP)->SetWindowText(ipstr(client->GetServerIP()));
			CServer* cserver = theApp.serverlist->GetServerByIPTCP(client->GetServerIP(), client->GetServerPort());
			if (cserver)
				GetDlgItem(IDC_DSNAME)->SetWindowText(cserver->GetListName());
			else
				GetDlgItem(IDC_DSNAME)->SetWindowText(_T("?"));
		}
		else{
			GetDlgItem(IDC_DSIP)->SetWindowText(_T("?"));
			GetDlgItem(IDC_DSNAME)->SetWindowText(_T("?"));
		}

		CKnownFile* file = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
		if (file)
			GetDlgItem(IDC_DDOWNLOADING)->SetWindowText(file->GetFileName() );
		else
			GetDlgItem(IDC_DDOWNLOADING)->SetWindowText(_T("-"));

		if (client->GetRequestFile())
			GetDlgItem(IDC_UPLOADING)->SetWindowText( client->GetRequestFile()->GetFileName()  );
		else 
			GetDlgItem(IDC_UPLOADING)->SetWindowText(_T("-"));

		GetDlgItem(IDC_DDUP)->SetWindowText(CastItoXBytes(client->GetTransferredDown(), false, false));

		GetDlgItem(IDC_DDOWN)->SetWindowText(CastItoXBytes(client->GetTransferredUp(), false, false));

		//MORPH START - Changed by SiRoB, Average display [wistily]
		/*
		buffer.Format(_T("%s"), CastItoXBytes(client->GetDownloadDatarate(), false, true));
		*/
		buffer.Format(_T("%s"), CastItoXBytes(client->GetAvDownDatarate(), false, true));
		//MORPH END   - Changed by SiRoB, Average display [wistily]
		GetDlgItem(IDC_DAVUR)->SetWindowText(buffer);

		//MORPH START - Changed by SiRoB, Average display [wistily]
		/*
		buffer.Format(_T("%s"),CastItoXBytes(client->GetDatarate(), false, true));
		*/
		buffer.Format(_T("%s"),CastItoXBytes(client->GetAvUpDatarate(), false, true));
		//MORPH END   - Changed by SiRoB, Average display [wistily]
		GetDlgItem(IDC_DAVDR)->SetWindowText(buffer);
	
		if (client->Credits()){
			GetDlgItem(IDC_DUPTOTAL)->SetWindowText(CastItoXBytes(client->Credits()->GetDownloadedTotal(), false, false));
			GetDlgItem(IDC_DDOWNTOTAL)->SetWindowText(CastItoXBytes(client->Credits()->GetUploadedTotal(), false, false));
			//MORPH START - Changed by IceCream, VQB: ownCredits
			/*
			buffer.Format(_T("%.1f"),(float)client->Credits()->GetScoreRatio(m_client->GetIP()));
			*/
			buffer.Format(_T("%.1f  [%.1f]"),(float)client->Credits()->GetScoreRatio(client->GetIP()),(float)client->Credits()->GetMyScoreRatio(client->GetIP()));
			//MORPH END   - Changed by IceCream, VQB: ownCredits
			GetDlgItem(IDC_DRATIO)->SetWindowText(buffer);
		
			if (theApp.clientcredits->CryptoAvailable()){
				switch(client->Credits()->GetCurrentIdentState(client->GetIP())){
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

		if (client->GetUserName() && client->Credits()!=NULL){
			buffer.Format(_T("%.1f"),(float)client->GetScore(false,client->IsDownloading(),true));
			GetDlgItem(IDC_DRATING)->SetWindowText(buffer);
		}
		else
		GetDlgItem(IDC_DRATING)->SetWindowText(_T("?"));

		if (client->GetUploadState() != US_NONE && client->Credits()!=NULL){
			if (!client->GetFriendSlot()){
				buffer.Format(_T("%u"),client->GetScore(false,client->IsDownloading(),false));
				GetDlgItem(IDC_DSCORE)->SetWindowText(buffer);
			}
			else
				GetDlgItem(IDC_DSCORE)->SetWindowText(GetResString(IDS_FRIENDDETAIL));
		}
		else
			GetDlgItem(IDC_DSCORE)->SetWindowText(_T("-"));

		if (client->GetKadPort() )
			buffer.Format( _T("%s"), GetResString(IDS_CONNECTED));
		else
			buffer.Format( _T("%s"), GetResString(IDS_DISCONNECTED));
		GetDlgItem(IDC_CLIENTDETAIL_KADCON)->SetWindowText(buffer);
	
		// [MightyKnife] Private modification
		#ifdef MIGHTY_TWEAKS
		CString AddInfo;
		uint32 ClientIP = client->GetIP ();
		AddInfo.Format (_T("User-ID: %u   IP: %d.%d.%d.%d:%d"),
						client->GetUserIDHybrid (),
						ClientIP & 0xFF, (ClientIP >> 8) & 0xFF, 
						(ClientIP >> 16) & 0xFF, (ClientIP >> 24) & 0xFF, 
						client->GetUserPort ());
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
		if(client->SupportsWebCache() && client->GetWebCacheName() == "")
			GetDlgItem(IDC_Webcache)->SetWindowText(GetResString(IDS_WEBCACHE_NOPROXY));
		if(client->SupportsWebCache() && client->GetWebCacheName() != "")
			GetDlgItem(IDC_Webcache)->SetWindowText(client->GetWebCacheName()); // Superlexx - webcache //JP changed to new GetWebcacheName-function
    	if(!client->SupportsWebCache())
			GetDlgItem(IDC_Webcache)->SetWindowText(GetResString(IDS_WEBCACHE_NOSUPPORT));
    	//MORPH END - Modified by Commander, WebCacheName

		double percentSessions = 0;
		if (client->WebCachedBlockRequests != 0)
			percentSessions = (double) 100 * client->SuccessfulWebCachedBlockDownloads / client->WebCachedBlockRequests;
		buffer.Format( _T("%u/%u (%1.1f%%)"), client->SuccessfulWebCachedBlockDownloads, client->WebCachedBlockRequests, percentSessions );
		GetDlgItem(IDC_WCSTATISTICS)->SetWindowText(buffer); //JP Client WC-Statistics
		if (client->IsTrustedOHCBSender())
			buffer.Format(GetResString(IDS_YES));
		else
			buffer.Format(GetResString(IDS_NO));
		GetDlgItem(IDC_TRUSTEDOHCBSENDER)->SetWindowText(buffer); //JP Is trusted OHCB sender
		//MORPH END   - Added by SiRoB, Webcache 1.2f

		m_bDataChanged = false;
	}
	return TRUE;
}

LRESULT CClientDetailPage::OnDataChanged(WPARAM, LPARAM)
{
	m_bDataChanged = true;
	return 1;
}

void CClientDetailPage::Localize()
{
	GetDlgItem(IDC_STATIC30)->SetWindowText(GetResString(IDS_CD_GENERAL));
	GetDlgItem(IDC_STATIC31)->SetWindowText(GetResString(IDS_CD_UNAME));
	GetDlgItem(IDC_STATIC32)->SetWindowText(GetResString(IDS_CD_UHASH));
	GetDlgItem(IDC_STATIC33)->SetWindowText(GetResString(IDS_CD_CSOFT) + _T(':'));
	GetDlgItem(IDC_STATIC35)->SetWindowText(GetResString(IDS_CD_SIP));
	GetDlgItem(IDC_STATIC38)->SetWindowText(GetResString(IDS_CD_SNAME));
	GetDlgItem(IDC_STATIC_OBF_LABEL)->SetWindowText(GetResString(IDS_OBFUSCATION) + _T(':'));

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
	GetDlgItem(IDC_STATIC54)->SetWindowText(GetResString(IDS_CD_WCSUCCFAIL));
	GetDlgItem(IDC_STATIC55)->SetWindowText(GetResString(IDS_CD_WCTRUSTSENDER));
	GetDlgItem(IDC_STATIC56)->SetWindowText(GetResString(IDS_PW_WEBCACHE) + _T(":"));
	GetDlgItem(IDC_STATIC57)->SetWindowText(GetResString(IDS_PW_WEBCACHE));
	GetDlgItem(IDC_STATIC133x)->SetWindowText(GetResString(IDS_CD_IDENT));
	GetDlgItem(IDC_DLOC2)->SetWindowText(GetResString(IDS_COUNTRY) + _T(":"));
	GetDlgItem(IDC_CLIENTDETAIL_KAD)->SetWindowText(GetResString(IDS_KADEMLIA) + _T(":"));
}


///////////////////////////////////////////////////////////////////////////////
// CClientDetailDialog

IMPLEMENT_DYNAMIC(CClientDetailDialog, CListViewWalkerPropertySheet)

BEGIN_MESSAGE_MAP(CClientDetailDialog, CListViewWalkerPropertySheet)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CClientDetailDialog::CClientDetailDialog(CUpDownClient* pClient, CListCtrlItemWalk* pListCtrl)
	: CListViewWalkerPropertySheet(pListCtrl)
{
	m_aItems.Add(pClient);
	Construct();
}

CClientDetailDialog::CClientDetailDialog(const CSimpleArray<CUpDownClient*>* paClients, CListCtrlItemWalk* pListCtrl)
	: CListViewWalkerPropertySheet(pListCtrl)
{
	for (int i = 0; i < paClients->GetSize(); i++)
		m_aItems.Add((*paClients)[i]);
	Construct();
}

void CClientDetailDialog::Construct()
{
	m_psh.dwFlags &= ~PSH_HASHELP;
	m_psh.dwFlags |= PSH_NOAPPLYNOW;

	m_wndClient.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndClient.m_psp.dwFlags |= PSP_USEICONID;
	m_wndClient.m_psp.pszIcon = _T("CLIENTDETAILS");
	m_wndClient.SetClients(&m_aItems);
	AddPage(&m_wndClient);
}

CClientDetailDialog::~CClientDetailDialog()
{
}

void CClientDetailDialog::OnDestroy()
{
	CListViewWalkerPropertySheet::OnDestroy();
}

BOOL CClientDetailDialog::OnInitDialog()
{		
	EnableStackedTabs(FALSE);
	BOOL bResult = CListViewWalkerPropertySheet::OnInitDialog();
	HighColorTab::UpdateImageList(*this);
	InitWindowStyles(this);
	EnableSaveRestore(_T("ClientDetailDialog")); // call this after(!) OnInitDialog
	SetWindowText(GetResString(IDS_CD_TITLE));
	return bResult;
}
