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
#include "StdAfx.h"
#include <windns.h>
#include "peercachefinder.h"
#include "emule.h"
#include "emuledlg.h"
#include "otherfunctions.h"
#include "Preferences.h"
#include "md5sum.h"
#pragma warning(disable:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#pragma warning(disable:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4702) // unreachable code
#pragma warning(disable:4189) // local variable is initialized but not referenced
#include <cryptopp/rsa.h>
#pragma warning(default:4189) // local variable is initialized but not referenced
#pragma warning(default:4702) // unreachable code
#pragma warning(default:4100) // unreferenced formal parameter
#pragma warning(default:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(default:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#include "Log.h"
#include "UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define PCBASELOCATION	"edcache.p2p."
#define RETRYDAYS		2
#define SIGNATURELENGTH 256

static char s_acDNSBuffer[MAXGETHOSTSTRUCT];

static const uint16 anPeerCachPorts[3] = {4662, 1214, 80};

static const uchar achVerify_Key[256] = {
    0xCE, 0x9A, 0x90, 0x08, 0x47, 0xE7, 0x1E, 0x15, 0x94, 0xFD, 0x65, 0xC7, 0x82, 0xF9, 0xB2, 
    0xB7, 0xB7, 0x6A, 0xF2, 0x4A, 0x31, 0x08, 0xFC, 0x36, 0x6C, 0xB1, 0xD9, 0x3F, 0x3B, 0x94, 0x6B, 
    0xC4, 0x55, 0x39, 0xAF, 0x53, 0x4D, 0x78, 0xE7, 0x1A, 0xE9, 0x83, 0x34, 0x1D, 0x6C, 0xE3, 0xEF, 
    0xAD, 0x56, 0x88, 0x0D, 0x38, 0x94, 0x86, 0x0F, 0x81, 0x7C, 0x9C, 0x21, 0xCF, 0x49, 0xDA, 0xE8, 
    0x13, 0x38, 0x30, 0x88, 0xBD, 0x6A, 0xCE, 0x66, 0x4C, 0xB0, 0x36, 0x3E, 0x43, 0x0B, 0x74, 0x1F, 
    0xCC, 0xA8, 0xB6, 0x7A, 0xC9, 0xEE, 0x45, 0xBD, 0xB6, 0x2C, 0x55, 0xA7, 0x8E, 0x20, 0x65, 0x60, 
    0x54, 0x0B, 0xBF, 0x68, 0x99, 0x4B, 0x30, 0x69, 0x16, 0xF7, 0xB7, 0x6E, 0xFC, 0x4E, 0x4B, 0x4F, 
    0xD8, 0x28, 0x56, 0x4E, 0xC2, 0xCE, 0x91, 0x0C, 0x74, 0xFB, 0xAE, 0x79, 0xAB, 0x86, 0x03, 0xEA, 
    0x35, 0x92, 0xB1, 0x10, 0x4B, 0x7F, 0xF2, 0xA9, 0x39, 0x56, 0xDB, 0xFF, 0x83, 0x5E, 0x44, 0x8F, 
    0xF2, 0x41, 0x61, 0x62, 0x74, 0x36, 0xB3, 0xCF, 0x3E, 0xE5, 0xAC, 0x0B, 0x04, 0x1B, 0x2B, 0x59, 
    0x81, 0x8A, 0xFC, 0x80, 0xC5, 0x4B, 0x36, 0x2E, 0x5F, 0xD0, 0xD3, 0x66, 0x7B, 0x40, 0xD3, 0x82, 
    0x57, 0xF7, 0xA5, 0x05, 0x8D, 0xA9, 0xC9, 0xEE, 0xA8, 0x5F, 0x82, 0x5F, 0xD5, 0x4B, 0xBB, 0x00, 
    0x4D, 0xA3, 0xD8, 0x72, 0x48, 0x84, 0xC0, 0x4B, 0x59, 0x6E, 0xDC, 0x49, 0xCF, 0x00, 0xDD, 0x7C, 
    0xF8, 0xB7, 0xAC, 0x7A, 0xD8, 0x43, 0x6F, 0xA4, 0x21, 0x51, 0x93, 0xDF, 0x58, 0x9A, 0xC1, 0xC7, 
    0x77, 0x45, 0xD7, 0xBE, 0x9B, 0x55, 0x0F, 0x2E, 0xC9, 0xD5, 0x85, 0x8F, 0xB5, 0xF9, 0xF0, 0x49, 
    0xF6, 0x85, 0x24, 0x7A, 0xA8, 0x74, 0x64, 0xB1, 0x8B, 0x71, 0x63, 0xFC, 0x1F, 0x1B, 0x5E, 0x26, 
    0xF5, 
} ;

CPeerCacheFinder::CPeerCacheFinder()
{
	m_dwPCIP = 0;
	m_PCStatus = PCS_NOINIT;
	m_PCLUState = LUS_NONE;
	m_posCurrentLookUp = 0;
	m_dwMyIP = 0;
	m_bValdited = false;
	m_bNotReValdited = false;
	m_bNotReSearched = false;
	m_nDownloadAttempts = 0;
	m_nFailedDownloads = 0;
	m_nPCPort = 0;
}

CPeerCacheFinder::~CPeerCacheFinder(void)
{

}

void CPeerCacheFinder::Save(){
	if (m_PCStatus > 10){ // permanent errorrange
		if (!m_bNotReSearched){
			struct tm tmTemp;
			thePrefs.SetPeerCacheLastSearch(mktime(CTime::GetCurrentTime().GetLocalTm(&tmTemp)));
			thePrefs.SetPeerCacheWasFound(false);
			thePrefs.SetPeerCachePort(0);
		}
	}
	else if (m_PCStatus == PCS_READY){
		if (!m_bNotReValdited){
			struct tm tmTemp;
			thePrefs.SetPeerCacheLastSearch(mktime(CTime::GetCurrentTime().GetLocalTm(&tmTemp)));
		}
		thePrefs.SetPeerCachePort(m_nPCPort);
		thePrefs.SetPeerCacheWasFound(true);
	}
}

void CPeerCacheFinder::Init(uint32 dwLastSearch, bool bLastSearchSuccess, bool bEnabled, uint16 nPort){
	if (!bEnabled){
		m_PCStatus = PCS_DISABLED;
	}
	else{
		if (bLastSearchSuccess){ // sanitycheck
			bool bOK = false;
			for (int i = 0; i < _countof(anPeerCachPorts); i++){
				if(anPeerCachPorts[i] == nPort)
					bOK = true;
			}
			if (!bOK){
				dwLastSearch = 0;
				bLastSearchSuccess = false;
				DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("Sanitycheck for PeerCache-Port '%u' failed"), nPort));
			}
			else
				m_nPCPort = nPort;
		}

		if (dwLastSearch != 0){
			CTime last(dwLastSearch);
			struct tm tmTemp;
			time_t tLast = safe_mktime(last.GetLocalTm(&tmTemp));
			time_t tNow = safe_mktime(CTime::GetCurrentTime().GetLocalTm(&tmTemp));
			if ( (difftime(tNow,tLast) / 86400) < RETRYDAYS ){
				if (!bLastSearchSuccess){
					// no retry to find the cache
					m_PCStatus = PCS_NOTFOUND;
					m_bNotReSearched = true;
					DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: Not starting a search - last failed search is too near")));
					return;
				}
				else{
					// no need to revaldite the cache yet
					m_bValdited = true;
					m_bNotReValdited = true;
					DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: CacheIdent still valid, not trying to revaldite this time")));
				}
			}
			else
				m_bNotReValdited = false;
		}
		m_bNotReSearched = false;
		SearchForPC();
	}
}

void CPeerCacheFinder::FoundMyPublicIPAddress(uint32 dwIP){
	m_dwMyIP = dwIP;
	if (m_PCStatus == PCS_OWNIPUNKNOWN){
		m_PCStatus = PCS_DOINGLOOKUPS;
		SearchForPC();
	}
}

void CPeerCacheFinder::SearchForPC(){
	switch(m_PCStatus){
		case PCS_NOINIT:
			m_PCStatus = PCS_DOINGLOOKUPS;
		case PCS_DOINGLOOKUPS:
			switch(m_PCLUState){
				case LUS_NONE:
					// ok lets start, base pos first
					m_PCLUState = LUS_BASEPCLOCATION;
					DoLookUp(PCBASELOCATION);
					break;
				case LUS_BASEPCLOCATION:
					// so the first lookup failed, now we need our hostname
					if (m_dwMyIP == 0){
						// we have to wait until we now our IP for a reverse lookup
						m_PCStatus = PCS_OWNIPUNKNOWN;
						break;
					}
					m_PCLUState = LUS_MYHOSTNAME;
					DoReverseLookUp(m_dwMyIP);
					break;
				case LUS_MYHOSTNAME:
					// we should have found the hostname and now start to contruct sublocations to lookup
					m_PCLUState = LUS_EXTPCLOCATION;
					m_posCurrentLookUp = 0;
				case LUS_EXTPCLOCATION:
					ASSERT ( !m_strMyHostname.IsEmpty() );
					// build string
					m_posCurrentLookUp = m_strMyHostname.Find('.',m_posCurrentLookUp+1);
					if (m_posCurrentLookUp == -1 || m_posCurrentLookUp+1 >= m_strMyHostname.GetLength() || m_strMyHostname.Find('.',m_posCurrentLookUp+1) == (-1) ){
						// failed:
						m_PCLUState = LUS_NONE;
						m_PCStatus = PCS_NOTFOUND;
					}
					else{
						DoLookUp(PCBASELOCATION + CStringA(m_strMyHostname.Mid(m_posCurrentLookUp+1)));
					}
					break;
				case LUS_FINISHED:
					if (m_dwMyIP == 0){
						// we have to wait until we now our IP for a reverse lookup
						m_PCStatus = PCS_OWNIPUNKNOWN;
						break;
					}
					if (!m_bValdited){
						m_PCStatus = PCS_VALDITING;
						ValditeDescriptorFile();
					}
					else{
						DEBUG_ONLY(theApp.QueueLogLine(false, GetResString(IDS_PEERCACHE_ENABLED)));
						m_PCStatus = PCS_READY;
					}
					break;
			}
			break;
	}
}

LRESULT CPeerCacheFinder::OnPeerCacheCheckResponse(WPARAM /*wParam*/, LPARAM lParam)
{
	if (m_PCLUState == LUS_MYHOSTNAME){
		if (WSAGETASYNCERROR(lParam) == 0)
		{
			int iBufLen = WSAGETASYNCBUFLEN(lParam);
			if (iBufLen >= sizeof(HOSTENT))
			{
				LPHOSTENT pHost = (LPHOSTENT)s_acDNSBuffer;
				m_strMyHostname = pHost->h_name;
				if (!m_strMyHostname.IsEmpty()){
					DEBUG_ONLY(AddDebugLogLine(false, _T("PeerCache: Found my Hostname: %s, continue search"), m_strMyHostname));
					SearchForPC();
					return 0;
				}
			}
		}
		m_PCStatus = PCS_NOTFOUND;
		DEBUG_ONLY(AddDebugLogLine(false, _T("DNS Reverse Lookup for own IP failed, aborting PC search")));

	}
	else{
		if (WSAGETASYNCERROR(lParam) == 0)
		{
			int iBufLen = WSAGETASYNCBUFLEN(lParam);
			if (iBufLen >= sizeof(HOSTENT))
			{
				LPHOSTENT pHost = (LPHOSTENT)s_acDNSBuffer;
				if (pHost->h_length == 4 && pHost->h_addr_list && pHost->h_addr_list[0])
				{
					m_dwPCIP = ((LPIN_ADDR)(pHost->h_addr_list[0]))->s_addr;
					DEBUG_ONLY(AddDebugLogLine(false, _T("Found PeerCache IP: %s"), ipstr(m_dwPCIP) ));
					
					m_PCLUState = LUS_FINISHED;
					SearchForPC();
					return 0;
				}
			}
		}
		// no luck, continue search
		DEBUG_ONLY(AddDebugLogLine(false, _T("DNS Lookup for PC, state %i, failed - PC not found yet"), m_PCLUState));
		SearchForPC();
	}
	return 0;
}

void CPeerCacheFinder::DoLookUp(CStringA strHostname){
	if (WSAAsyncGetHostByName(theApp.emuledlg->m_hWnd, UM_PEERCHACHE_RESPONSE, strHostname, s_acDNSBuffer, sizeof(s_acDNSBuffer)) == 0){
		DEBUG_ONLY(AddDebugLogLine(false, _T("DNS Lookup for PC, state %i, failed (DoLookUP) - PC not found yet"), m_PCLUState));
	}
}

bool CPeerCacheFinder::IsCacheAvailable() const { 
	if(m_nDownloadAttempts > 20 && m_nFailedDownloads > 0){
		if ( (float)(m_nDownloadAttempts/m_nFailedDownloads) < (float)2)
			return false;
	}
	return m_PCStatus == PCS_READY && theApp.GetPublicIP() != 0 && m_nPCPort != 0; 
}


CString ReverseDnsLookup(DWORD dwIP)
{
	CString strHostName;

	HMODULE hLib = LoadLibrary(_T("dnsapi.dll"));
	if (hLib)
	{
		DNS_STATUS (WINAPI *pfnDnsQueryConfig)(DNS_CONFIG_TYPE Config, DWORD Flag, PWSTR pwsAdapterName, PVOID pReserved, PVOID pBuffer, PDWORD pBufferLength);
		DNS_STATUS (WINAPI *pfnDnsQuery)(PCTSTR pszName, WORD wType, DWORD Options, PIP4_ARRAY aipServers, PDNS_RECORD* ppQueryResults, PVOID* pReserved);
		VOID (WINAPI *pfnDnsRecordListFree)(PDNS_RECORD pRecordList, DNS_FREE_TYPE FreeType);

		(FARPROC&)pfnDnsQueryConfig = GetProcAddress(hLib, "DnsQueryConfig");
		(FARPROC&)pfnDnsQuery = GetProcAddress(hLib, _TWINAPI("DnsQuery_"));
		(FARPROC&)pfnDnsRecordListFree = GetProcAddress(hLib, "DnsRecordListFree");

		if (pfnDnsQueryConfig && pfnDnsQuery && pfnDnsRecordListFree)
		{
			// WinXP: We explicitly need to pass the DNS servers to be used to DnsQuery, 
			// otherwise that function will never query a DNS server for the *local* host name.
			IP4_ARRAY* pDnsServers = NULL;
			BYTE aucBuff[16384];
			memset(aucBuff, 0, sizeof aucBuff);
			DWORD dwSize = sizeof aucBuff;
			DNS_STATUS nDnsState = (*pfnDnsQueryConfig)(DnsConfigDnsServerList, FALSE, NULL, NULL, aucBuff, &dwSize);
			if (nDnsState == 0)
			{
				if (dwSize >= 4)
				{
					DWORD dwDnsServers = ((DWORD*)aucBuff)[0];
					if (dwDnsServers >= 1 && dwSize == sizeof(DWORD) + dwDnsServers*sizeof(DWORD))
					{
						UINT uArrSize = sizeof(IP4_ARRAY) + sizeof(IP4_ADDRESS)*dwDnsServers;
						pDnsServers = (IP4_ARRAY*)new BYTE[uArrSize];
						memset(pDnsServers, 0, uArrSize);
						pDnsServers->AddrCount = dwDnsServers;
						for (UINT s = 0; s < dwDnsServers; s++)
							pDnsServers->AddrArray[s] = ((DWORD*)aucBuff)[1+s];
					}
				}
			}
			else{
				if (thePrefs.GetVerbose())
					DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("ReverseDNS: Failed to get list of DNS servers - %s"), GetErrorMessage(nDnsState, 1)));
			}

			CString strDnsQuery;
			strDnsQuery.Format(_T("%u.%u.%u.%u.IN-ADDR.ARPA"), (dwIP >> 24) & 0xFF, (dwIP >> 16) & 0xFF, (dwIP >> 8) & 0xFF, (dwIP >> 0) & 0xFF);

			// This is a *blocking* call!
			PDNS_RECORD pDnsRecords = NULL;
			nDnsState = (*pfnDnsQuery)(strDnsQuery, DNS_TYPE_PTR, DNS_QUERY_BYPASS_CACHE, pDnsServers, &pDnsRecords, NULL);
			if (nDnsState == 0)
			{
				if (!IsBadReadPtr(pDnsRecords, sizeof(*pDnsRecords) - sizeof(pDnsRecords->Data) + sizeof(pDnsRecords->Data.PTR)))
				{
					if (!IsBadReadPtr(pDnsRecords->Data.PTR.pNameHost, sizeof(TCHAR)))
						strHostName = pDnsRecords->Data.PTR.pNameHost;
					(*pfnDnsRecordListFree)(pDnsRecords, DnsFreeRecordListDeep);
				}
			}
			else{
				if (thePrefs.GetVerbose())
					DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("ReverseDNS: Failed to resolve address \"%s\" - %s"), strDnsQuery, GetErrorMessage(nDnsState, 1)));
			}

			delete[] (BYTE*)pDnsServers;
		}
		FreeLibrary(hLib);
	}

	return strHostName;
}

void CPeerCacheFinder::DoReverseLookUp(uint32 dwIP){
	CPCReverseDnsThread* pThread = (CPCReverseDnsThread*)AfxBeginThread(RUNTIME_CLASS(CPCReverseDnsThread), THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED);
	pThread->m_dwIP = dwIP;
	pThread->m_hwndAsyncResult = theApp.emuledlg->m_hWnd;
	pThread->ResumeThread();
}

void CPeerCacheFinder::ValditeDescriptorFile(){
	CPCValditeThread* pValditeThread = (CPCValditeThread*) AfxBeginThread(RUNTIME_CLASS(CPCValditeThread), THREAD_PRIORITY_BELOW_NORMAL,0, CREATE_SUSPENDED);
	pValditeThread->SetValues(this, m_dwPCIP, m_dwMyIP); 
	pValditeThread->ResumeThread();	
}

void	CPeerCacheFinder::AddBannedVersion(CClientVersionInfo cviVersion){
	// Thread safe, due to logic this is not really needed at this time
	// (because noone will access the list while the ValditeThread is running),
	// but better to be on the safe side in case the code changes
	CSingleLock lock(&m_SettingsMutex, TRUE);
	liBannedVersions.Add(cviVersion);
}

void CPeerCacheFinder::AddAllowedVersion(CClientVersionInfo cviVersion){
	CSingleLock lock(&m_SettingsMutex, TRUE);
	liAllowedVersions.Add(cviVersion);
}

bool CPeerCacheFinder::IsClientPCCompatible(uint32 dwTagVersionInfo, UINT nClientSoft){
	return IsClientPCCompatible(CClientVersionInfo(dwTagVersionInfo, nClientSoft));
}

bool CPeerCacheFinder::IsClientPCCompatible(const CClientVersionInfo& cviToCheck){
	CSingleLock lock(&m_SettingsMutex, TRUE);
	for (int i = 0; i != liAllowedVersions.GetSize(); i++){
		if (cviToCheck < liAllowedVersions[i])
			return false;
	}
	for (int i = 0; i != liBannedVersions.GetSize(); i++){
		if (cviToCheck == liBannedVersions[i])
			return false;
	}
	return true;
}

void CPeerCacheFinder::DownloadAttemptFailed(){
	m_nFailedDownloads++;
	if(m_nDownloadAttempts > 20 && m_nFailedDownloads > 0){
		DEBUG_ONLY(AddDebugLogLine(DLP_LOW, false, _T("PeerCache fail value: %0.2f"), (float)(m_nDownloadAttempts/m_nFailedDownloads)));
		if ( (float)(m_nDownloadAttempts/m_nFailedDownloads) < (float)2)
			AddDebugLogLine(DLP_LOW, false, _T("PeerCache fail value too high, disabling cache downloads"));
	}
}

///////////////////////////////////////////////////////////////////////////////////////
/// CPCValditeThread

IMPLEMENT_DYNCREATE(CPCValditeThread, CWinThread)

CPCValditeThread::CPCValditeThread()
{
}

CPCValditeThread::~CPCValditeThread()
{
}

BOOL CPCValditeThread::InitInstance()
{
	DbgSetThreadName("PCValditeThread");
	InitThreadLocale();
	return TRUE;
}

BOOL CPCValditeThread::Run(){
	// SLUGFILLER: SafeHash
	CReadWriteLock lock(&theApp.m_threadlock);
	if (!lock.ReadLock(0))
		return 0;
	// SLUGFILLER: SafeHash

	if (theApp.emuledlg != NULL && theApp.emuledlg->IsRunning()){
		if (Valdite()){
			m_pOwner->m_bValdited = true;
			m_pOwner->m_nPCPort = m_nPCPort;
			if (m_pOwner->m_PCStatus == PCS_VALDITING){
				DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: Valditing .p2pinfo succeeded")));
				DEBUG_ONLY(theApp.QueueLogLine(false, GetResString(IDS_PEERCACHE_ENABLED)));
				m_pOwner->m_PCStatus = PCS_READY;
			}
			else
				ASSERT ( false );
		}
		else{
			m_pOwner->m_bValdited = false;
			m_pOwner->m_PCStatus = PCS_NOTVERIFIED;
			m_pOwner->m_nPCPort = 0;
		}
	}
	return 0;
}

void CPCValditeThread::SetValues(CPeerCacheFinder* in_pOwner, uint32 dwPCIP, uint32 dwMyIP){
	m_dwPCIP = dwPCIP;
	m_dwMyIP = dwMyIP;
	m_pOwner = in_pOwner;
}

bool CPCValditeThread::Valdite(){
	ASSERT ( m_dwPCIP != 0 );
	ASSERT ( m_dwMyIP != 0 );

	CInternetSession session;
	CInternetFile* file = NULL;
	for (int i = 0; i < _countof(anPeerCachPorts); i++){
		try
		{
			// try to connect to the URL
			CString strURL;
			strURL.Format(_T("http://%s:%u/.p2pinfo"), ipstr(m_dwPCIP), anPeerCachPorts[i]);
			file = (CInternetFile*)session.OpenURL(strURL);
			m_nPCPort = anPeerCachPorts[i];
			break;
		}
		catch (CInternetException* pException){
			// set file to NULL if there's an error
			file = NULL;
			CString strError;
			pException->GetErrorMessage(strError.GetBuffer(512), 512);
			strError.ReleaseBuffer();
			strError.Trim(_T(" \r\n"));
			pException->Delete();
			DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: Failed to retrieve .p2pinfo file on Port %u - %s"),anPeerCachPorts[i], strError));
			if (i == (_countof(anPeerCachPorts)-1)){ // was last try
				DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: Failed to retrieve .p2pinfo file, cache disabled"),anPeerCachPorts[i]));
				return false;
			}
		}
	}
	ASSERT( file != NULL );
	// TODO: This will work successfully only for very small files which were received completely right at the
	// time the 'GetLength' function is called. -> Use a polling loop which reads everything up to get bytes
	// specified in 'Content-Length' or some other appropriate limit in case 'Content-Length' is missing.
	uint32 nIFileSize = (UINT)file->GetLength();
	ASSERT (nIFileSize > SIGNATURELENGTH);

	if (file && nIFileSize > SIGNATURELENGTH){
		// read content
		CArray<uint32, uint32> adwCacheIPs;
		CStringAArray astrIPRanges;
		CStringA strLine;
		bool bContentCheckFailed = false;

		UINT uContentSize = nIFileSize - SIGNATURELENGTH;
		char* pcContent = new char[uContentSize + 1];
		if (file->Read(pcContent, uContentSize) != uContentSize)
			DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: Failed to read p2pinfo file content")));
		pcContent[uContentSize] = '\0';

		//----------- CHECKING .P2PINFO CONTENT
		// NOTE: 'pcContent' gets destroyed with 'strtok' !
		LPSTR pszLine = strtok(pcContent, "\r\n");
		while (pszLine && !bContentCheckFailed)
		{
			strLine = pszLine;
			int posSeperator = strLine.Find('=',1);
			if ( posSeperator != -1 && strLine.GetLength() - posSeperator > 1)
			{
				CStringA strTopic = strLine.Left(posSeperator).Trim();
				CStringA strContent = strLine.Mid(posSeperator+1).Trim();

				//DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: Current line to be processed: %hs"),strLine);

				///////***** CacheIP
				if (strTopic == "CacheIP"){
					adwCacheIPs.Add(inet_addr(strContent));
				}
				///////***** ClientIPRange
				else if (strTopic == "ClientIPRange"){
					astrIPRanges.Add(strContent);
				}
				///////***** ExpireDate
				else if (strTopic == "ExpireDate"){
					bool bDateCheckFailed = true;

					if (strContent.GetLength() == 8){
						uint32 nYear =  atol(strContent.Mid(0,4));
						uint32 nMonth = atol(strContent.Mid(4,2));
						uint32 nDay = atol(strContent.Mid(6,2));
						if (nYear <= 3000 && nMonth <= 12 && nDay <= 31){
							CTime timeExpireDate(nYear, nMonth, nDay, 0, 0, 0);
							if (timeExpireDate >= CTime::GetCurrentTime())
								bDateCheckFailed = false;	// check passed
						}
					}
					if (bDateCheckFailed){
						bContentCheckFailed = true;
						DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: ExpireDate check failed. Expiring date: %hs"),strContent));
					}
				}
				///////***** EnableED2K
				else if (strTopic == "EnableED2K"){
					if (atol(strContent) != 1){
						bContentCheckFailed = true;
						DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: EnableED2K check failed. Value: %hs"),strContent));
					}
				}
				else{
					DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: Unused content tag for validity check: %hs"),strLine));
				}

			}

			pszLine = strtok(NULL, "\r\n");
		}
		// finish the CacheIP check
		bool bIPRangeCheckFailed = true;
		for (int i = 0; i != adwCacheIPs.GetCount(); i++){
			if(adwCacheIPs[i] == m_dwPCIP){
				bIPRangeCheckFailed = false;
				break;
			}
		}
		if (bIPRangeCheckFailed){
			bContentCheckFailed = true;
			DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: CacheIP check failed.")));
		}
		// finish the RangeChack
		bool bIPCheckFailed = true;
		for (int i = 0; i != astrIPRanges.GetCount(); i++){ 
			CStringA strCurRange = astrIPRanges[i];
			int posContentSeperator = strCurRange.Find('-',7);
			if ( !(posContentSeperator == -1 || strCurRange.GetLength() - posContentSeperator <= 7) ){
				uint32 dwIPRangeStart = inet_addr(strCurRange.Left(posContentSeperator).Trim());
				uint32 dwIPRangeEnd = inet_addr(strCurRange.Mid(posContentSeperator+1).Trim());
				if (dwIPRangeStart != INADDR_NONE || dwIPRangeEnd != INADDR_NONE){
					if (ntohl(dwIPRangeStart) <= ntohl(m_dwMyIP) && ntohl(dwIPRangeEnd) >= ntohl(m_dwMyIP))
						bIPCheckFailed = false; // check passed	
				}
			}
		}
		if (bIPCheckFailed){
			bContentCheckFailed = true;
			DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: ClientIPRange check failed. my IP: %s"), ipstr(m_dwMyIP)));
		}

		//----------- CHECKING SPECIAL HEADERS CONTENT
		char achBuffer[3072]; // 3KB should be enough for headers
		DWORD nBufferSize = sizeof(achBuffer);
		if( !bContentCheckFailed && HttpQueryInfo(*file, HTTP_QUERY_RAW_HEADERS_CRLF, achBuffer, &nBufferSize, NULL) ){
			CStringA strHeaders = achBuffer;
			int nHeaderPos = 0;
			CStringA strCurHeader = strHeaders.Tokenize("\r\n",nHeaderPos);
			while (!strCurHeader.IsEmpty())
			{
				int posSeperator = strCurHeader.Find(':',1);
				if ( posSeperator != -1 && strCurHeader.GetLength() - posSeperator > 1)
				{
					CStringA strTopic = strCurHeader.Left(posSeperator).Trim();
					CStringA strContent = strCurHeader.Mid(posSeperator+1).Trim();

					DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: Current Header to be processed: %hs"),strCurHeader));
					///////***** X-eMule-Require-Version
					if (strTopic == "X-eMule-Require-Version"){
						int curPos= 0;
						CString strPCEncodedVersion(strContent);
						CString strVersion = strPCEncodedVersion.Tokenize(_T(" "),curPos);
						while (!strVersion.IsEmpty())
						{
							m_pOwner->AddAllowedVersion(CClientVersionInfo(strVersion));
							strVersion = strPCEncodedVersion.Tokenize(_T(" "),curPos);
						}
					} 
					///////***** X-eMule-Reject-Version
					else if (strTopic == "X-eMule-Reject-Version"){
						int curPos= 0;
						CString strPCEncodedVersion(strContent);
						CString strVersion = strPCEncodedVersion.Tokenize(_T(" "),curPos);
						while (!strVersion.IsEmpty())
						{
							m_pOwner->AddBannedVersion(CClientVersionInfo(strVersion));
							strVersion = strPCEncodedVersion.Tokenize(_T(" "),curPos);
						}
					}
				}
				strCurHeader = strHeaders.Tokenize("\r\n",nHeaderPos);
			}
			///////***** Own version check
			if (!m_pOwner->IsClientPCCompatible(CClientVersionInfo(CemuleApp::m_nVersionMjr, CemuleApp::m_nVersionMin, CemuleApp::m_nVersionUpd, CemuleApp::m_nVersionBld, SO_EMULE))){
				bContentCheckFailed = true;
				DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: Current Version not allowed to use this PC-Server, please update")));
			}
		}
		else if (!bContentCheckFailed)
			DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache Error: Failed to retrieve headers, Errornumber %s"),GetErrorMessage(GetLastError()) ));


		//----------- CHECKING .P2PINFO SIGNATURE
		bool bSignatureCheckResult = false;
		if (!bContentCheckFailed){
			uchar* pachCompleteFile = new uchar[nIFileSize];
			file->SeekToBegin();
			if (file->Read(pachCompleteFile, nIFileSize) != nIFileSize){
				ASSERT( false );
				bSignatureCheckResult = false;
			}
			else{
				using namespace CryptoPP;
				Integer e, m, n;
				e = 3;
				n.Decode(achVerify_Key, _countof(achVerify_Key));

				uchar aucSignature[SIGNATURELENGTH];
				for (int i = 0; i != SIGNATURELENGTH; i++)
					aucSignature[(SIGNATURELENGTH-1)-i] = pachCompleteFile[(nIFileSize-SIGNATURELENGTH)+i];
				m.Decode(aucSignature, SIGNATURELENGTH);

				RSAFunction rsa;
				rsa.SetPublicExponent(e);
				rsa.SetModulus(n);
				Integer result = rsa.ApplyFunction(m);
				uchar aucResult[SIGNATURELENGTH];
				//MORPH START - Changed by Stulle, Use of cryptopp 5.6.0
				/*
				if(result.Encode(aucResult, SIGNATURELENGTH)){
				*/
				result.Encode(aucResult, SIGNATURELENGTH);
				{
				//MORPH END   - Changed by Stulle, Use of cryptopp 5.6.0
					uchar aucHash1[16];
					for (int i = 0; i != 16; i++)
						aucHash1[i] = aucResult[(SIGNATURELENGTH-1)-i]; 
					bSignatureCheckResult = md4cmp(MD5Sum(pachCompleteFile, nIFileSize-SIGNATURELENGTH).GetRawHash(), aucHash1) == 0;
				}
			}
			delete[] pachCompleteFile;
			if (!bSignatureCheckResult)
				DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("PeerCache: Failed to verify PeerCache Server, not using it")));
		}

		delete[] pcContent;
		file->Close();
		delete file;
		return (bSignatureCheckResult && !bContentCheckFailed );
	}
	return false;
}

BEGIN_MESSAGE_MAP(CPCValditeThread, CWinThread)
END_MESSAGE_MAP()



IMPLEMENT_DYNCREATE(CPCReverseDnsThread, CWinThread)
BOOL CPCReverseDnsThread::InitInstance()
{
	ASSERT( m_hwndAsyncResult != NULL );
	ASSERT( m_dwIP != 0 );
	InitThreadLocale();

	// SLUGFILLER: SafeHash
	CReadWriteLock lock(&theApp.m_threadlock);
	if (!lock.ReadLock(0))
		return FALSE;
	// SLUGFILLER: SafeHash

	memset(s_acDNSBuffer, 0, sizeof s_acDNSBuffer);
	CString strHostname = ReverseDnsLookup(m_dwIP);
	UINT uBufLen = 0;
	UINT uError = WSAEINVAL;
	if (!strHostname.IsEmpty())
	{
		CStringA strHostnameA(strHostname);
		uBufLen = sizeof(HOSTENT)		// 'hostent' structure
			+ sizeof(char*)		// h_aliases list + NUL entry
			+ sizeof(DWORD)*2		// h_addr_list + NUL entry
			+ strHostnameA.GetLength() + 1;
		if (uBufLen <= sizeof(s_acDNSBuffer))
		{
			LPHOSTENT pHost = (LPHOSTENT)s_acDNSBuffer;
			char* p = (char*)(pHost + 1);

			pHost->h_aliases = (char**)p;
			pHost->h_aliases[0] = (char*)NULL;
			p += sizeof(char*);

			pHost->h_addrtype = AF_INET;
			pHost->h_length = sizeof(DWORD);

			pHost->h_addr_list = (char**)p;
			pHost->h_addr_list[0] = (char*)m_dwIP;
			pHost->h_addr_list[1] = (char*)NULL;
			p += sizeof(char*)*2;

			pHost->h_name = p;
			strcpy(pHost->h_name, strHostnameA);
			p += strHostnameA.GetLength() + 1;

			ASSERT( (UINT)(p - s_acDNSBuffer) == uBufLen );
			uError = 0;
		}
		else
			uError = WSAENOBUFS;

		VERIFY( PostMessage(m_hwndAsyncResult, UM_PEERCHACHE_RESPONSE, 0, WSAMAKEASYNCREPLY(uBufLen, uError)) );
	}
	else
	{
		in_addr IPHost;
		// FIXME: Unable to resolve my own host - will always get the Windows Computer/Domainname. Dunno how to avoid this
		// cheap walk arround below by using another IP
		IPHost.s_addr = ntohl(ntohl(m_dwIP)+1);
		if (WSAAsyncGetHostByAddr(theApp.emuledlg->m_hWnd, UM_PEERCHACHE_RESPONSE, (const char*) &IPHost, sizeof(struct in_addr), AF_INET, s_acDNSBuffer, sizeof(s_acDNSBuffer)) == 0){
			if (thePrefs.GetVerbose())
				DEBUG_ONLY(theApp.QueueDebugLogLine(false, _T("DNS Reverse Lookup for own IP failed")));
		}	
	}
	return FALSE;
}
