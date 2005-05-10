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

#include "StdAfx.h"
#include <share.h>
#include "fakecheck.h"
#include "emule.h"
#include "otherfunctions.h"
#include "HttpDownloadDlg.h"
#include "emuleDlg.h"
#include "Preferences.h"
#include "ZipFile.h"
#include "log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

CFakecheck::CFakecheck(){
	m_pLastHit = NULL;
	LoadFromFile();
}

CFakecheck::~CFakecheck(){
	RemoveAllFakes();
}

void CFakecheck::AddFake(uchar* Hash,uint32& Lenght,CString& Realtitle){
	Fakes_Struct* newFilter=new Fakes_Struct;
	md4cpy(newFilter->Hash, Hash);
	newFilter->Lenght=Lenght;
	newFilter->RealTitle=Realtitle;
	m_fakelist.Add(newFilter);
}

static int __cdecl CmpFakeByHash_Lenght(const void* p1, const void* p2)
{
	const Fakes_Struct* pFake1 = *(Fakes_Struct**)p1;
	const Fakes_Struct* pFake2 = *(Fakes_Struct**)p2;
	int diff = memcmp(pFake1->Hash, pFake2->Hash, 16);
	if (diff)
		return diff;
	return pFake1->Lenght-pFake2->Lenght;
}

int CFakecheck::LoadFromFile(){
	DWORD startMesure = GetTickCount();
	CString strfakecheckfile = GetDefaultFilePath();
	FILE* readFile = _tfsopen(strfakecheckfile, _T("r"), _SH_DENYWR);
	if (readFile!=NULL) {
		CString sbuffer, sbuffer2;
		int pos;
		uint32 Lenght;
		CString Title;
		char buffer[512];
		int fakecounter = 0;
		int iDuplicate = 0;
		int iMerged = 0;
		RemoveAllFakes();
		while (fgets(buffer, ARRSIZE(buffer), readFile) != NULL)
		{
			
			sbuffer=buffer;
			if (sbuffer.GetAt(0) == _T('#') || sbuffer.GetAt(0) == _T('/') || sbuffer.GetLength() < 5)
				continue;
			pos=sbuffer.Find(_T(','));
			if (pos==-1) continue;
			sbuffer2=sbuffer.Left(pos).Trim();
			uchar Hash[16];
			DecodeBase16(sbuffer2.GetBuffer(),sbuffer2.GetLength(),Hash,ARRSIZE(Hash));
			int pos2=sbuffer.Find(_T(","),pos+1);
			if (pos2==-1) continue;
			Lenght=_tstoi(sbuffer.Mid(pos+1,pos2-pos-1).Trim());
			Title=sbuffer.Mid(pos2+1,sbuffer.GetLength()-pos2-2);
			AddFake(&Hash[0],Lenght,Title);
			++fakecounter;
		}
		fclose(readFile);
		// sort the FakeCheck entry by Hash 
		qsort(m_fakelist.GetData(), m_fakelist.GetCount(), sizeof(m_fakelist[0]), CmpFakeByHash_Lenght);

		// merge overlapping and adjacent filter ranges
		if (m_fakelist.GetCount() >= 2)
		{
			Fakes_Struct* pPrv = m_fakelist[0];
			int i = 1;
			while (i < m_fakelist.GetCount())
			{
				Fakes_Struct* pCur = m_fakelist[i];
				if ( pCur->Hash == pPrv->Hash && pCur->Lenght == pPrv->Lenght)
				{
					if (pCur->RealTitle != pPrv->RealTitle)
					{
						//pPrv->RealTitle += _T("; ") + pCur->RealTitle;
						iMerged++;
					}
					else
					{
						iDuplicate++;
					}
					delete pCur;
					m_fakelist.RemoveAt(i);
					continue;
				}
				pPrv = pCur;
				++i;
			}
		}

		AddLogLine(false, GetResString(IDS_FC_LOADED), m_fakelist.GetCount());
		if (thePrefs.GetVerbose())
		{
			AddDebugLogLine(false, GetResString(IDS_LOG_FC_LOADED), strfakecheckfile, GetTickCount()-startMesure);
			AddDebugLogLine(false, GetResString(IDS_LOG_FC_INFO), fakecounter, iDuplicate, iMerged);
		}
	}
	return m_fakelist.GetCount();
}

bool CFakecheck::IsFake(uchar* Hash2test, uint32 lenght){
	if (m_fakelist.GetCount() == 0)
		return false;
	Fakes_Struct** ppFound = (Fakes_Struct**)bsearch(&Hash2test, m_fakelist.GetData(), m_fakelist.GetCount(), sizeof(m_fakelist[0]), CmpFakeByHash_Lenght);
	if (ppFound)
	{
		m_pLastHit = *ppFound;
		return true;
	}

	return false;
}
CString CFakecheck::GetLastHit() const
{
	return m_pLastHit ? m_pLastHit->RealTitle : _T("");
}

void CFakecheck::RemoveAllFakes()
{
	for (int i = 0; i < m_fakelist.GetCount(); i++)
		delete m_fakelist[i];
	m_fakelist.RemoveAll();
	m_pLastHit = NULL;
}

void CFakecheck::DownloadFakeList()
{
	CString sbuffer;
	CString strURL = thePrefs.GetUpdateURLFakeList();
	strURL.TrimRight(_T(".txt"));
	strURL.TrimRight(_T(".dat"));
	strURL.TrimRight(_T(".zip"));
	strURL.Append(_T(".txt"));

	TCHAR szTempFilePath[_MAX_PATH];
	_tmakepath(szTempFilePath, NULL, thePrefs.GetAppDir(), DFLT_FAKECHECK_FILENAME, _T("tmp"));
	_tremove(szTempFilePath);

	CHttpDownloadDlg dlgDownload;
	dlgDownload.m_strTitle = GetResString(IDS_DOWNFAKECHECKVER);
	dlgDownload.m_sURLToDownload = strURL;
	dlgDownload.m_sFileToDownloadInto = szTempFilePath;

	if (dlgDownload.DoModal() != IDOK)
	{
		_tremove(szTempFilePath);
		LogError(LOG_STATUSBAR, GetResString(IDS_LOG_ERRDWN), strURL);
		return;
	}
	FILE* readFile = _tfsopen(szTempFilePath, _T("r"), _SH_DENYWR);

	char buffer[9];
	int lenBuf = 9;
	fgets(buffer,lenBuf,readFile);
	sbuffer = buffer;
	sbuffer = sbuffer.Trim();
	fclose(readFile);
	_tremove(szTempFilePath);

	if ((thePrefs.GetFakesDatVersion() < (uint32) _tstoi(sbuffer)) || !PathFileExists(GetDefaultFilePath()) || FileSize(GetDefaultFilePath()) < 10240) {
		
		CString FakeCheckURL = thePrefs.GetUpdateURLFakeList();

		_tmakepath(szTempFilePath, NULL, thePrefs.GetConfigDir(), DFLT_FAKECHECK_FILENAME, _T("tmp"));
		_tremove(szTempFilePath);

		CHttpDownloadDlg dlgDownload;
		dlgDownload.m_strTitle = GetResString(IDS_DOWNFAKECHECKFILE);
		dlgDownload.m_sURLToDownload = FakeCheckURL;
		dlgDownload.m_sFileToDownloadInto = szTempFilePath;
		if (dlgDownload.DoModal() != IDOK || FileSize(szTempFilePath) < 10240)
		{
			_tremove(szTempFilePath);
			LogError(LOG_STATUSBAR, GetResString(IDS_FAKECHECKUPERROR));
			return;
		}

		bool bIsZipFile = false;
		bool bUnzipped = false;
		CZIPFile zip;
		if (zip.Open(szTempFilePath))
		{
			bIsZipFile = true;

			CZIPFile::File* zfile = zip.GetFile(DFLT_FAKECHECK_FILENAME);
			if (zfile)
			{
				TCHAR szTempUnzipFilePath[MAX_PATH];
				_tmakepath(szTempUnzipFilePath, NULL, thePrefs.GetConfigDir(), DFLT_FAKECHECK_FILENAME, _T(".unzip.tmp"));
				if (zfile->Extract(szTempUnzipFilePath))
				{
					zip.Close();
					zfile = NULL;

					if (_tremove(GetDefaultFilePath()) != 0)
						TRACE("*** Error: Failed to remove default fake check file \"%s\" - %s\n", GetDefaultFilePath(), _tcserror(errno));
					if (_trename(szTempUnzipFilePath, GetDefaultFilePath()) != 0)
						TRACE("*** Error: Failed to rename uncompressed fake check file \"%s\" to default fake check file \"%s\" - %s\n", szTempUnzipFilePath, GetDefaultFilePath(), _tcserror(errno));
					if (_tremove(szTempFilePath) != 0)
						TRACE("*** Error: Failed to remove temporary fake check file \"%s\" - %s\n", szTempFilePath, _tcserror(errno));
					bUnzipped = true;
				}
				else
					LogError(LOG_STATUSBAR, GetResString(IDS_LOG_FC_ERR1), szTempFilePath);
			}
			else
				LogError(LOG_STATUSBAR, GetResString(IDS_LOG_FC_ERR2), szTempFilePath);

			zip.Close();
		}

		if (!bIsZipFile && !bUnzipped)
		{
			_tremove(GetDefaultFilePath());
			_trename(szTempFilePath, GetDefaultFilePath());
		}

		if(bIsZipFile && !bUnzipped){
			return;
		}

		LoadFromFile();

		thePrefs.SetFakesDatVersion(_tstoi(sbuffer));
		thePrefs.Save();
	}
}
CString CFakecheck::GetDefaultFilePath() const
{
	return thePrefs.GetConfigDir() + DFLT_FAKECHECK_FILENAME;
}