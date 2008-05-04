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

#include "StdAfx.h"
#include "firewallopener.h"
#include "emule.h"
#include "preferences.h"
#include "otherfunctions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define RETURN_ON_FAIL(x)	if (!SUCCEEDED(x)) return false;

CFirewallOpener::CFirewallOpener(void)
{
	m_bInited = false;
	m_pINetSM = NULL;

	//MORPH START - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
	m_bClearMappings = false;
	//MORPH END   - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
}

CFirewallOpener::~CFirewallOpener(void)
{
	UnInit();
}

bool CFirewallOpener::Init(bool bPreInit)
{
	if (!m_bInited){
		ASSERT ( m_liAddedRules.IsEmpty() );
		if (thePrefs.GetWindowsVersion() != _WINVER_XP_ || !SUCCEEDED(CoInitialize(NULL)))
			return false;
		HRESULT hr = CoInitializeSecurity (NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
		if (!SUCCEEDED(hr) || !SUCCEEDED(::CoCreateInstance (__uuidof(NetSharingManager), NULL, CLSCTX_ALL, __uuidof(INetSharingManager), (void**)&m_pINetSM)) ){
			CoUninitialize(); 
			return false;
		}
	}
	m_bInited = true;
	if (bPreInit){
		// will return here in order to not create an instance when not really needed
		// preinit is only used to call CoInitializeSecurity before its too late for that (aka something else called it)
		// will have to look deeper into this issue in order to find a nicer way if possible
		return true;
	}

	if (m_pINetSM == NULL){
		if (!SUCCEEDED(::CoCreateInstance (__uuidof(NetSharingManager), NULL, CLSCTX_ALL, __uuidof(INetSharingManager), (void**)&m_pINetSM)) ){
			UnInit(); 
			return false;
		}
	}
	return true;
}

void CFirewallOpener::UnInit(){
	if (!m_bInited)
		return;
	
	for (int i = 0; i != m_liAddedRules.GetCount(); i++){
		//MORPH START - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
		/*
		if (m_liAddedRules[i].m_bRemoveOnExit)
		*/
		if (m_liAddedRules[i].m_bRemoveOnExit || m_bClearMappings)
		//MORPH END   - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
			RemoveRule(m_liAddedRules[i]);
	}
	m_liAddedRules.RemoveAll();

	m_bInited = false;
	if (m_pINetSM != NULL){
		m_pINetSM->Release();
		m_pINetSM = NULL;
	}
	else
		ASSERT ( false );
	CoUninitialize();
}

bool CFirewallOpener::DoAction(const EFOCAction eAction, const CICSRuleInfo& riPortRule){
	if ( !Init() )
		return false;
	//TODO lets see if we can find a reliable method to find out the internet standard connection set by the user

	bool bSuccess = true;
	bool bPartialSucceeded = false;
	bool bFoundAtLeastOneConn = false;

	INetSharingEveryConnectionCollectionPtr NSECCP;
	IEnumVARIANTPtr varEnum;
    IUnknownPtr pUnk;
    RETURN_ON_FAIL(m_pINetSM->get_EnumEveryConnection(&NSECCP));
    RETURN_ON_FAIL(NSECCP->get__NewEnum(&pUnk));
	RETURN_ON_FAIL(pUnk->QueryInterface(__uuidof(IEnumVARIANT), (void**)&varEnum));
	
	_variant_t var;
	while (S_OK == varEnum->Next(1, &var, NULL)) {
		INetConnectionPtr NCP;		
		if (V_VT(&var) == VT_UNKNOWN
			&& SUCCEEDED(V_UNKNOWN(&var)->QueryInterface(__uuidof(INetConnection),(void**)&NCP))) 
		{
			INetConnectionPropsPtr pNCP;
			if ( !SUCCEEDED(m_pINetSM->get_NetConnectionProps (NCP, &pNCP)) )
				continue;	
			DWORD dwCharacteristics = 0;
			pNCP->get_Characteristics(&dwCharacteristics);
			if (dwCharacteristics & (NCCF_FIREWALLED)) {
				NETCON_MEDIATYPE MediaType = NCM_NONE;
				pNCP->get_MediaType (&MediaType);
				if ((MediaType != NCM_SHAREDACCESSHOST_LAN) && (MediaType != NCM_SHAREDACCESSHOST_RAS) ){
					INetSharingConfigurationPtr pNSC;
					if ( !SUCCEEDED(m_pINetSM->get_INetSharingConfigurationForINetConnection (NCP, &pNSC)) )
						continue;
					VARIANT_BOOL varbool = VARIANT_FALSE;
					pNSC->get_InternetFirewallEnabled(&varbool);
					if (varbool == VARIANT_FALSE)
						continue;
					bFoundAtLeastOneConn = true;
					switch(eAction){
							case FOC_ADDRULE:{
								bool bResult;
								// we do not want to overwrite an existing rule
								if (FindRule(FOC_FINDRULEBYPORT, riPortRule, pNSC, NULL)){
									bResult = true;
								}
								else
									bResult = AddRule(riPortRule, pNSC, pNCP);
								bSuccess = bSuccess && bResult;
								if (bResult && !bPartialSucceeded)
									m_liAddedRules.Add(riPortRule); // keep track of added rules
								bPartialSucceeded = bPartialSucceeded || bResult;
								break;
							}
							case FOC_FWCONNECTIONEXISTS:
								return true;
							case FOC_DELETERULEBYNAME:
							case FOC_DELETERULEEXCACT:
								bSuccess = bSuccess && FindRule(eAction, riPortRule, pNSC, NULL);
								break;
							case FOC_FINDRULEBYNAME:
								if (FindRule(FOC_FINDRULEBYNAME, riPortRule, pNSC, NULL))
									return true;
								else
									bSuccess = false;
								break;
							case FOC_FINDRULEBYPORT:
								if (FindRule(FOC_FINDRULEBYPORT, riPortRule, pNSC, NULL))
									return true;
								else
									bSuccess = false;
								break;
							default:
								ASSERT ( false );
					}
				}
			}
		}
		var.Clear();
	}
	return bSuccess && bFoundAtLeastOneConn;
}

bool CFirewallOpener::AddRule(const CICSRuleInfo& riPortRule, const INetSharingConfigurationPtr pNSC, const INetConnectionPropsPtr pNCP){
        INetSharingPortMappingPtr pNSPM;
        HRESULT hr = pNSC->AddPortMapping(riPortRule.m_strRuleName.AllocSysString(),
                             riPortRule.m_byProtocol,
                             riPortRule.m_nPortNumber,
                             riPortRule.m_nPortNumber,
                             0,
                             L"127.0.0.1",
                             ICSTT_IPADDRESS,
                             &pNSPM);
		CComBSTR bstrName;
		pNCP->get_Name(&bstrName);
		if ( SUCCEEDED(hr) && SUCCEEDED(pNSPM->Enable())){
			//MORPH START - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
			if(riPortRule.m_bRemoveOnExit || m_bClearMappings){
				CICSRuleInfo ruleToAdd(riPortRule);
				AddToICFdat(ruleToAdd);
			}
			//MORPH END   - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
			theApp.QueueDebugLogLine(false, _T("Succeeded to add Rule '%s' for Port '%u' on Connection '%s'"),riPortRule.m_strRuleName, riPortRule.m_nPortNumber, CString(bstrName));
			return true;
		}
		else{
			theApp.QueueDebugLogLine(false, _T("Failed to add Rule '%s' for Port '%u' on Connection '%s'"),riPortRule.m_strRuleName, riPortRule.m_nPortNumber, CString(bstrName));
			return false;
		}
}

bool CFirewallOpener::FindRule(const EFOCAction eAction, const CICSRuleInfo& riPortRule, const INetSharingConfigurationPtr pNSC, INetSharingPortMappingPropsPtr* outNSPMP){
	INetSharingPortMappingCollectionPtr pNSPMC;
    RETURN_ON_FAIL(pNSC->get_EnumPortMappings (ICSSC_DEFAULT, &pNSPMC));
       
	INetSharingPortMappingPtr pNSPM;
    IEnumVARIANTPtr varEnum;
    IUnknownPtr pUnk;
    RETURN_ON_FAIL(pNSPMC->get__NewEnum(&pUnk));
    RETURN_ON_FAIL(pUnk->QueryInterface(__uuidof(IEnumVARIANT), (void**)&varEnum));
	_variant_t var;
    while (S_OK == varEnum->Next(1, &var, NULL)) {
		INetSharingPortMappingPropsPtr pNSPMP;
		if (V_VT(&var) == VT_DISPATCH
			&& SUCCEEDED(V_DISPATCH(&var)->QueryInterface(__uuidof(INetSharingPortMapping),(void**)&pNSPM)) 
			&& SUCCEEDED(pNSPM->get_Properties (&pNSPMP)))
		{
			UCHAR ucProt = 0;
			long uExternal = 0;
			CComBSTR bstrName;
			pNSPMP->get_IPProtocol (&ucProt);
			pNSPMP->get_ExternalPort (&uExternal);
			pNSPMP->get_Name(&bstrName);
			//MORPH START - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
			CComBSTR bstrTargetIP;
			pNSPMP->get_TargetIPAddress(&bstrTargetIP);
			//MORPH END   - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
			switch(eAction){
				case FOC_FINDRULEBYPORT:
					//MORPH START - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
					/*
					if (riPortRule.m_nPortNumber == uExternal && riPortRule.m_byProtocol == ucProt){
					*/
					if (riPortRule.m_nPortNumber == uExternal && riPortRule.m_byProtocol == ucProt && bstrTargetIP == CComBSTR("127.0.0.1")){
					//MORPH END   - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
						if (outNSPMP != NULL)
							*outNSPMP = pNSPM;
						return true;
					}
					break;
				case FOC_FINDRULEBYNAME:
					//MORPH START - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
					/*
					if (riPortRule.m_strRuleName == CString(bstrName)){
					*/
					if (riPortRule.m_strRuleName == CString(bstrName) && bstrTargetIP == CComBSTR("127.0.0.1")){
					//MORPH END    - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
						if (outNSPMP != NULL)
							*outNSPMP = pNSPM;
						return true;
					}
					break;
				case FOC_DELETERULEEXCACT:
					//MORPH START - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
					/*
					if (riPortRule.m_strRuleName == CString(bstrName)
						&& riPortRule.m_nPortNumber == uExternal && riPortRule.m_byProtocol == ucProt)
					{
					*/
					if (riPortRule.m_strRuleName == CString(bstrName)
						&& riPortRule.m_nPortNumber == uExternal && riPortRule.m_byProtocol == ucProt
						&& bstrTargetIP == CComBSTR("127.0.0.1"))
					{
					//MORPH END   - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
						RETURN_ON_FAIL(pNSC->RemovePortMapping(pNSPM));
						theApp.QueueDebugLogLine(false,_T("Rule removed"));
					}
					break;
				case FOC_DELETERULEBYNAME:
					//MORPH START - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
					/*
					if (riPortRule.m_strRuleName == CString(bstrName)){
					*/
					if (riPortRule.m_strRuleName == CString(bstrName) && bstrTargetIP == CComBSTR("127.0.0.1")){
					//MORPH END   - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
						RETURN_ON_FAIL(pNSC->RemovePortMapping(pNSPM));
						theApp.QueueDebugLogLine(false,_T("Rule removed"));
					}
					break;
				default:
					ASSERT( false );
			}
		}
		var.Clear();
	}

	switch(eAction){
		case FOC_DELETERULEBYNAME:
		case FOC_DELETERULEEXCACT:
			return true;
		case FOC_FINDRULEBYPORT:
		case FOC_FINDRULEBYNAME:
		default:
			return false;
	}
}

bool CFirewallOpener::RemoveRule(const CString strName){
	return DoAction(FOC_DELETERULEBYNAME, CICSRuleInfo(0, 0, strName) );
}

bool CFirewallOpener::RemoveRule(const CICSRuleInfo& riPortRule){
	return DoAction(FOC_DELETERULEEXCACT, riPortRule);
}

bool CFirewallOpener::DoesRuleExist(const CString strName){
	return DoAction(FOC_FINDRULEBYNAME, CICSRuleInfo(0, 0, strName) );
}

bool CFirewallOpener::DoesRuleExist(const uint16 nPortNumber,const uint8 byProtocol){
	return DoAction(FOC_FINDRULEBYPORT, CICSRuleInfo(nPortNumber, byProtocol, _T("")) );
}

bool CFirewallOpener::OpenPort(const uint16 nPortNumber,const uint8 byProtocol,const CString strRuleName, const bool bRemoveOnExit){
	return DoAction(FOC_ADDRULE, CICSRuleInfo(nPortNumber, byProtocol, strRuleName, bRemoveOnExit));
}

bool CFirewallOpener::OpenPort(const CICSRuleInfo& riPortRule){
	return DoAction(FOC_ADDRULE, riPortRule);
}

bool CFirewallOpener::DoesFWConnectionExist(){
	return DoAction(FOC_FWCONNECTIONEXISTS, CICSRuleInfo());
}
//MORPH START - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
void CFirewallOpener::ClearOld(){
	bool deleteFile = true;
	bool ret = false;

	//Clears old mappings
	CFile fICFdat;
	if(fICFdat.Open(GetICFdatFileName(), CFile::modeRead | CFile::typeBinary)){
		CICSRuleInfo oldmapping;

		while(ReadFromICFdat(fICFdat, oldmapping)){
			ret = RemoveRule(oldmapping);
			deleteFile = deleteFile && ret;
		}
		
		fICFdat.Close();

		if(deleteFile)
			CFile::Remove(GetICFdatFileName());
	}
}

void CFirewallOpener::ClearMappingsAtEnd(){
	m_bClearMappings = true;
	CICSRuleInfo mapping, search;

	//Add all mappings to the ICF.dat file 
	CFile fICFdat;
	if(fICFdat.Open(GetICFdatFileName(), CFile::modeCreate|CFile::modeNoTruncate|CFile::modeReadWrite|CFile::typeBinary)){
		for(int i = 0; i < m_liAddedRules.GetCount(); i++){
			mapping = m_liAddedRules.GetAt(i);

			//Search for the item on ICF.dat
			fICFdat.SeekToBegin();
			bool found = false;
			while(!found && ReadFromICFdat(fICFdat, search)){
				if(search.m_nPortNumber == mapping.m_nPortNumber &&
					search.m_byProtocol == mapping.m_byProtocol &&
					search.m_strRuleName == mapping.m_strRuleName)
				{
					found = true;
				}
			}

			if(!found){
				fICFdat.SeekToEnd();
				AddToICFdat(fICFdat, mapping);
			}
		}
		fICFdat.Close();
	}
}

bool CFirewallOpener::AddToICFdat(CFile &file, CICSRuleInfo &mapping){
	bool ret = false;
	if(file.m_hFile  != CFile::hFileNull){
		int iDescLen;
		file.Write(&(mapping.m_nPortNumber), sizeof(uint16));
		file.Write(&(mapping.m_byProtocol), sizeof(uint8));
		iDescLen = mapping.m_strRuleName.GetLength();
		file.Write(&iDescLen, sizeof(int));
		file.Write(mapping.m_strRuleName.GetBuffer(), iDescLen);
		ret = true;
	}
	return ret;
}

bool CFirewallOpener::AddToICFdat(CICSRuleInfo &mapping){
	CFile fICFdat;
	bool ret = false;
	if(fICFdat.Open(GetICFdatFileName(), CFile::modeCreate|CFile::modeNoTruncate|CFile::modeWrite|CFile::typeBinary)){
		fICFdat.SeekToEnd();
		ret = AddToICFdat(fICFdat, mapping);
		fICFdat.Close();
	}
	return ret;
}

bool CFirewallOpener::ReadFromICFdat(CFile &file, CICSRuleInfo &mapping){
	UINT uiBRead;
	CICSRuleInfo newMapping;
	bool ret = false;

	newMapping.m_bRemoveOnExit = true;

	uiBRead = file.Read(&(newMapping.m_nPortNumber), sizeof(uint16));
	if(uiBRead == sizeof(uint16)){
		uiBRead = file.Read(&(newMapping.m_byProtocol), sizeof(uint8));
		if(uiBRead == sizeof(uint8)){
			UINT iDescLen = 0;
			uiBRead = file.Read(&iDescLen, sizeof(int));
			if(uiBRead == sizeof(int)){
				char *cDesc;
				cDesc = new char[iDescLen+1];
				cDesc[iDescLen]='\0';
				uiBRead = file.Read(cDesc, iDescLen);
				if(uiBRead == iDescLen){
					newMapping.m_strRuleName = CString(cDesc);
					ret = true;
				}
				delete[] cDesc;
			}
		}
	}

	if(ret){
		mapping = newMapping;
	}

	return ret;
}

CString CFirewallOpener::GetICFdatFileName(){
	return CString(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR)+_T("ICF.dat"));
}
//MORPH END   - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
