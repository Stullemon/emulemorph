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
#include <io.h>
#include "emule.h"
#include "PerfLog.h"
#include "ini2.h"
#include "Opcodes.h"
#include "Preferences.h"
#include "DownloadQueue.h"
#include "UploadQueue.h"
#ifndef _CONSOLE
#include "emuledlg.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CPerfLog thePerfLog;

CPerfLog::CPerfLog()
{
	m_eMode = None;
	m_dwInterval = MIN2MS(5);
	m_bInitialized = false;
	m_dwLastSampled = 0;
	m_nLastSessionSentBytes = 0;
	m_nLastSessionRecvBytes = 0;
	m_nLastDnOH = 0;
	m_nLastUpOH = 0;
}

void CPerfLog::Startup()
{
	if (m_bInitialized)
		return;

	// set default log file path
	TCHAR szAppPath[MAX_PATH];
	GetModuleFileName(NULL, szAppPath, MAX_PATH);
	PathRemoveFileSpec(szAppPath);
	CString strDefFilePath = szAppPath;
	strDefFilePath += _T("\\perflog.csv");

	CString strIniFile;
	strIniFile.Format(_T("%spreferences.ini"), theApp.glob_prefs->GetConfigDir());
	CIni ini(strIniFile, _T("PerfLog"));

	m_eMode = (ELogMode)ini.GetInt(_T("Mode"), None);
	if (m_eMode != None && m_eMode != OneSample)
		m_eMode = None;

	m_dwInterval = MIN2MS(ini.GetInt(_T("Interval"), 5));
	if ((int)m_dwInterval <= 0)
		m_dwInterval = MIN2MS(5);

	m_strFilePath = ini.GetString(_T("File"), strDefFilePath);
	if (m_strFilePath.IsEmpty())
		m_strFilePath = strDefFilePath;

	m_bInitialized = true;

	if (m_eMode == OneSample)
		LogSamples();
}

void CPerfLog::WriteSamples(UINT nCurDn, UINT nCurUp, UINT nCurDnOH, UINT nCurUpOH)
{
	ASSERT( m_bInitialized );

	time_t tNow = time(NULL);
	char szTime[40];
	// do not localize this date/time string!
	strftime(szTime, ARRSIZE(szTime), "%m/%d/%Y %H:%M:%S", localtime(&tNow));

	FILE* fp = fopen(m_strFilePath, "wt");
	if (fp == NULL){
		theApp.emuledlg->AddLogLine(false, _T("Failed to open performance log file \"%s\" - %hs"), m_strFilePath, strerror(errno));
		return;
	}
	setvbuf(fp, NULL, _IOFBF, 16384); // ensure that all lines are written to file with one call
	fprintf(fp, "\"(PDH-CSV 4.0)\",\"DatDown\",\"DatUp\",\"OvrDown\",\"OvrUp\"\n");
	fprintf(fp, "\"%s\",\"%u\",\"%u\",\"%u\",\"%u\"\n", szTime, nCurDn, nCurUp, nCurDnOH, nCurUpOH);
	fclose(fp);
}

void CPerfLog::LogSamples()
{
	if (m_eMode == None)
		return;

	DWORD dwNow = GetTickCount();
	if (dwNow - m_dwLastSampled < m_dwInterval)
		return;

	// 'data counters' amount of transfered file data
	UINT nCurDn = theApp.stat_sessionReceivedBytes - m_nLastSessionRecvBytes;
	UINT nCurUp = theApp.stat_sessionSentBytes - m_nLastSessionSentBytes;

	// 'overhead counters' amount of total overhead
	uint64 nDnOHTotal = theApp.downloadqueue->GetDownDataOverheadFileRequest() + theApp.downloadqueue->GetDownDataOverheadSourceExchange() + theApp.downloadqueue->GetDownDataOverheadServer() + theApp.downloadqueue->GetDownDataOverheadOther();
	uint64 nUpOHTotal = theApp.uploadqueue->GetUpDataOverheadFileRequest() + theApp.uploadqueue->GetUpDataOverheadSourceExchange() + theApp.uploadqueue->GetUpDataOverheadServer() + theApp.uploadqueue->GetUpDataOverheadOther();
	UINT nCurDnOH = nDnOHTotal - m_nLastDnOH;
	UINT nCurUpOH = nUpOHTotal - m_nLastUpOH;

	WriteSamples(nCurDn, nCurUp, nCurDnOH, nCurUpOH);

	m_nLastSessionRecvBytes = theApp.stat_sessionReceivedBytes;
	m_nLastSessionSentBytes = theApp.stat_sessionSentBytes;
	m_nLastDnOH = nDnOHTotal;
	m_nLastUpOH = nUpOHTotal;
	m_dwLastSampled = dwNow;
}

void CPerfLog::Shutdown()
{
	if (m_eMode == OneSample)
		WriteSamples(0, 0, 0, 0);
}
