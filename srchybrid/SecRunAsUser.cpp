//this file is part of eMule
//Copyright (C)2004 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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


#include "StdAfx.h"
#include "emule.h"
#include "secrunasuser.h"
#include "Preferences.h"
#include "emuledlg.h"
#include "otherfunctions.h"

CSecRunAsUser::CSecRunAsUser(void)
{
	bRunningAsEmule = false;
	m_hADVAPI32_DLL = 0;
	m_hACTIVEDS_DLL = 0;
}

CSecRunAsUser::~CSecRunAsUser(void)
{
	FreeAPI();
}

bool CSecRunAsUser::PrepareUser(){

	USES_CONVERSION;
	CoInitialize(NULL);
	bool bResult = false;
	if (!LoadAPI())
		return false;
	
	IADsContainerPtr pUsers;
	try{
		IADsWinNTSystemInfoPtr pNTsys;
		if (CoCreateInstance(CLSID_WinNTSystemInfo,NULL,CLSCTX_INPROC_SERVER,IID_IADsWinNTSystemInfo,(void**)&pNTsys) != S_OK)
			throw CString("Failed to create IADsWinNTSystemInfo");
		// check if we are already running on our eMule Account
		// todo: check if the current account is an administrator
		
		CComBSTR bstrUserName;
		pNTsys->get_UserName(&bstrUserName);
		m_strCurrentUser = bstrUserName;

		if (m_strCurrentUser == EMULEACCOUNTW){
			theApp.QueueLogLine(false, GetResString(IDS_RAU_RUNNING), EMULEACCOUNT); 
			bRunningAsEmule = true;
			throw CString("Already running as eMule_Secure Account (everything is fine)");
		}
		CComBSTR bstrCompName;
		pNTsys->get_ComputerName(&bstrCompName);
		CStringW cscompName = bstrCompName;

		CComBSTR bstrDomainName;
		pNTsys->get_DomainName(&bstrDomainName);
		m_strDomain = bstrDomainName;
	
		ADSPath.Format(L"WinNT://%s,computer",cscompName);
		if ( !SUCCEEDED(ADsGetObject(ADSPath.AllocSysString(),IID_IADsContainer,(void **)&pUsers)) )
			throw CString("Failed ADsGetObject()");

		IEnumVARIANTPtr pEnum; 
		ADsBuildEnumerator (pUsers,&pEnum);
		int cnt=0;

		IADsUserPtr pChild;
		_variant_t vChild;			  
		while( ADsEnumerateNext (pEnum,1,&vChild,NULL) == S_OK )
		{	
			if (vChild.pdispVal->QueryInterface(IID_IADsUser,(void **)&pChild) != S_OK)
				continue;
			//If the object in the container is user then get properties
			CComBSTR bstrName; 
			pChild->get_Name(&bstrName);
			CStringW csName= bstrName;
			
			// find the emule user account if possible
			if ( csName == EMULEACCOUNTW ){
				// account found, set new random password and save it
				m_strPassword = CreateRandomPW();
				if ( !SUCCEEDED(pChild->SetPassword(m_strPassword.AllocSysString())) )
					throw CString("Failed to set password");

				bResult = true;
				break;
			}
		}

	}
	catch(CString error){
		// clean up and abort
		theApp.QueueDebugLogLine(false, _T("Run as unpriveleged user: Exception while preparing user account: %s!"), error);
		CoUninitialize();
		return false;
	}

	if (bResult || CreateEmuleUser(pUsers) ){
		bResult = SetDirectoryPermissions();
	}

	CoUninitialize();
	FreeAPI();
	return bResult;
}

bool CSecRunAsUser::CreateEmuleUser(IADsContainerPtr pUsers){

	IDispatchPtr pDisp=NULL;
	if ( !SUCCEEDED(pUsers->Create(L"user",CString(EMULEACCOUNT).AllocSysString() ,&pDisp )) )
		return false;

	IADsUserPtr pUser;
	if (!SUCCEEDED(pDisp->QueryInterface(&pUser)) )
		return false;

	VARIANT_BOOL bAccountDisabled=FALSE;
	VARIANT_BOOL bIsLocked=FALSE;
	VARIANT_BOOL bPwRequired=TRUE;
	pUser->put_AccountDisabled(bAccountDisabled);
	pUser->put_IsAccountLocked(bIsLocked);
	pUser->put_PasswordRequired(bPwRequired);
	pUser->put_Description(CString(_T("Account used to run eMule with additional security")).AllocSysString() );
	pUser->SetInfo();
	m_strPassword = CreateRandomPW();
	if ( !SUCCEEDED(pUser->SetPassword(m_strPassword.AllocSysString())) )
		return false;
	return true;
}

CStringW CSecRunAsUser::CreateRandomPW(){
	CStringW strResult;
	while (strResult.GetLength() < 10){
		char chRnd=48 + (rand() % 74);
		if( (chRnd > 97 && chRnd < 122) || (chRnd > 65 && chRnd < 90)
			|| (chRnd >48 && chRnd < 57) ||(chRnd==95) ){
				strResult.AppendChar(chRnd);
		}
	}
	return strResult;
}

bool CSecRunAsUser::SetDirectoryPermissions(){
#define FULLACCESS ADS_RIGHT_GENERIC_ALL
	// shared files list: read permission only
	// we odnt check for success here, for emule will also run if one dir fails for some reason
	// if there is a dir which is also an incoming dir, rights will be overwritten below
	for (POSITION pos = thePrefs.shareddir_list.GetHeadPosition();pos != 0;)
	{
		VERIFY( SetObjectPermission(thePrefs.shareddir_list.GetNext(pos), ADS_RIGHT_GENERIC_READ) );
	}

	// set special permission for emule account on needed folders
	bool bSucceeded = true;
	bSucceeded = bSucceeded && SetObjectPermission(thePrefs.GetAppDir(), FULLACCESS);
	bSucceeded = bSucceeded && SetObjectPermission(thePrefs.GetConfigDir(), FULLACCESS);
	bSucceeded = bSucceeded && SetObjectPermission(thePrefs.GetTempDir(), FULLACCESS);
	bSucceeded = bSucceeded && SetObjectPermission(thePrefs.GetIncomingDir(), FULLACCESS);

	uint16 cCats = thePrefs.GetCatCount();
	for (int i= 0; i!= cCats; i++){
		if (!CString(thePrefs.GetCatPath(i)).IsEmpty())
			bSucceeded = bSucceeded && SetObjectPermission(thePrefs.GetCatPath(i), FULLACCESS);
	}
	if (!bSucceeded)
		theApp.QueueDebugLogLine(false, _T("Run as unpriveleged user: Error: Failed to set directoy permissions!"));
	
	return bSucceeded;
}

bool CSecRunAsUser::SetObjectPermission(CString strDirFile, DWORD lGrantedAccess){
	USES_CONVERSION;
	if (!m_hADVAPI32_DLL){
		ASSERT ( false );
		return false;
	}
	if ( strDirFile.IsEmpty() )
		return true;

	SID_NAME_USE   snuType;
	TCHAR* szDomain = NULL;
	LPVOID pUserSID = NULL;
	PACL pNewACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	bool fAPISuccess;
	
	try {
		// get user sid
		DWORD cbDomain = 0;
		DWORD cbUserSID = 0;
		fAPISuccess = LookupAccountName(NULL, EMULEACCOUNT, pUserSID, &cbUserSID, szDomain, &cbDomain, &snuType);
		if ( (!fAPISuccess) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			throw CString(_T("Run as unpriveleged user: Error: LookupAccountName() failed,"));

		pUserSID = MHeapAlloc(cbUserSID);
		if (!pUserSID)
			throw CString(_T("Run as unpriveleged user: Error: Allocating memory failed,"));
		
		szDomain = (TCHAR*)MHeapAlloc(cbDomain * sizeof(TCHAR));
		if (!szDomain)
			throw CString(_T("Run as unpriveleged user: Error: Allocating memory failed,"));

		fAPISuccess = LookupAccountName(NULL, EMULEACCOUNT, pUserSID, &cbUserSID, szDomain, &cbDomain, &snuType);

		if (!fAPISuccess)
			throw CString(_T("Run as unpriveleged user: Error: LookupAccountName()2 failed"));

		if (CStringW(T2W(szDomain)) != m_strDomain)
			throw CString(_T("Run as unpriveleged user: Logical Error: Domainname mismatch"));

		// get old ACL

		PACL pOldACL = NULL;
		if (GetNamedSecurityInfo(strDirFile.GetBuffer(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldACL, NULL, &pSD) != ERROR_SUCCESS){
			throw CString(_T("Run as unpriveleged user: Error: GetNamedSecurityInfo() failed"));
		}

		// calculate size for new ACL
		ACL_SIZE_INFORMATION AclInfo;
		AclInfo.AceCount = 0; // Assume NULL DACL.
		AclInfo.AclBytesFree = 0;
		AclInfo.AclBytesInUse = sizeof(ACL);

		if (pOldACL != NULL && !GetAclInformation(pOldACL, &AclInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))  
			throw CString(_T("Run as unpriveleged user: Error: GetAclInformation() failed"));

		// Create new ACL
		DWORD cbNewACL = AclInfo.AclBytesInUse + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pUserSID) - sizeof(DWORD);

		pNewACL = (PACL)MHeapAlloc(cbNewACL);
		if (!pNewACL)
			throw CString(_T("Run as unpriveleged user: Error: Allocating memory failed,"));

		if (!InitializeAcl(pNewACL, cbNewACL, ACL_REVISION2))
			throw CString(_T("Run as unpriveleged user: Error: Allocating memory failed,"));


		// copy the entries form the old acl into the new one and enter a new ace in the right order
		uint32 newAceIndex = 0;
		uint32 CurrentAceIndex = 0;
		if (AclInfo.AceCount) {
			for (CurrentAceIndex = 0; CurrentAceIndex < AclInfo.AceCount; CurrentAceIndex++) {

					LPVOID pTempAce = NULL;
					if (!GetAce(pOldACL, CurrentAceIndex, &pTempAce))
						throw CString(_T("Run as unpriveleged user: Error: GetAce() failed,"));

					if (((ACCESS_ALLOWED_ACE *)pTempAce)->Header.AceFlags
						& INHERITED_ACE)
						break;
					// no multiple entries
					if (EqualSid(pUserSID, &(((ACCESS_ALLOWED_ACE *)pTempAce)->SidStart)))
						continue;

					if (!AddAce(pNewACL, ACL_REVISION, MAXDWORD, pTempAce, ((PACE_HEADER) pTempAce)->AceSize))
						throw CString(_T("Run as unpriveleged user: Error: AddAce()1 failed,"));
					newAceIndex++;
			}
		}
		// here we add the actually entry
		if (!AddAccessAllowedAceEx(pNewACL, ACL_REVISION2, CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE, lGrantedAccess, pUserSID))
			throw CString(_T("Run as unpriveleged user: Error: AddAccessAllowedAceEx() failed,"));	
		
		// copy the rest
		if (AclInfo.AceCount) {
			for (; CurrentAceIndex < AclInfo.AceCount; CurrentAceIndex++) {

					LPVOID pTempAce = NULL;
					if (!GetAce(pOldACL, CurrentAceIndex, &pTempAce))
						throw CString(_T("Run as unpriveleged user: Error: GetAce()2 failed,"));

					if (!AddAce(pNewACL, ACL_REVISION, MAXDWORD, pTempAce, ((PACE_HEADER) pTempAce)->AceSize))
						throw CString(_T("Run as unpriveleged user: Error: AddAce()2 failed,"));
				}
		}


		if (SetNamedSecurityInfo(strDirFile.GetBuffer(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewACL, NULL) != ERROR_SUCCESS)
			throw CString(_T("Run as unpriveleged user: Error: SetNamedSecurityInfo() failed,"));
		fAPISuccess = true;
	}
	catch(CString error){
		fAPISuccess = false;
		theApp.QueueDebugLogLine(false, error);
	}
	// clean up
	if (pUserSID != NULL)
		MHeapFree(pUserSID);
	if (szDomain != NULL)
		MHeapFree(szDomain);
	if (pNewACL != NULL)
		MHeapFree(pNewACL);
	if (pSD != NULL)
		LocalFree(pSD);

	// finished
	return fAPISuccess;
}

bool CSecRunAsUser::RestartAsUser(){
	USES_CONVERSION;

	if (!LoadAPI())
		return false;

	ASSERT ( !m_strPassword.IsEmpty() );
	bool bResult;
	try{
		PROCESS_INFORMATION ProcessInfo = {0};
		TCHAR szAppPath[MAX_PATH];
		GetModuleFileName(NULL, szAppPath, MAX_PATH);
		CString strAppName;
		strAppName.Format(_T("\"%s\""),szAppPath);
		
		STARTUPINFOW StartInf = {0};
		StartInf.cb = sizeof(StartInf);
		StartInf.dwFlags = STARTF_USESHOWWINDOW;
		StartInf.wShowWindow = SW_NORMAL;

		// remove the current mutex, so that the restart emule can create its own without problems
		// in the rare case CreateProcessWithLogonW fails, this will allow mult. instances, but if that function fails we have other problems anyway
		::CloseHandle(theApp.m_hMutexOneInstance);
		
		bResult = CreateProcessWithLogonW(EMULEACCOUNTW, m_strDomain, m_strPassword,
			LOGON_WITH_PROFILE, NULL, (LPWSTR)T2CW(strAppName), 0, NULL, NULL, &StartInf, &ProcessInfo);

	}
	catch(...){
		theApp.QueueDebugLogLine(false, _T("Run as unpriveleged user: Error: Unexpected exception while loading advapi32.dll"));
		FreeAPI();
		return false;
	}
	FreeAPI();
	if (!bResult)
		theApp.QueueDebugLogLine(false, _T("Run as unpriveleged user: Error: Failed to restart eMule as different user! Error Code: %i"),GetLastError());
	return bResult;
}

CStringW CSecRunAsUser::GetCurrentUserW(){
	USES_CONVERSION;
	if ( m_strCurrentUser.IsEmpty() )
		return T2W(_T("Unknown"));
	else
		return m_strCurrentUser;
}

bool CSecRunAsUser::LoadAPI(){
	if (m_hADVAPI32_DLL == 0)
		m_hADVAPI32_DLL = LoadLibrary(_T("Advapi32.dll"));
	if (m_hACTIVEDS_DLL == 0)
		m_hACTIVEDS_DLL = LoadLibrary(_T("ActiveDS"));

    if (m_hADVAPI32_DLL == 0) {
        AddDebugLogLine(false,_T("Failed to load Advapi32.dll!"));
        return false;
    }
    if (m_hACTIVEDS_DLL == 0) {
        AddDebugLogLine(false,_T("Failed to load ActiveDS.dll!"));
        return false;
    }

	bool bSucceeded = true;
	bSucceeded = bSucceeded && (CreateProcessWithLogonW = (TCreateProcessWithLogonW) GetProcAddress(m_hADVAPI32_DLL,"CreateProcessWithLogonW")) != NULL;
	bSucceeded = bSucceeded && (GetNamedSecurityInfo = (TGetNamedSecurityInfo)GetProcAddress(m_hADVAPI32_DLL,"GetNamedSecurityInfoA")) != NULL;
	bSucceeded = bSucceeded && (SetNamedSecurityInfo = (TSetNamedSecurityInfo)GetProcAddress(m_hADVAPI32_DLL,"SetNamedSecurityInfoA")) != NULL;
	bSucceeded = bSucceeded && (AddAccessAllowedAceEx = (TAddAccessAllowedAceEx)GetProcAddress(m_hADVAPI32_DLL,"AddAccessAllowedAceEx")) != NULL;
	// Probably these functions do not need to bel loaded dynamically, but just to be sure
	bSucceeded = bSucceeded && (LookupAccountName = (TLookupAccountName)GetProcAddress(m_hADVAPI32_DLL,"LookupAccountNameA")) != NULL;
	bSucceeded = bSucceeded && (GetAclInformation = (TGetAclInformation)GetProcAddress(m_hADVAPI32_DLL,"GetAclInformation")) != NULL;
	bSucceeded = bSucceeded && (InitializeAcl = (TInitializeAcl)GetProcAddress(m_hADVAPI32_DLL,"InitializeAcl")) != NULL;
	bSucceeded = bSucceeded && (GetAce = (TGetAce)GetProcAddress(m_hADVAPI32_DLL,"GetAce")) != NULL;
	bSucceeded = bSucceeded && (AddAce = (TAddAce)GetProcAddress(m_hADVAPI32_DLL,"AddAce")) != NULL;
	bSucceeded = bSucceeded && (EqualSid = (TEqualSid)GetProcAddress(m_hADVAPI32_DLL,"EqualSid")) != NULL;
	bSucceeded = bSucceeded && (GetLengthSid = (TGetLengthSid)GetProcAddress(m_hADVAPI32_DLL,"GetLengthSid")) != NULL;
	
	// activeDS.dll
	bSucceeded = bSucceeded && (ADsGetObject = (TADsGetObject)GetProcAddress(m_hACTIVEDS_DLL,"ADsGetObject")) != NULL;
	bSucceeded = bSucceeded && (ADsBuildEnumerator = (TADsBuildEnumerator)GetProcAddress(m_hACTIVEDS_DLL,"ADsBuildEnumerator")) != NULL;
	bSucceeded = bSucceeded && (ADsEnumerateNext = (TADsEnumerateNext)GetProcAddress(m_hACTIVEDS_DLL,"ADsEnumerateNext")) != NULL;
	
	if (!bSucceeded){
		AddDebugLogLine(false,_T("Failed to load all functions from Advapi32.dll!"));
		FreeAPI();
		return false;
	}
	return true;
}

void CSecRunAsUser::FreeAPI(){
	if (m_hADVAPI32_DLL != 0){
		FreeLibrary(m_hADVAPI32_DLL);
		m_hADVAPI32_DLL = 0;
	}
	if (m_hACTIVEDS_DLL != 0){
		FreeLibrary(m_hACTIVEDS_DLL);
		m_hACTIVEDS_DLL = 0;
	}

}