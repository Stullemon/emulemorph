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
#include "fakecheck.h"
#include "emule.h"
#include "otherfunctions.h"
#include "HttpDownloadDlg.h"
#include "emuleDlg.h"
#include "Preferences.h"
#include "ZipFile.h"

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
	FILE* readFile = fopen(thePrefs.GetConfigDir()+_T("fakes.dat"), "r");
	if (readFile!=NULL) {
		CString sbuffer, sbuffer2;
		int pos;
		uint32 Lenght;
		CString Title;
		char buffer[1024];
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
			Lenght=atoi(sbuffer.Mid(pos+1,pos2-pos-1).Trim());
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

		AddLogLine(false, _T("%i Fake Check reference loaded"), m_fakelist.GetCount());
		if (thePrefs.GetVerbose())
		{
			AddDebugLogLine(false, _T("Found Fake Reference:%u  Duplicate:%u  Merged:%u"), fakecounter, iDuplicate, iMerged);
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
	char buffer[5];
	int lenBuf = 5;
	CString sbuffer;
	CString strURL = thePrefs.GetUpdateURLFakeList();
	strURL.TrimRight(".txt");
	strURL.TrimRight(".dat");
	strURL.Append(".txt");
	CString strTempFilename;
	strTempFilename.Format(CString(thePrefs.GetAppDir())+"fakes.txt");
	FILE* readFile= fopen(strTempFilename, "r");
	CHttpDownloadDlg dlgDownload;
	dlgDownload.m_strTitle = _T("Downloading Fake Check version file");
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
	if ((thePrefs.GetFakesDatVersion() < (uint32) atoi(sbuffer))) {
		thePrefs.SetFakesDatVersion(atoi(sbuffer));
		thePrefs.Save(); //MORPH - Added by SiRoB, Fix the continuous update while emule have not been shutdown in case used in an autoupdater
		CString FakeCheckURL = strURL.TrimRight(".txt")+".dat";
		strTempFilename.Format(CString(thePrefs.GetConfigDir())+"fakes.dat");
		if (fopen(strTempFilename, "r")) {
			fclose(readFile);
			remove(strTempFilename);
		}

		TCHAR szTempFilePath[MAX_PATH];
		_tmakepath(szTempFilePath, NULL, thePrefs.GetConfigDir(), DFLT_FAKECHECK_FILENAME, _T("tmp"));

		CHttpDownloadDlg dlgDownload;
		dlgDownload.m_strTitle = _T("Downloading Fake Check file");
		dlgDownload.m_sURLToDownload = FakeCheckURL;
		dlgDownload.m_sFileToDownloadInto = szTempFilePath;
		if (dlgDownload.DoModal() != IDOK)
		{
			_tremove(szTempFilePath);
			AddLogLine(true,GetResString(IDS_FAKECHECKUPERROR));
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
				_tmakepath(szTempUnzipFilePath, NULL, thePrefs.GetConfigDir(), DFLT_FAKECHECK_FILENAME, _T(".unzip.tmp"));
				if (zfile->Extract(szTempUnzipFilePath))
				{
					zip.Close();
					zfile = NULL;

					if (_tremove(GetDefaultFilePath()) != 0)
						TRACE("*** Error: Failed to remove default IP filter file \"%s\" - %s\n", GetDefaultFilePath(), strerror(errno));
					if (_trename(szTempUnzipFilePath, GetDefaultFilePath()) != 0)
						TRACE("*** Error: Failed to rename uncompressed IP filter file \"%s\" to default IP filter file \"%s\" - %s\n", szTempUnzipFilePath, GetDefaultFilePath(), strerror(errno));
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
			_tremove(GetDefaultFilePath());
			_trename(szTempFilePath, GetDefaultFilePath());
		}

		LoadFromFile();
	}
}
CString CFakecheck::GetDefaultFilePath() const
{
	return thePrefs.GetConfigDir() + DFLT_FAKECHECK_FILENAME;
}