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
#include "stdafx.h"
#include <share.h>
#include "emule.h"
#include "IPFilter.h"
#include "otherfunctions.h"
#include "Preferences.h"
#include "emuleDlg.h"
#include "HttpDownloadDlg.h"//MORPH START added by Yun.SF3: Ipfilter.dat update
#include "ZipFile.h"//MORPH - Added by SiRoB, ZIP File download decompress
#include "IP2Country.h"//Commander

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define	DFLT_FILTER_LEVEL	100 // default filter level if non specified

CIPFilter::CIPFilter()
{
	m_pLastHit = NULL;
	LoadFromDefaultFile(false);
}

CIPFilter::~CIPFilter()
{
	RemoveAllIPFilters();
}

//MORPH - Added by SiRoB
void CIPFilter::AddIP(uint32 IP, UINT level, const CString& desc)
{
	uint32 ip1;
	((BYTE*)&ip1)[0] = ((BYTE*)&IP)[3];
	((BYTE*)&ip1)[1] = ((BYTE*)&IP)[2];
	((BYTE*)&ip1)[2] = ((BYTE*)&IP)[1];
	((BYTE*)&ip1)[3] = ((BYTE*)&IP)[0];
	AddIPRange(ip1, ip1, level, desc);
}
//MORPH - Added by SiRoB

void CIPFilter::AddIPRange(uint32 start, uint32 end, UINT level, const CString& desc)
{
	SIPFilter* newFilter = new SIPFilter;
	newFilter->start = start;
	newFilter->end = end;
	newFilter->level = level;
	newFilter->desc = desc;
	newFilter->hits = 0;
	m_iplist.Add(newFilter);
}

static int __cdecl CmpSIPFilterByStartAddr(const void* p1, const void* p2)
{
	const SIPFilter* rng1 = *(SIPFilter**)p1;
	const SIPFilter* rng2 = *(SIPFilter**)p2;
	return CompareUnsigned(rng1->start, rng2->start);
}

CString CIPFilter::GetDefaultFilePath() const
{
	return thePrefs.GetConfigDir() + DFLT_IPFILTER_FILENAME;
}

int CIPFilter::LoadFromDefaultFile(bool bShowResponse)
{
	RemoveAllIPFilters();
	return AddFromFile(GetDefaultFilePath(), bShowResponse);
}
int CIPFilter::AddFromFile(LPCTSTR pszFilePath, bool bShowResponse)
{
	FILE* readFile = _tfsopen(pszFilePath, _T("r"), _SH_DENYWR);
	if (readFile != NULL)
	{
		enum EIPFilterFileType
		{
			Unknown = 0,
			FilterDat = 1,		// ipfilter.dat/ip.prefix format
			PeerGuardian = 2	// PeerGuardian format
		} eFileType = Unknown;

		TCHAR szNam[_MAX_FNAME];
		TCHAR szExt[_MAX_EXT];
		_tsplitpath(pszFilePath, NULL, NULL, szNam, szExt);
		if (_tcsicmp(szExt, _T(".p2p")) == 0 || (_tcsicmp(szNam, _T("guarding.p2p")) == 0 && _tcsicmp(szExt, _T(".txt")) == 0))
			eFileType = PeerGuardian;
		else if (_tcsicmp(szExt, _T(".prefix")) == 0)
			eFileType = FilterDat;

		int iLine = 0;
		int iFoundRanges = 0;
		int iDuplicate = 0;
		int iMerged = 0;
		CString sbuffer;
		TCHAR szBuffer[1024];
		while (_fgetts(szBuffer, ARRSIZE(szBuffer), readFile) != NULL)
		{
			iLine++;
			sbuffer = szBuffer;
			
			// ignore comments & too short lines
			if (sbuffer.GetAt(0) == _T('#') || sbuffer.GetAt(0) == _T('/') || sbuffer.GetLength() < 5)
				continue;

			if (eFileType == Unknown)
			{
				// looks like html
				if (sbuffer.Find(_T('>')) > -1 && sbuffer.Find(_T('<')) > -1)
					sbuffer.Delete(0, sbuffer.ReverseFind(_T('>')) + 1);

				// check for <IP> - <IP> at start of line
				UINT u1, u2, u3, u4, u5, u6, u7, u8;
				if (_stscanf(sbuffer, _T("%u.%u.%u.%u - %u.%u.%u.%u"), &u1, &u2, &u3, &u4, &u5, &u6, &u7, &u8) == 8)
				{
					eFileType = FilterDat;
				}
				else
				{
					// check for <description> ':' <IP> '-' <IP>
					int iColon = sbuffer.Find(_T(':'));
					if (iColon > -1)
					{
						CString strIPRange = sbuffer.Mid(iColon + 1);
						UINT u1, u2, u3, u4, u5, u6, u7, u8;
						if (_stscanf(strIPRange, _T("%u.%u.%u.%u - %u.%u.%u.%u"), &u1, &u2, &u3, &u4, &u5, &u6, &u7, &u8) == 8)
						{
							eFileType = PeerGuardian;
						}
					}
				}
			}

			bool bValid = false;
			uint32 start, end;
			UINT level;
			CString desc;
			if (eFileType == FilterDat)
				bValid = ParseFilterLine1(sbuffer, start, end, level, desc);
			else if (eFileType == PeerGuardian)
				bValid = ParseFilterLine2(sbuffer, start, end, level, desc);

			// add a filter
			if (bValid)
			{
				AddIPRange(start, end, level, desc);
				iFoundRanges++;
			}
		}
		fclose(readFile);

		// sort the IP filter list by IP range start addresses
		qsort(m_iplist.GetData(), m_iplist.GetCount(), sizeof(m_iplist[0]), CmpSIPFilterByStartAddr);

		// merge overlapping and adjacent filter ranges
		if (m_iplist.GetCount() >= 2)
		{
			SIPFilter* pPrv = m_iplist[0];
			int i = 1;
			while (i < m_iplist.GetCount())
			{
				SIPFilter* pCur = m_iplist[i];
				if (   pCur->start >= pPrv->start && pCur->start <= pPrv->end	 // overlapping
					|| pCur->start == pPrv->end+1 && pCur->level == pPrv->level) // adjacent
				{
					if (pCur->start != pPrv->start || pCur->end != pPrv->end) // don't merge identical entries
					{
						//TODO: not yet handled, overlapping entries with different 'level'
						if (pCur->end > pPrv->end)
							pPrv->end = pCur->end;
						//pPrv->desc += _T("; ") + pCur->desc; // this may create a very very long description string...
						iMerged++;
					}
					else
					{
						// if we have identical entries, use the lowest 'level'
						if (pCur->level < pPrv->level)
							pPrv->level = pCur->level;
						iDuplicate++;
					}
					delete pCur;
					m_iplist.RemoveAt(i);
					continue;
				}
				pPrv = pCur;
				i++;
			}
		}

		theApp.emuledlg->AddLogLine(bShowResponse, GetResString(IDS_IPFILTERLOADED), m_iplist.GetCount());
		if (thePrefs.GetVerbose())
		{
			theApp.emuledlg->AddDebugLogLine(false, _T("Loaded IP filters from \"%s\""), pszFilePath);
			theApp.emuledlg->AddDebugLogLine(false, _T("Parsed lines:%u  Found IP ranges:%u  Duplicate:%u  Merged:%u"), iLine, iFoundRanges, iDuplicate, iMerged);
		}
	}
	return m_iplist.GetCount();
}

void CIPFilter::SaveToDefaultFile()
{
	CString strFilePath = thePrefs.GetConfigDir() + DFLT_IPFILTER_FILENAME;
	FILE* fp = _tfsopen(strFilePath, _T("wt"), _SH_DENYWR);
	if (fp != NULL)
	{
		for (int i = 0; i < m_iplist.GetCount(); i++)
		{
			const SIPFilter* flt = m_iplist[i];

			TCHAR szStart[16];
			_tcscpy(szStart, ipstr(htonl(flt->start)));

			TCHAR szEnd[16];
			_tcscpy(szEnd, ipstr(htonl(flt->end)));

			if (_ftprintf(fp, _T("%-15s - %-15s , %3u , %s\n"), szStart, szEnd, flt->level, flt->desc) == 0 || ferror(fp))
			{
				CString strError;
				strError.Format(_T("Failed to save IP filter to file \"%s\" - %hs"), strFilePath, strerror(errno));
				throw strError;
			}
		}
		fclose(fp);
	}
	else
	{
		CString strError;
		strError.Format(_T("Failed to save IP filter to file \"%s\" - %hs"), strFilePath, strerror(errno));
		throw strError;
	}
}

bool CIPFilter::ParseFilterLine1(const CString& sbuffer, uint32& ip1, uint32& ip2, UINT& level, CString& desc) const
{
	TCHAR szDesc[256];
	memset(szDesc, 0, sizeof szDesc);
	UINT u1, u2, u3, u4, u5, u6, u7, u8, uLevel = DFLT_FILTER_LEVEL;
	int iItems = _stscanf(sbuffer, _T("%u.%u.%u.%u - %u.%u.%u.%u , %u , %255c"), &u1, &u2, &u3, &u4, &u5, &u6, &u7, &u8, &uLevel, szDesc);
	if (iItems < 8)
		return false;

	((BYTE*)&ip1)[0] = u4;
	((BYTE*)&ip1)[1] = u3;
	((BYTE*)&ip1)[2] = u2;
	((BYTE*)&ip1)[3] = u1;

	((BYTE*)&ip2)[0] = u8;
	((BYTE*)&ip2)[1] = u7;
	((BYTE*)&ip2)[2] = u6;
	((BYTE*)&ip2)[3] = u5;

	if (iItems == 8)
	{
		level = DFLT_FILTER_LEVEL;	// set default level
		return true;
	}

	level = uLevel;

	if (iItems == 9)
		return true;	 // no description available

	ASSERT( iItems == 10 );
	desc = szDesc;
	desc.Trim();

	return true;
}

bool CIPFilter::ParseFilterLine2(const CString& sbuffer, uint32& ip1, uint32& ip2, UINT& level, CString& desc) const
{
	int iPos = sbuffer.ReverseFind(_T(':'));
	if (iPos < 0)
		return false;

	desc = sbuffer.Left(iPos);
	desc.Replace(_T("PGIPDB"), _T(""));
	desc.Trim();

	CString strIPRange = sbuffer.Mid(iPos + 1, sbuffer.GetLength() - iPos);
	UINT u1, u2, u3, u4, u5, u6, u7, u8;
	if (_stscanf(strIPRange, _T("%u.%u.%u.%u - %u.%u.%u.%u"), &u1, &u2, &u3, &u4, &u5, &u6, &u7, &u8) != 8)
		return false;

	((BYTE*)&ip1)[0] = u4;
	((BYTE*)&ip1)[1] = u3;
	((BYTE*)&ip1)[2] = u2;
	((BYTE*)&ip1)[3] = u1;

	((BYTE*)&ip2)[0] = u8;
	((BYTE*)&ip2)[1] = u7;
	((BYTE*)&ip2)[2] = u6;
	((BYTE*)&ip2)[3] = u5;

	level = DFLT_FILTER_LEVEL;

	return true;
}

void CIPFilter::RemoveAllIPFilters()
{
	for (int i = 0; i < m_iplist.GetCount(); i++)
		delete m_iplist[i];
	m_iplist.RemoveAll();
	m_pLastHit = NULL;
}

bool CIPFilter::IsFiltered(uint32 ip) /*const*/
{
	return IsFiltered(ip, thePrefs.GetIPFilterLevel());
}

static int __cdecl CmpSIPFilterByAddr(const void* pvKey, const void* pvElement)
{
	uint32 ip = *(uint32*)pvKey;
	const SIPFilter* pIPFilter = *(SIPFilter**)pvElement;

	if (ip < pIPFilter->start)
		return -1;
	if (ip > pIPFilter->end)
		return 1;
	return 0;
}

bool CIPFilter::IsFiltered(uint32 ip, UINT level) /*const*/
{
	if (m_iplist.GetCount() == 0 || ip == 0)
		return false;
	
	ip = htonl(ip);
	/*for (int i = 0; i < m_iplist.GetCount(); i++)
	{
		SIPFilter* search = m_iplist[i];
		if (ip < search->start)
			return false;
		if (search->level < level && ip >= search->start && ip <= search->end)
		{
			search->hits++;
			m_pLastHit = search;
			return true;
		}
	}*/

	// to speed things up we use a binary search
	//	*)	the IP filter list must be sorted by IP range start addresses
	//	*)	the IP filter list is not allowed to contain overlapping IP ranges (see also the IP range merging code when
	//		loading the list)
	//	*)	the filter 'level' is ignored during the binary search and is evaluated only for the found element
	//
	// TODO: this can still be improved even more:
	//	*)	use a pre assembled list of IP ranges which contains only the IP ranges for the currently used filter level
	//	*)	use a dumb plain array for storing the IP range structures. this will give more cach hits when processing
	//		the list. but(!) this would require to also use a dumb SIPFilter structure (don't use data items with ctors).
	//		otherwise the creation of the array would be rather slow.
	SIPFilter** ppFound = (SIPFilter**)bsearch(&ip, m_iplist.GetData(), m_iplist.GetCount(), sizeof(m_iplist[0]), CmpSIPFilterByAddr);
	if (ppFound && (*ppFound)->level < level)
	{
		(*ppFound)->hits++;
		m_pLastHit = *ppFound;
		return true;
	}

	return false;
}

LPCTSTR CIPFilter::GetLastHit() const
{
	return m_pLastHit ? m_pLastHit->desc : _T("Not available");
}

const CIPFilterArray& CIPFilter::GetIPFilter() const
{
	return m_iplist;
}

bool CIPFilter::RemoveIPFilter(const SIPFilter* pFilter)
{
	for (int i = 0; i < m_iplist.GetCount(); i++)
	{
		if (m_iplist[i] == pFilter)
		{
			delete m_iplist[i];
			m_iplist.RemoveAt(i);
			return true;
		}
	}
	return false;
}

//MORPH START added by Yun.SF3: Ipfilter.dat update
void CIPFilter::UpdateIPFilterURL()
{
	char buffer[5];
	int lenBuf = 5;
	CString sbuffer;
	CString strURL = thePrefs.GetUpdateURLIPFilter();
	strURL.TrimRight(".txt");
	strURL.TrimRight(".dat");
	strURL.Append(".txt");
	CString strTempFilename;
	strTempFilename.Format(CString(thePrefs.GetAppDir())+"ipfilter.txt");
	FILE* readFile= fopen(strTempFilename, "r");
	CHttpDownloadDlg dlgDownload;
	dlgDownload.m_strTitle = _T("Downloading IP filter version file");
	dlgDownload.m_sURLToDownload = strURL;
	dlgDownload.m_sFileToDownloadInto = strTempFilename;
	if (dlgDownload.DoModal() != IDOK)
	{
		AddLogLine(true, "Error downloading %s", strURL);
		return;
	}
	readFile= fopen(strTempFilename, "r");
	fgets(buffer,lenBuf,readFile);
		//return false;
	sbuffer = buffer;
	sbuffer = sbuffer.Trim();
	fclose(readFile);
	remove(strTempFilename);
	// Mighty Knife: cleanup - removed that nasty signed-unsigned-message
	if ((thePrefs.GetIPfilterVersion()< (uint32) atoi(sbuffer)) || (readFile == NULL)) {
	// [end] Mighty Knife

		CString IPFilterURL = strURL.TrimRight(".txt") + ".dat";
		strTempFilename.Format(CString(thePrefs.GetConfigDir())+"ipfilter.dat");

		if (fopen(strTempFilename, "r")) {
			fclose(readFile);
			remove(strTempFilename);
		}

		TCHAR szTempFilePath[MAX_PATH];
		_tmakepath(szTempFilePath, NULL, thePrefs.GetConfigDir(), DFLT_IPFILTER_FILENAME, _T("tmp"));

		CHttpDownloadDlg dlgDownload;
		dlgDownload.m_strTitle = _T("Downloading IP filter file");
		dlgDownload.m_sURLToDownload = IPFilterURL;
		dlgDownload.m_sFileToDownloadInto = szTempFilePath;
		if (dlgDownload.DoModal() != IDOK)
		{
			_tremove(szTempFilePath);
			AddLogLine(true, _T("IP Filter download failed"));
			return;
		}

		bool bIsZipFile = false;
		bool bUnzipped = false;
		CZIPFile zip;
		if (zip.Open(szTempFilePath))
		{
			bIsZipFile = true;

			CZIPFile::File* zfile = zip.GetFile(_T("guarding.p2p"));
			if (zfile)
			{
				TCHAR szTempUnzipFilePath[MAX_PATH];
				_tmakepath(szTempUnzipFilePath, NULL, thePrefs.GetConfigDir(), DFLT_IPFILTER_FILENAME, _T(".unzip.tmp"));
				if (zfile->Extract(szTempUnzipFilePath))
				{
					zip.Close();
					zfile = NULL;

					if (_tremove(theApp.ipfilter->GetDefaultFilePath()) != 0)
						TRACE("*** Error: Failed to remove default IP filter file \"%s\" - %s\n", theApp.ipfilter->GetDefaultFilePath(), strerror(errno));
					if (_trename(szTempUnzipFilePath, theApp.ipfilter->GetDefaultFilePath()) != 0)
						TRACE("*** Error: Failed to rename uncompressed IP filter file \"%s\" to default IP filter file \"%s\" - %s\n", szTempUnzipFilePath, theApp.ipfilter->GetDefaultFilePath(), strerror(errno));
					if (_tremove(szTempFilePath) != 0)
						TRACE("*** Error: Failed to remove temporary IP filter file \"%s\" - %s\n", szTempFilePath, strerror(errno));
					bUnzipped = true;
				}
				else
					AddLogLine(true, _T("Failed to extract IP filter file from downloaded IP filter ZIP file \"%s\"."), szTempFilePath);
			}
			else
				AddLogLine(true, _T("Downloaded IP filter file \"%s\" is a ZIP file with unexpected content."), szTempFilePath);

			zip.Close();
		}

		if (!bIsZipFile && !bUnzipped)
		{
			_tremove(theApp.ipfilter->GetDefaultFilePath());
			_trename(szTempFilePath, theApp.ipfilter->GetDefaultFilePath());
		}
        //Commander - Fixed: Versionnumber on failed download - Start
		if(bIsZipFile && !bUnzipped){
			return;
		}
			LoadFromDefaultFile();
            
			thePrefs.SetIpfilterVersion(atoi(sbuffer));
			thePrefs.Save();
		//Commander - Fixed: Versionnumber on failed download - End
	}
}
//MORPH END added by Yun.SF3: Ipfilter.dat update

//Commander - Added: IP2Country auto-updating - Start
void CIPFilter::UpdateIP2CountryURL()
{   
	char buffer[9]; //Versionformat: Ymmdd -> 20040101
	int lenBuf = 9;
	CString sbuffer;
	CString strVerURL = thePrefs.GetUpdateVerURLIP2Country(); //Version URL to keep it separated
	CString strURL = thePrefs.GetUpdateURLIP2Country(); // File URL
	CString strTempFilename;
	strTempFilename.Format(CString(thePrefs.GetAppDir())+"ip-to-country.txt");
	FILE* readFile= fopen(strTempFilename, "r");
	CHttpDownloadDlg dlgDownload;
	dlgDownload.m_strTitle = _T("Downloading IP2Country version file");
	dlgDownload.m_sURLToDownload = strVerURL;
	dlgDownload.m_sFileToDownloadInto = strTempFilename;
	if (dlgDownload.DoModal() != IDOK)
	{
		AddLogLine(true, "Error downloading %s", strVerURL);
		return;
	}
	readFile= fopen(strTempFilename, "r");
	fgets(buffer,lenBuf,readFile);
	sbuffer = buffer;
	sbuffer = sbuffer.Trim();
	fclose(readFile);
	remove(strTempFilename);

    // Compare the Version numbers
	if ((thePrefs.GetIP2CountryVersion()< (uint32) atoi(sbuffer)) || (readFile == NULL)) {
		
		if(thePrefs.GetIP2CountryNameMode() != IP2CountryName_DISABLE || thePrefs.IsIP2CountryShowFlag()){
			theApp.ip2country->Unload();
			AddLogLine(false,"IP2Country.csv unloaded due to update in progress");
		}
		CString IP2CountryURL = strURL;
		CString ext;
		
		ext = strURL;
		ext.TrimRight("."); //Trim the file URL in order to save its extension in ext

		strTempFilename.Format(CString(thePrefs.GetConfigDir())+"ip-to-country."+"ext"); //create a file with the original extension

		if (fopen(strTempFilename, "r")) {
			fclose(readFile);
			remove(strTempFilename);
		}

		TCHAR szTempFilePath[MAX_PATH];
		_tmakepath(szTempFilePath, NULL, thePrefs.GetConfigDir(), DFLT_IP2COUNTRY_FILENAME, _T("tmp"));

		CHttpDownloadDlg dlgDownload;
		dlgDownload.m_strTitle = _T("Downloading IP2Country file");
		dlgDownload.m_sURLToDownload = IP2CountryURL;
		dlgDownload.m_sFileToDownloadInto = szTempFilePath;
		if (dlgDownload.DoModal() != IDOK)
		{
			_tremove(szTempFilePath);
			AddLogLine(true, _T("IP2Country file download failed"));
			if(thePrefs.GetIP2CountryNameMode() != IP2CountryName_DISABLE || thePrefs.IsIP2CountryShowFlag()){
				theApp.ip2country->Load();
				AddLogLine(false,"IP2Country.csv loaded after unsuccessful update (backup file loaded)");
			}
			return;
		}
        
		bool bIsZipFile = false;
		bool bUnzipped = false;
		CZIPFile zip;
		if (zip.Open(szTempFilePath))
		{
			bIsZipFile = true;

			CZIPFile::File* zfile = zip.GetFile(_T("ip-to-country.csv")); // It has to be a zip-file which includes a file called: ip-to-country.csv
			if (zfile)
			{
				TCHAR szTempUnzipFilePath[MAX_PATH];
				_tmakepath(szTempUnzipFilePath, NULL, thePrefs.GetConfigDir(), DFLT_IP2COUNTRY_FILENAME, _T(".unzip.tmp"));
				TCHAR szTempCurrentFilePath[MAX_PATH];
				_tmakepath(szTempCurrentFilePath, NULL, thePrefs.GetConfigDir(), DFLT_IP2COUNTRY_FILENAME, _T(""));

				if (zfile->Extract(szTempUnzipFilePath))
				{
					zip.Close();
					zfile = NULL;
                    //Successfully unziped, rename the unzipped temp file to its destination name and remove the zipped file
					bUnzipped = true;
                    if(PathFileExists(thePrefs.GetConfigDir()+_T("ip-to-country.csv")))
						_tremove(szTempCurrentFilePath);
					_trename(szTempUnzipFilePath, thePrefs.GetConfigDir()+_T("ip-to-country.csv"));
					_tremove(szTempFilePath);
				}
				else
					AddLogLine(true, _T("Failed to extract IP filter file from downloaded IP filter ZIP file \"%s\"."), szTempFilePath);
			}
			else
				AddLogLine(true, _T("Downloaded IP filter file \"%s\" is a ZIP file with unexpected content."), szTempFilePath); //File not found inside the zip-file

			zip.Close();
		}
		else 
			_trename(szTempFilePath, thePrefs.GetConfigDir()+_T("ip-to-country.csv")); //If its not a zipfile, rename it to its destination name
		
        
		if(bIsZipFile && bUnzipped == false){ //Is Zipfile and failed to unzip
			return;
		}

        //load the new one
		if(thePrefs.GetIP2CountryNameMode() != IP2CountryName_DISABLE || thePrefs.IsIP2CountryShowFlag()){
		  theApp.ip2country->Load();
		  AddLogLine(false,"IP2Country.csv loaded after successful update");
		}

		thePrefs.SetIP2CountryVersion(atoi(sbuffer)); //Commander - Added: Update version number
		thePrefs.Save();
	}
}
//Commander - Added: IP2Country auto-updating - End