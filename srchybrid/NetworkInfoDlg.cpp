#include "stdafx.h"
#include "emule.h"
#include "NetworkInfoDlg.h"
#include "RichEditCtrlX.h"
#include "OtherFunctions.h"
#include "Sockets.h"
#include "Preferences.h"
#include "ServerList.h"
#include "Server.h"
#include "kademlia/kademlia/kademlia.h"
#include "kademlia/kademlia/prefs.h"
#include "kademlia/kademlia/indexed.h"
#include "WebServer.h"
#include "clientlist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


#define	PREF_INI_SECTION	_T("NetworkInfoDlg")


IMPLEMENT_DYNAMIC(CNetworkInfoDlg, CDialog)

BEGIN_MESSAGE_MAP(CNetworkInfoDlg, CResizableDialog)
END_MESSAGE_MAP()

CNetworkInfoDlg::CNetworkInfoDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CNetworkInfoDlg::IDD, pParent)
{
}

CNetworkInfoDlg::~CNetworkInfoDlg()
{
}

void CNetworkInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NETWORK_INFO, m_info);
}

BOOL CNetworkInfoDlg::OnInitDialog()
{
	ReplaceRichEditCtrl(GetDlgItem(IDC_NETWORK_INFO), this, GetDlgItem(IDC_NETWORK_INFO_LABEL)->GetFont());
	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);

	AddAnchor(IDC_NETWORK_INFO, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	EnableSaveRestore(PREF_INI_SECTION);

	SetWindowText(GetResString(IDS_NETWORK_INFO));
	SetDlgItemText(IDC_NETWORK_INFO_LABEL, GetResString(IDS_NETWORK_INFO));

	m_info.SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
	m_info.SetAutoURLDetect();
	m_info.SetEventMask(m_info.GetEventMask() | ENM_LINK);

	CHARFORMAT cfDef = {0};
	CHARFORMAT cfBold = {0};

	PARAFORMAT pf = {0};
	pf.cbSize = sizeof pf;
	if (m_info.GetParaFormat(pf)){
		pf.dwMask |= PFM_TABSTOPS;
		pf.cTabCount = 4;
		pf.rgxTabs[0] = 900;
		pf.rgxTabs[1] = 1000;
		pf.rgxTabs[2] = 1100;
		pf.rgxTabs[3] = 1200;
		m_info.SetParaFormat(pf);
	}

	cfDef.cbSize = sizeof cfDef;
	if (m_info.GetSelectionCharFormat(cfDef)){
		cfBold = cfDef;
		cfBold.dwMask |= CFM_BOLD;
		cfBold.dwEffects |= CFE_BOLD;
	}

	CreateNetworkInfo(m_info, cfDef, cfBold, true);

	return TRUE;
}

void CreateNetworkInfo(CRichEditCtrlX& rCtrl, CHARFORMAT& rcfDef, CHARFORMAT& rcfBold, bool bFullInfo)
{
	CString buffer;

	if (bFullInfo)
	{
		///////////////////////////////////////////////////////////////////////////
		// Ports Info
		///////////////////////////////////////////////////////////////////////////
		rCtrl.SetSelectionCharFormat(rcfBold);
		rCtrl << GetResString(IDS_CLIENT) << _T("\r\n");
		rCtrl.SetSelectionCharFormat(rcfDef);

		rCtrl << GetResString(IDS_PW_NICK) << _T(":\t") << thePrefs.GetUserNick() << _T("\r\n");
		rCtrl << GetResString(IDS_CD_UHASH) << _T("\t") << md4str((uchar*)thePrefs.GetUserHash()) << _T("\r\n");
		rCtrl << _T("TCP-") << GetResString(IDS_PORT) << _T(":\t") << thePrefs.GetPort() << _T("\r\n");
		rCtrl << _T("UDP-") << GetResString(IDS_PORT) << _T(":\t") << thePrefs.GetUDPPort() << _T("\r\n");
		rCtrl << _T("\r\n");
	}

	///////////////////////////////////////////////////////////////////////////
	// ED2K
	///////////////////////////////////////////////////////////////////////////
	rCtrl.SetSelectionCharFormat(rcfBold);
	rCtrl << _T("eD2K ") << GetResString(IDS_NETWORK) << _T("\r\n");
	rCtrl.SetSelectionCharFormat(rcfDef);

	rCtrl << GetResString(IDS_STATUS) << _T(":\t");
	if (theApp.serverconnect->IsConnected())
		rCtrl << GetResString(IDS_CONNECTED);
	else if(theApp.serverconnect->IsConnecting())
		rCtrl << GetResString(IDS_CONNECTING);
	else 
		rCtrl << GetResString(IDS_DISCONNECTED);
	rCtrl << _T("\r\n");

	//I only show this in full display as the normal display is not
	//updated at regular intervals.
	if (bFullInfo && theApp.serverconnect->IsConnected())
	{
		uint32 uTotalUser = 0;
		uint32 uTotalFile = 0;

		theApp.serverlist->GetUserFileStatus(uTotalUser, uTotalFile);
		rCtrl << GetResString(IDS_UUSERS) << _T(":\t") << GetFormatedUInt(uTotalUser) << _T("\r\n");
		rCtrl << GetResString(IDS_PW_FILES) << _T(":\t") << GetFormatedUInt(uTotalFile) << _T("\r\n");
	}

	if (theApp.serverconnect->IsConnected()){
		rCtrl << GetResString(IDS_IP) << _T(":") << GetResString(IDS_PORT) << _T(":") ;
		if (theApp.serverconnect->IsLowID())
			buffer = GetResString(IDS_UNKNOWN);
		else
			buffer.Format(_T("%s:%u"), ipstr(theApp.serverconnect->GetClientID()), thePrefs.GetPort());
		rCtrl << _T("\t") << buffer << _T("\r\n");

		rCtrl << GetResString(IDS_ID) << _T(":\t");
		if (theApp.serverconnect->IsConnected()){
			buffer.Format(_T("%u"),theApp.serverconnect->GetClientID());
			rCtrl << buffer;
		}
		rCtrl << _T("\r\n");

		rCtrl << _T("\t");
		if (theApp.serverconnect->IsLowID())
			rCtrl << GetResString(IDS_IDLOW);
		else
			rCtrl << GetResString(IDS_IDHIGH);
		rCtrl << _T("\r\n");

		CServer* cur_server = theApp.serverconnect->GetCurrentServer();
		CServer* srv = cur_server ? theApp.serverlist->GetServerByAddress(cur_server->GetAddress(), cur_server->GetPort()) : NULL;
		if (srv){
			rCtrl << _T("\r\n");
			rCtrl.SetSelectionCharFormat(rcfBold);
			rCtrl << _T("eD2K ") << GetResString(IDS_SERVER) << _T("\r\n");
			rCtrl.SetSelectionCharFormat(rcfDef);

			rCtrl << GetResString(IDS_SW_NAME) << _T(":\t") << srv->GetListName() << _T("\r\n");
			rCtrl << GetResString(IDS_DESCRIPTION) << _T(":\t") << srv->GetDescription() << _T("\r\n");
			rCtrl << GetResString(IDS_IP) << _T(":") << GetResString(IDS_PORT) << _T(":\t") << srv->GetAddress() << _T(":") << srv->GetPort() << _T("\r\n");
			//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
//			if (srv->GetConnPort() != srv->GetPort())  
			rCtrl << GetResString(IDS_AUXPORTS) << _T(":\t") << srv->GetConnPort() << _T("\r\n"); 
			//Morph End - added by AndCycle, aux Ports, by lugdunummaster
			rCtrl << GetResString(IDS_VERSION) << _T(":\t") << srv->GetVersion() << _T("\r\n");
			rCtrl << GetResString(IDS_UUSERS) << _T(":\t") << GetFormatedUInt(srv->GetUsers()) << _T("\r\n");
			rCtrl << GetResString(IDS_PW_FILES) << _T(":\t") << GetFormatedUInt(srv->GetFiles()) << _T("\r\n");

			if (bFullInfo)
			{
				rCtrl << GetResString(IDS_IDLOW) << _T(":\t") << GetFormatedUInt(srv->GetLowIDUsers()) << _T("\r\n");
				rCtrl << GetResString(IDS_PING) << _T(":\t") << srv->GetPing() << _T(" ms\r\n");

				rCtrl << _T("\r\n");
				rCtrl.SetSelectionCharFormat(rcfBold);
				rCtrl << _T("eD2K ") << GetResString(IDS_SERVER) << _T(" ") << GetResString(IDS_FEATURES) << _T("\r\n");
				rCtrl.SetSelectionCharFormat(rcfDef);

				rCtrl << GetResString(IDS_SERVER_LIMITS) << _T(": ") << GetFormatedUInt(srv->GetSoftFiles()) << _T("/") << GetFormatedUInt(srv->GetHardFiles()) << _T("\r\n");

				if (thePrefs.IsExtControlsEnabled())
				{
					rCtrl << GetResString(IDS_SRV_TCPCOMPR) << _T(": ");
					if (srv->GetTCPFlags() & SRV_TCPFLG_COMPRESSION)
						rCtrl << GetResString(IDS_YES);
					else
						rCtrl << GetResString(IDS_NO);
					rCtrl << _T("\r\n");

					rCtrl << GetResString(IDS_SHORTTAGS) << _T(": ");
					if ((srv->GetTCPFlags() & SRV_TCPFLG_NEWTAGS) || (srv->GetUDPFlags() & SRV_UDPFLG_NEWTAGS))
						rCtrl << GetResString(IDS_YES);
					else
						rCtrl << GetResString(IDS_NO);
					rCtrl << _T("\r\n");

					rCtrl << _T("Unicode") << _T(": ");
					if ((srv->GetTCPFlags() & SRV_TCPFLG_UNICODE) || (srv->GetUDPFlags() & SRV_UDPFLG_UNICODE))
						rCtrl << GetResString(IDS_YES);
					else
						rCtrl << GetResString(IDS_NO);
					rCtrl << _T("\r\n");

					rCtrl << _T("Integer type tags") << _T(": ");
					if (srv->GetTCPFlags() & SRV_TCPFLG_TYPETAGINTEGER)
						rCtrl << GetResString(IDS_YES);
					else
						rCtrl << GetResString(IDS_NO);
					rCtrl << _T("\r\n");

					rCtrl << GetResString(IDS_SRV_UDPSR) << _T(": ");
					if (srv->GetUDPFlags() & SRV_UDPFLG_EXT_GETSOURCES)
						rCtrl << GetResString(IDS_YES);
					else
						rCtrl << GetResString(IDS_NO);
					rCtrl << _T("\r\n");

					rCtrl << GetResString(IDS_SRV_UDPSR) << _T(" #2: ");
					if (srv->GetUDPFlags() & SRV_UDPFLG_EXT_GETSOURCES2)
						rCtrl << GetResString(IDS_YES);
					else
						rCtrl << GetResString(IDS_NO);
					rCtrl << _T("\r\n");

					rCtrl << GetResString(IDS_SRV_UDPFR) << _T(": ");
					if (srv->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)
						rCtrl << GetResString(IDS_YES);
					else
						rCtrl << GetResString(IDS_NO);
					rCtrl << _T("\r\n");

					rCtrl << GetResString(IDS_SRV_LARGEFILES) << _T(": ");
					if (srv->SupportsLargeFilesTCP() || srv->SupportsLargeFilesUDP())
						rCtrl << GetResString(IDS_YES);
					else
						rCtrl << GetResString(IDS_NO);
					rCtrl << _T("\r\n");
				}
			}
		}
	}
	rCtrl << _T("\r\n");

	///////////////////////////////////////////////////////////////////////////
	// Kademlia
	///////////////////////////////////////////////////////////////////////////
	rCtrl.SetSelectionCharFormat(rcfBold);
	rCtrl << GetResString(IDS_KADEMLIA) << _T(" ") << GetResString(IDS_NETWORK) << _T("\r\n");
	rCtrl.SetSelectionCharFormat(rcfDef);
	
	rCtrl << GetResString(IDS_STATUS) << _T(":\t");
	if(Kademlia::CKademlia::IsConnected()){
		if(Kademlia::CKademlia::IsFirewalled())
			rCtrl << GetResString(IDS_FIREWALLED);
		else
			rCtrl << GetResString(IDS_KADOPEN);
		rCtrl << _T("\r\n");

		CString IP;
		IP = ipstr(ntohl(Kademlia::CKademlia::GetPrefs()->GetIPAddress()));
		buffer.Format(_T("%s:%i"), IP, thePrefs.GetUDPPort());
		rCtrl << GetResString(IDS_IP) << _T(":") << GetResString(IDS_PORT) << _T(":\t") << buffer << _T("\r\n");

		buffer.Format(_T("%u"),Kademlia::CKademlia::GetPrefs()->GetIPAddress());
		rCtrl << GetResString(IDS_ID) << _T(":\t") << buffer << _T("\r\n");

		rCtrl << GetResString(IDS_BUDDY) << _T(":\t");
		switch ( theApp.clientlist->GetBuddyStatus() )
		{
			case Disconnected:
				rCtrl << GetResString(IDS_BUDDYNONE);
				break;
			case Connecting:
				rCtrl << GetResString(IDS_CONNECTING);
				break;
			case Connected:
				rCtrl << GetResString(IDS_CONNECTED);
				break;
		}
		rCtrl << _T("\r\n");

		if (bFullInfo)
		{

			CString sKadID;
			Kademlia::CKademlia::GetPrefs()->GetKadID(&sKadID);
			rCtrl << GetResString(IDS_CD_UHASH) << _T("\t") << sKadID << _T("\r\n");

			rCtrl << GetResString(IDS_UUSERS) << _T(":\t") << GetFormatedUInt(Kademlia::CKademlia::GetKademliaUsers()) << _T("\r\n");
			rCtrl << GetResString(IDS_PW_FILES) << _T(":\t") << GetFormatedUInt(Kademlia::CKademlia::GetKademliaFiles()) << _T("\r\n");

			rCtrl <<  GetResString(IDS_INDEXED) << _T(":\r\n");
			buffer.Format(GetResString(IDS_KADINFO_SRC) , Kademlia::CKademlia::GetIndexed()->m_uTotalIndexSource);
			rCtrl << buffer;
			buffer.Format(GetResString(IDS_KADINFO_KEYW), Kademlia::CKademlia::GetIndexed()->m_uTotalIndexKeyword);
			rCtrl << buffer;
			buffer.Format(_T("\t%s: %u\r\n"), GetResString(IDS_NOTES), Kademlia::CKademlia::GetIndexed()->m_uTotalIndexNotes);
			rCtrl << buffer;
			buffer.Format(_T("\t%s: %u\r\n"), GetResString(IDS_THELOAD), Kademlia::CKademlia::GetIndexed()->m_uTotalIndexLoad);
			rCtrl << buffer;
		}
	}
	else if (Kademlia::CKademlia::IsRunning())
		rCtrl << GetResString(IDS_CONNECTING) << _T("\r\n");
	else
		rCtrl << GetResString(IDS_DISCONNECTED) << _T("\r\n");
	rCtrl << _T("\r\n");

	///////////////////////////////////////////////////////////////////////////
	// Web Interface
	///////////////////////////////////////////////////////////////////////////
	rCtrl.SetSelectionCharFormat(rcfBold);
	rCtrl << GetResString(IDS_WEBSRV) << _T("\r\n");
	rCtrl.SetSelectionCharFormat(rcfDef);
	rCtrl << GetResString(IDS_STATUS) << _T(":\t");
	rCtrl << (thePrefs.GetWSIsEnabled() ? GetResString(IDS_ENABLED) : GetResString(IDS_DISABLED)) << _T("\r\n");
	if (thePrefs.GetWSIsEnabled()){
		CString count;
		count.Format(_T("%i %s"),theApp.webserver->GetSessionCount(),GetResString(IDS_ACTSESSIONS));
		rCtrl << _T("\t") << count << _T("\r\n");
		CString strHostname;
		if (!thePrefs.GetYourHostname().IsEmpty() && thePrefs.GetYourHostname().Find(_T('.')) != -1)
			strHostname = thePrefs.GetYourHostname();
		else
			strHostname = ipstr(theApp.serverconnect->GetLocalIP());
		rCtrl << _T("URL:\t") << _T("http://") << strHostname << _T(":") << thePrefs.GetWSPort() << _T("/\r\n");
	}
	
    //MORPH START - Added by SiRoB / Commander, Wapserver [emulEspa�a]
	///////////////////////////////////////////////////////////////////////////
	// Wap Interface
	///////////////////////////////////////////////////////////////////////////
	rCtrl << _T("\r\n");
	rCtrl.SetSelectionCharFormat(rcfBold);
	rCtrl << GetResString(IDS_WAPSRV) << _T("\r\n");
	rCtrl.SetSelectionCharFormat(rcfDef);
	rCtrl << GetResString(IDS_STATUS) << _T(":\t");
	rCtrl << (theApp.wapserver->IsRunning() ? GetResString(IDS_ENABLED) : GetResString(IDS_DISABLED)) << _T("\r\n");
	if (thePrefs.GetWapServerEnabled()){
		CString count;
		count.Format(_T("%i %s"),theApp.wapserver->GetSessionCount(),GetResString(IDS_ACTSESSIONS));
		rCtrl << _T("\t") << count << _T("\r\n");

		CString strHostname;
		if (!thePrefs.GetYourHostname().IsEmpty() && thePrefs.GetYourHostname().Find(_T('.')) != -1)
			strHostname = thePrefs.GetYourHostname();
		else{
			if(Kademlia::CKademlia::IsConnected()){
				strHostname = ipstr(ntohl(Kademlia::CKademlia::GetPrefs()->GetIPAddress()));
			}
			else if (theApp.serverconnect->IsConnected()){
				if (theApp.serverconnect->IsLowID())
					strHostname = GetResString(IDS_UNKNOWN);
		else
					strHostname = ipstr(theApp.serverconnect->GetClientID());
			}
			else{
				strHostname = GetResString(IDS_UNKNOWN);
			}
		}
		rCtrl << _T("URL:\t") << _T("http://") << strHostname << _T(":") << thePrefs.GetWapPort() << _T("/\r\n");
	}
	//MORPH END - Added by SiRoB / Commander, Wapserver [emulEspa�a]
}