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
#include <sys/stat.h>
#include "emule.h"
#include "Preview.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "PartFile.h"
#include "MenuCmds.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CPreviewApps thePreviewApps;

///////////////////////////////////////////////////////////////////////////////
// CPreviewThread

IMPLEMENT_DYNCREATE(CPreviewThread, CWinThread)

BEGIN_MESSAGE_MAP(CPreviewThread, CWinThread)
END_MESSAGE_MAP()

CPreviewThread::CPreviewThread()
{
}

CPreviewThread::~CPreviewThread()
{
}

BOOL CPreviewThread::Run(){
	DbgSetThreadName("PartFilePreview");

	ASSERT (m_pPartfile) ;
	CFile* srcFile = 0;
	CFile destFile;
	try{
		srcFile = m_pPartfile->m_hpartfile.Duplicate();
		uint32 nSize = m_pPartfile->GetFileSize();
		CString strExtension = CString(strrchr(m_pPartfile->GetFileName(), '.'));
		CString strPreviewName = CString(thePrefs.GetTempDir())+ CString("\\") + CString(m_pPartfile->GetFileName()).Mid(0,5) + CString("_preview") + strExtension;
		bool bFullSized = true;
		if (!strExtension.CompareNoCase(".mpg") || !strExtension.CompareNoCase(".mpeg"))
			bFullSized = false;
		destFile.Open(strPreviewName, CFile::modeWrite | CFile::shareExclusive | CFile::modeCreate);
		srcFile->SeekToBegin();
		if (bFullSized)
			destFile.SetLength(nSize);
		destFile.SeekToBegin();
		BYTE abyBuffer[4096];
		uint32 nRead;
		while (destFile.GetPosition()+4096 < PARTSIZE*2){
			nRead = srcFile->Read(abyBuffer,4096);
			destFile.Write(abyBuffer,nRead);
		}
		srcFile->Seek(-(PARTSIZE*2),CFile::end);
		uint32 nToGo =PARTSIZE*2;
		if (bFullSized)
			destFile.Seek(-(PARTSIZE*2),CFile::end);
		do{
			nRead = (nToGo - 4096 < 1)? nToGo:4096;
			nToGo -= nRead;
			nRead = srcFile->Read(abyBuffer,4096);
			destFile.Write(abyBuffer,nRead);
		}
		while (nToGo);
		destFile.Close();
		srcFile->Close();
		m_pPartfile->m_bPreviewing = false;

		SHELLEXECUTEINFO SE;
		memset(&SE,0,sizeof(SE));
		SE.fMask = SEE_MASK_NOCLOSEPROCESS ;
		SE.lpVerb = "open";
		
		CString path;
		if (m_player.GetLength()>0) {

			char shortPath[512]; //Cax2 short path for vlc
			GetShortPathName(strPreviewName,shortPath,512);

			path=thePrefs.GetVideoPlayer();
			int pos=path.ReverseFind('\\');
			if (pos==-1) path=""; else path=path.Left(pos+1);
			SE.lpFile = m_player.GetBuffer();
			SE.lpParameters=shortPath;
			SE.lpDirectory=path.GetBuffer();
		} else SE.lpFile = strPreviewName.GetBuffer();
		SE.nShow = SW_SHOW;
		SE.cbSize = sizeof(SE);
		ShellExecuteEx(&SE);
		if (SE.hProcess){
			WaitForSingleObject(SE.hProcess, INFINITE);
			CloseHandle(SE.hProcess);
		}
		CFile::Remove(strPreviewName.GetBuffer());
	}	
	catch(CFileException* error){
		m_pPartfile->m_bPreviewing = false;
		if (srcFile->m_hFile != INVALID_HANDLE_VALUE)
			srcFile->Close();
		if (destFile.m_hFile != INVALID_HANDLE_VALUE)
			destFile.Close();
		error->Delete();	//mf
	}
	if (srcFile)
		delete srcFile;
	AfxEndThread(0,true);
	return 0;
}

void CPreviewThread::SetValues(CPartFile* pPartFile,CString player){
	m_pPartfile = pPartFile;
	m_player=player;
}


///////////////////////////////////////////////////////////////////////////////
// CPreviewApps

CPreviewApps::CPreviewApps()
{
	m_tDefAppsFileLastModified = 0;
}

CString CPreviewApps::GetDefaultAppsFile() const
{
	return thePrefs.GetConfigDir() + _T("PreviewApps.dat");
}

void CPreviewApps::RemoveAllApps()
{
	m_aApps.RemoveAll();
	m_tDefAppsFileLastModified = 0;
}

int CPreviewApps::ReadAllApps()
{
	RemoveAllApps();

	CString strFilePath = GetDefaultAppsFile();
	FILE* readFile = fopen(strFilePath, "r");
	if (readFile != NULL)
	{
		CString name, url, sbuffer;
		while (!feof(readFile))
		{
			char buffer[1024];
			if (fgets(buffer, ARRSIZE(buffer), readFile) == NULL)
				break;
			sbuffer = buffer;

			// ignore comments & too short lines
			if (sbuffer.GetAt(0) == _T('#') || sbuffer.GetAt(0) == _T('/') || sbuffer.GetLength() < 5)
				continue;

			int iPos = 0;
			CString strTitle = sbuffer.Tokenize(_T("="), iPos);
			strTitle.Trim();
			if (!strTitle.IsEmpty())
			{
				CString strCommandLine = sbuffer.Tokenize(_T(";"), iPos);
				strCommandLine.Trim();
				if (!strCommandLine.IsEmpty())
				{
					LPCTSTR pszCommandLine = strCommandLine;
					LPTSTR pszCommandArgs = PathGetArgs(pszCommandLine);
					CString strCommand, strCommandArgs;
					if (pszCommandArgs)
						strCommand = strCommandLine.Left(pszCommandArgs - pszCommandLine);
					else
						strCommand = strCommandLine;
					strCommand.Trim(_T(" \t\""));
					if (!strCommand.IsEmpty())
					{
						SPreviewApp svc;
						svc.strTitle = strTitle;
						svc.strCommand = strCommand;
						svc.strCommandArgs = pszCommandArgs;
						svc.strCommandArgs.Trim();
						m_aApps.Add(svc);
					}
				}
			}
		}
		fclose(readFile);

		struct _stat st;
		if (_tstat(strFilePath, &st) == 0)
			m_tDefAppsFileLastModified = st.st_mtime;
	}

	return m_aApps.GetCount();
}

int CPreviewApps::GetAllMenuEntries(CMenu& rMenu, const CPartFile* file)
{
	if (m_aApps.GetCount() == 0)
	{
		ReadAllApps();
	}
	else
	{
		struct _stat st;
		if (_tstat(GetDefaultAppsFile(), &st) == 0 && st.st_mtime > m_tDefAppsFileLastModified)
			ReadAllApps();
	}

	for (int i = 0; i < m_aApps.GetCount(); i++)
	{
		const SPreviewApp& rSvc = m_aApps.GetAt(i);
		if (MP_PREVIEW_APP_MIN + i > MP_PREVIEW_APP_MAX)
			break;
		bool bEnabled = false;
		if (file)
		{
			if (file->GetCompletedSize() >= 16*1024)
				bEnabled = true;
		}
		rMenu.AppendMenu(MF_STRING | (bEnabled ? MF_ENABLED : MF_GRAYED), MP_PREVIEW_APP_MIN + i, rSvc.strTitle);
	}
	return m_aApps.GetCount();
}

void CPreviewApps::RunApp(CPartFile* file, UINT uMenuID)
{
	const SPreviewApp& svc = m_aApps.GetAt(uMenuID - MP_PREVIEW_APP_MIN);

	CString strPartFilePath = file->GetFullName();

	// strip available ".met" extension to get the part file name.
	if (strPartFilePath.GetLength()>4 && strPartFilePath.Right(4)==_T(".met"))
		strPartFilePath.Delete(strPartFilePath.GetLength()-4,4);

	// if the path contains spaces, quote the entire path
	if (strPartFilePath.Find(_T(' ')) != -1)
		strPartFilePath = _T('\"') + strPartFilePath + _T('\"');

	// get directory of video player application
	CString strCommandDir = svc.strCommand;
	int iPos = strCommandDir.ReverseFind(_T('\\'));
	if (iPos == -1)
		strCommandDir.Empty();
	else
		strCommandDir = strCommandDir.Left(iPos + 1);
	PathRemoveBackslash(strCommandDir.GetBuffer());
	strCommandDir.ReleaseBuffer();

	CString strArgs = svc.strCommandArgs;
	if (!strArgs.IsEmpty())
		strArgs += _T(' ');
	strArgs += strPartFilePath;

	file->FlushBuffer(true);

	TRACE("Starting preview application:\n");
	TRACE("  Command =%s\n", svc.strCommand);
	TRACE("  Args    =%s\n", strArgs);
	TRACE("  Dir     =%s\n", strCommandDir);
	ShellExecute(NULL, _T("open"), svc.strCommand, strArgs, strCommandDir, SW_SHOWNORMAL);
}
