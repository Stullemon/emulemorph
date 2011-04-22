//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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

// class to configure the ICS-Firewall of Windows XP - will not work with WinXP-SP2 yet

#pragma once
#include <comdef.h>
#include <initguid.h>
#include <Netcon.h>

typedef _com_ptr_t<_com_IIID<INetSharingEveryConnectionCollection,&IID_INetSharingEveryConnectionCollection>	>	INetSharingEveryConnectionCollectionPtr;
typedef _com_ptr_t<_com_IIID<INetConnection,&IID_INetConnection>	>												INetConnectionPtr;
typedef _com_ptr_t<_com_IIID<INetConnectionProps,&IID_INetConnectionProps>	>										INetConnectionPropsPtr;
typedef _com_ptr_t<_com_IIID<INetSharingPortMapping,&IID_INetSharingPortMapping>	>								INetSharingPortMappingPtr;
typedef _com_ptr_t<_com_IIID<INetSharingConfiguration,&IID_INetSharingConfiguration>	>							INetSharingConfigurationPtr;
typedef _com_ptr_t<_com_IIID<INetSharingPortMappingProps,&IID_INetSharingPortMappingProps>	>						INetSharingPortMappingPropsPtr;
typedef _com_ptr_t<_com_IIID<INetSharingPortMappingCollection,&IID_INetSharingPortMappingCollection>	>			INetSharingPortMappingCollectionPtr;

enum EFOCAction{
	FOC_ADDRULE,
	FOC_DELETERULEBYNAME,
	FOC_FINDRULEBYNAME,
	FOC_FINDRULEBYPORT,
	FOC_DELETERULEEXCACT,
	FOC_FWCONNECTIONEXISTS
};

//MORPH START - Changed by SiRoB [MoNKi: -Improved ICS-Firewall support-]
/*
#define EMULE_DEFAULTRULENAME_UDP	_T("eMule_UDP_Port")
#define EMULE_DEFAULTRULENAME_TCP	_T("eMule_TCP_Port")
*/
#define EMULE_DEFAULTRULENAME_UDP			_T("UDP Port")
#define EMULE_DEFAULTRULENAME_TCP			_T("TCP Port")
#define EMULE_DEFAULTRULENAME_SERVERUDP		_T("Server UDP Port")
#define EMULE_DEFAULTRULEDESCRIPTIONFORMAT 	_T("eMule (%s) [%s: %u]")
// Added by MoNKi [MoNKi: -UPnPNAT Support-]
#define EMULE_DEFAULTRULENAME_UPNP_TCP		_T("UPnP_TCP_Port")
#define EMULE_DEFAULTRULENAME_UPNP_UDP		_T("UPnP_UDP_Port")
// End -UPnPNAT Support-
//MORPH END   - Changed by SiRoB [MoNKi: -Improved ICS-Firewall support-]

#define NAT_PROTOCOL_TCP 6
#define NAT_PROTOCOL_UDP 17

///////////////////////////////////////////////////////////////////////////////////////
/// CICSRuleInfo
class CICSRuleInfo{
public:
	CICSRuleInfo()								{}
	CICSRuleInfo(const CICSRuleInfo& ri)		{*this = ri;}
	CICSRuleInfo(uint16 nPortNumber, uint8 byProtocol, CString strRuleName, bool bRemoveOnExit = false)
	{
		m_nPortNumber = nPortNumber;
		m_byProtocol = byProtocol;
		//MORPH START - Changed by SiRoB [MoNKi: -Improved ICS-Firewall support-]
		/*
		m_strRuleName = strRuleName;
		*/
		m_strRuleName.Format(EMULE_DEFAULTRULEDESCRIPTIONFORMAT,
			strRuleName,
			(byProtocol == NAT_PROTOCOL_TCP) ? _T("TCP") : _T("UDP"),
			nPortNumber);
		//MORPH END   - Changed by SiRoB [MoNKi: -Improved ICS-Firewall support-]
		m_bRemoveOnExit = bRemoveOnExit;
	}

	CICSRuleInfo& operator=(const CICSRuleInfo& ri)
	{
		m_nPortNumber = ri.m_nPortNumber;
		m_byProtocol = ri.m_byProtocol;
		m_strRuleName = ri.m_strRuleName;
		m_bRemoveOnExit = ri.m_bRemoveOnExit;
		return *this;
	}

	uint16	m_nPortNumber;
	uint8	m_byProtocol;
	bool	m_bRemoveOnExit;
	CString	m_strRuleName;
};

///////////////////////////////////////////////////////////////////////////////////////
/// CFirewallOpener

class CFirewallOpener
{
public:
	CFirewallOpener(void);
	~CFirewallOpener(void);
	bool			OpenPort(const CICSRuleInfo& riPortRule);
	bool			OpenPort(const uint16 nPortNumber,const uint8 byProtocol,const CString strRuleName, const bool bRemoveOnExit = false);
	bool			RemoveRule(const CString strName);
	bool			RemoveRule(const CICSRuleInfo& riPortRule);
	bool			DoesRuleExist(const CString strName);
	bool			DoesRuleExist(const uint16 nPortNumber,const uint8 byProtocol);
	bool			DoesFWConnectionExist();
	void			UnInit();
	bool			Init(bool bPreInit = false);

protected:

	bool			AddRule(const CICSRuleInfo& riPortRule, const INetSharingConfigurationPtr pNSC, const INetConnectionPropsPtr pNCP);
	bool			DoAction(const EFOCAction eAction, const CICSRuleInfo& riPortRule);			
	bool			FindRule(const EFOCAction eAction, const CICSRuleInfo& riPortRule, const INetSharingConfigurationPtr pNSC, INetSharingPortMappingPropsPtr* outNSPMP);

	CArray<CICSRuleInfo>	m_liAddedRules;

private:
	INetSharingManager*		m_pINetSM;
	bool					m_bInited;

//MORPH START - Changed by SiRoB [MoNKi: -Improved ICS-Firewall support-]
public:
	void		ClearOld();
	void		ClearMappingsAtEnd();
private:
	bool m_bClearMappings;
	
	bool		AddToICFdat(CFile &file, CICSRuleInfo &mapping);
	bool		AddToICFdat(CICSRuleInfo &mapping);
	bool		ReadFromICFdat(CFile &file, CICSRuleInfo &mapping);
	CString		GetICFdatFileName();
//MORPH END   - Changed by SiRoB [MoNKi: -Improved ICS-Firewall support-]
};
