// emulEspaña. Added by MoNKi [MoNKi: -UPnPNAT Support-]

//File coded by MoNKi for the "emulEspaña Mod" based on a sample code by Bkausbk

#pragma once

#ifndef __CUPNPNAT_H__
#define __CUPNPNAT_H__

class CUPnPNat
{
public:
	CUPnPNat(void);
	~CUPnPNat(void);

	typedef enum{
		UNAT_OK,						// Successfull
		UNAT_ERROR,						// Error, use GetLastError() to get an error description
		UNAT_NOT_OWNED_PORTMAPPING,		// Error, you are trying to remove a port mapping not owned by this class
		UNAT_EXTERNAL_PORT_IN_USE,		// Error, you are trying to add a port mapping with an external port in use
		UNAT_NOT_IN_LAN					// Error, you aren't in a LAN -> no router or firewall
	} UPNPNAT_RETURN;

	typedef enum{
		UNAT_TCP,						// TCP Protocol
		UNAT_UDP						// UDP Protocol
	} UPNPNAT_PROTOCOL;

	typedef struct{
		WORD internalPort;				// Port mapping internal port
		WORD externalPort;				// Port mapping external port
		UPNPNAT_PROTOCOL protocol;		// Protocol-> TCP (UPNPNAT_PROTOCOL:UNAT_TCP) || UDP (UPNPNAT_PROTOCOL:UNAT_UDP)
		CString description;			// Port mapping description
	} UPNPNAT_MAPPING;

	UPNPNAT_RETURN AddNATPortMapping(UPNPNAT_MAPPING *mapping, bool tryRandom = false);
	UPNPNAT_RETURN RemoveNATPortMapping(UPNPNAT_MAPPING mapping, bool removeFromList = true);

	CString		GetLastError();
	CComBSTR	GetLocalIPBSTR();
	CString		GetLocalIPStr();
	WORD		GetLocalIP();
	bool		IsLANIP(WORD nIP);

private:
	CList<UPNPNAT_MAPPING, UPNPNAT_MAPPING> m_Mappings;
	CString		m_slastError;
	CString		m_slocalIP;
	WORD		m_uLocalIP;
	
	void		InitLocalIP();
	void		SetLastError(CString error);
	void		SetLastClassError(HRESULT result, CString defaultString);
	void		SetLastIUPnPNATError(HRESULT result, CString defaultString);
};

#endif // __CUPNPNAT_H__
