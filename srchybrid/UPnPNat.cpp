// emulEspaña. Added by MoNKi [MoNKi: -UPnPNAT Support-]

//File coded by MoNKi for the "emulEspaña Mod" based on a sample code by Bkausbk

#include "StdAfx.h"
#include "natupnp.h"
#include "UPnPNat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

CUPnPNat::CUPnPNat(void)
{
	m_uLocalIP = 0;
}

CUPnPNat::~CUPnPNat(void)
{
	UPNPNAT_MAPPING search;
	POSITION pos = m_Mappings.GetHeadPosition();
	while(pos){
		search = m_Mappings.GetNext(pos);
		RemoveNATPortMapping(search, false);
	}

	m_Mappings.RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////////
// Adds a NAT Port Mapping
// Params:
//		UPNPNAT_MAPPING *mapping  ->  Port Mapping Data
//			If mapping->externalPort is 0, then
//			mapping->externalPort gets the value of mapping->internalPort
//		bool tryRandom:
//			If If mapping->externalPort is in use, tries to find a free
//			random external port.
//
// Return:
//		UNAT_OK:
//			Successfull.
//		UNAT_EXTERNAL_PORT_IN_USE:
//			Error, you are trying to add a port mapping with an external port
//			in use.
//		UNAT_NOT_IN_LAN:
//			Error, you aren't in a LAN -> no router or firewall
//		UNAT_ERROR:
//			Error, use GetLastError() to get an error description.
/////////////////////////////////////////////////////////////////////////////////
CUPnPNat::UPNPNAT_RETURN CUPnPNat::AddNATPortMapping(UPNPNAT_MAPPING *mapping, bool tryRandom){
	IUPnPNAT *nat = NULL;
	IStaticPortMappingCollection *spmc;
	IStaticPortMapping *spm;
	CComBSTR Proto;
	CString	ProtoStr;
	UPNPNAT_RETURN Status = UNAT_ERROR;
	int retries = 0;

	::CoInitialize(NULL);

	if(!IsLANIP(GetLocalIP())){
		SetLastError(_T("You aren't behind a Hardware Firewall or Router"));
		return UNAT_NOT_IN_LAN;
	}
	
	if (mapping->protocol == UNAT_TCP){
		Proto = _T("TCP");
		ProtoStr = _T("TCP");
	}
	else {
		Proto = _T("UDP");
		ProtoStr = _T("UDP");
	}

	if(mapping->externalPort == 0)
		mapping->externalPort = mapping->internalPort;

	HRESULT hResult = ::CoCreateInstance(__uuidof(UPnPNAT), NULL,  CLSCTX_ALL,  __uuidof(IUPnPNAT), (void**)&nat);

	if (hResult == S_OK && nat) {
		hResult = nat->get_StaticPortMappingCollection(&spmc);
		if (hResult == S_OK && spmc) {
			int retries = 255;
			BOOL portUsed = FALSE;
			WORD rndPort = mapping->externalPort;
			do {
				hResult = spmc->get_Item(rndPort, Proto, &spm);
				portUsed = spm != NULL;
				if (hResult == S_OK && spm) {
					// External port in use

					spm->Release();
					if (tryRandom){
						retries--;
						if (retries == 0) {
							// Too many retries
							spmc->Release();
							nat->Release();
							SetLastError(_T("External NAT port in use: Too many retries"));
							return UNAT_EXTERNAL_PORT_IN_USE;
						}
						rndPort = 2049 + (((float)rand() / RAND_MAX) * (65535 - 2049));
					}
					else{
						spmc->Release();
						nat->Release();
						SetLastError(_T("External NAT port in use"));
						return UNAT_EXTERNAL_PORT_IN_USE;
					}
				}
			} while (portUsed);

			mapping->externalPort = rndPort;

			CString Desc;
			Desc.Format(_T("eMule (%s) [%s: %u]"), mapping->description, ProtoStr, mapping->externalPort);

			CComBSTR DescBSTR(Desc);
			hResult = spmc->Add(mapping->externalPort, Proto, mapping->internalPort, GetLocalIPBSTR(), TRUE, DescBSTR, &spm);
			if (hResult == S_OK && spm) {
				/* Add new NAT port mapping */
				spm->Release();
				Status = UNAT_OK;
				
				m_Mappings.AddTail(*mapping);
			}
			spmc->Release();
		}
		else{
			if (hResult == S_OK) //spmc == NULL
				SetLastError(_T("Error getting StaticPortMappingCollection"));
			else
				SetLastIUPnPNATError(hResult, _T("Unknown error (2)"));
		}
		nat->Release();
	}
	else {
		if (hResult == S_OK) //nat == NULL
			SetLastError(_T("Error getting IUPnPNAT"));
		else
			SetLastClassError(hResult, _T("Unknown error (1)"));
	}

	return Status;
}

/////////////////////////////////////////////////////////////////////////////////
// Removes a NAT Port Mapping
// Params:
//		UPNPNAT_MAPPING *mapping  ->  Port Mapping Data
//			Should be the same struct passed to AddNATPortMapping
//		bool removeFromList	-> Remove the port mapping from the internal list
//			Should by allways true (dafault value if not passed).
//			If you set it to false can cause an unexpected error.
//
//
// Return:
//		UNAT_OK:
//			Successfull.
//		UNAT_NOT_OWNED_PORTMAPPING:
//			Error, you are trying to remove a port mapping not owned by this class
//		UNAT_NOT_IN_LAN:
//			Error, you aren't in a LAN -> no router or firewall
//		UNAT_ERROR:
//			Error, use GetLastError() to get an error description.
/////////////////////////////////////////////////////////////////////////////////
CUPnPNat::UPNPNAT_RETURN CUPnPNat::RemoveNATPortMapping(UPNPNAT_MAPPING mapping, bool removeFromList){
	IUPnPNAT *nat = NULL;
	IStaticPortMappingCollection *spmc;
	CComBSTR Proto;
	UPNPNAT_RETURN Status = UNAT_ERROR;
	bool found = false;

	::CoInitialize(NULL);
   
	if(!IsLANIP(GetLocalIP())){
		SetLastError(_T("You aren't behind a Hardware Firewall or Router"));
		return UNAT_NOT_IN_LAN;
	}

	UPNPNAT_MAPPING search;
	POSITION pos=NULL;
	for(pos = m_Mappings.GetHeadPosition(); !found && pos!=NULL; m_Mappings.GetNext(pos)){
		search = m_Mappings.GetAt(pos);

		if (search.externalPort == mapping.externalPort 
			&& search.protocol == mapping.protocol)
		{
			found = true;
   			HRESULT hResult = ::CoCreateInstance(__uuidof(UPnPNAT), NULL,  CLSCTX_ALL,  __uuidof(IUPnPNAT), (void**)&nat);
		    
			if (mapping.protocol == UNAT_TCP)
				Proto = "TCP";
			else
				Proto = "UDP";

			if (hResult == S_OK && nat) {
				hResult = nat->get_StaticPortMappingCollection(&spmc);
				if (hResult == S_OK && spmc) {
					spmc->Remove(mapping.externalPort, Proto);
					spmc->Release();
					Status = UNAT_OK;

					if(removeFromList)
						m_Mappings.RemoveAt(pos);
				}
				else {
					if (hResult == S_OK) //spmc == NULL
						SetLastError(_T("Error getting StaticPortMappingCollection"));
					else
						SetLastIUPnPNATError(hResult, _T("Unknown error (5)"));
				}
				nat->Release();
			}
			else {
				if (hResult == S_OK) //nat == NULL
					SetLastError(_T("Error getting IUPnPNAT"));
				else
					SetLastClassError(hResult, _T("Unknown error (4)"));
			}
		}
	}

	if(!found){
		SetLastError(_T("Port mapping not owned by this class"));
		return UNAT_NOT_OWNED_PORTMAPPING;
	}
	else
		return Status;
}

/////////////////////////////////////////////////////////////////////////////////
// Initializes m_localIP variable, for future access to GetLocalIP()
/////////////////////////////////////////////////////////////////////////////////
void CUPnPNat::InitLocalIP()
{
	try{
		char szHost[256];
		if (gethostname(szHost, sizeof szHost) == 0){
			hostent* pHostEnt = gethostbyname(szHost);
			if (pHostEnt != NULL && pHostEnt->h_length == 4 && pHostEnt->h_addr_list[0] != NULL){
				CUPnPNat::UPNPNAT_MAPPING mapping;
				struct in_addr addr;

				memcpy(&addr, pHostEnt->h_addr_list[0], sizeof(struct in_addr));
				m_slocalIP = inet_ntoa(addr);
				m_uLocalIP = addr.S_un.S_addr;
			}
			else{
				m_slocalIP = "";
				m_uLocalIP = 0;
			}
		}
		else{
			m_slocalIP = "";
			m_uLocalIP = 0;
		}
	}
	catch(...){
		m_slocalIP = "";
		m_uLocalIP = 0;
	}	
}

/////////////////////////////////////////////////////////////////////////////////
// Returns the Local IP
/////////////////////////////////////////////////////////////////////////////////
WORD CUPnPNat::GetLocalIP()
{
	if(m_uLocalIP == 0)
		InitLocalIP();
	
	return m_uLocalIP;
}

/////////////////////////////////////////////////////////////////////////////////
// Returns a CString with the local IP in format xxx.xxx.xxx.xxx
/////////////////////////////////////////////////////////////////////////////////
CString CUPnPNat::GetLocalIPStr()
{
	if(m_slocalIP.IsEmpty())
		InitLocalIP();
	
	return m_slocalIP;
}

/////////////////////////////////////////////////////////////////////////////////
// Returns a CComBSTR with the local IP in format xxx.xxx.xxx.xxx
/////////////////////////////////////////////////////////////////////////////////
CComBSTR CUPnPNat::GetLocalIPBSTR()	
{
	return CComBSTR(GetLocalIPStr()); 
}

/////////////////////////////////////////////////////////////////////////////////
// Sets the value of m_lastError (last error description)
/////////////////////////////////////////////////////////////////////////////////
void CUPnPNat::SetLastError(CString error)	{
	m_slastError = error;
};

/////////////////////////////////////////////////////////////////////////////////
// Sets the m_lastError string value if CoCreateInstance() fails
/////////////////////////////////////////////////////////////////////////////////
void CUPnPNat::SetLastClassError(HRESULT result, CString defaultString){
	switch (result){
		case REGDB_E_CLASSNOTREG:
			m_slastError = "Class not registered in the registration database";
			break;
		case CLASS_E_NOAGGREGATION:
			m_slastError = "This class cannot be created as part of an aggregate";
			break;
		case E_NOINTERFACE:
			m_slastError = "The specified class does not implement the requested interface";
			break;
		default:
			m_slastError = defaultString;
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////////
// Sets the m_lastError string value if an IUPnPNAT object fails
/////////////////////////////////////////////////////////////////////////////////
void CUPnPNat::SetLastIUPnPNATError(HRESULT result, CString defaultString){
	switch(result){
		case E_ABORT:
			m_slastError = "The operation was aborted";
			break;
		case E_FAIL:
			m_slastError = "An unspecified error occurred";
			break;
		case E_INVALIDARG:
			m_slastError = "One of the parameters is invalid";
			break;
		case E_NOINTERFACE:
			m_slastError = "A specified interface is not supported";
			break;
		case E_NOTIMPL: 
			m_slastError = "A specified method is not implemented";
			break;
		case E_OUTOFMEMORY:
			m_slastError = "The method was unable to allocate required memory";
			break;
		case E_POINTER:
			m_slastError = "A pointer passed as a parameter is not valid";
			break;
		case E_UNEXPECTED:
			m_slastError = "The method failed for unknown reasons";
			break;
		default:
			m_slastError = defaultString;
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////////
// Returns the last error description in a CString
/////////////////////////////////////////////////////////////////////////////////
CString CUPnPNat::GetLastError()
{ 
	return m_slastError; 
}

/////////////////////////////////////////////////////////////////////////////////
// Returns true if nIP is a LAN ip, false otherwise
/////////////////////////////////////////////////////////////////////////////////
bool CUPnPNat::IsLANIP(WORD nIP){
// filter LAN IP's
// -------------------------------------------
// 0.*
// 10.0.0.0 - 10.255.255.255  class A
// 172.16.0.0 - 172.31.255.255  class B
// 192.168.0.0 - 192.168.255.255 class C

unsigned char nFirst = (unsigned char)nIP;
unsigned char nSecond = (unsigned char)(nIP >> 8);

if (nFirst==192 && nSecond==168) // check this 1st, because those LANs IPs are mostly spreaded
  return true;

if (nFirst==172 && nSecond>=16 && nSecond<=31)
  return true;

if (nFirst==0 || nFirst==10)
  return true;

return false; 
}