//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include <share.h>
#include "emule.h"
#include "Preview.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "PartFile.h"
#include "MenuCmds.h"
#include "opcodes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
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

BOOL CPreviewThread::InitInstance()
{
	DbgSetThreadName("PartFilePreview");
	InitThreadLocale();
	return TRUE;
}

BOOL CPreviewThread::Run()
{
	// SLUGFILLER: SafeHash
	CReadWriteLock lock(&theApp.m_threadlock);
	if (!lock.ReadLock(0))
		return 0;
	// SLUGFILLER: SafeHash

	ASSERT (m_pPartfile) ;
	CFile destFile;
	CFile srcFile;
	if (!srcFile.Open(m_pPartfile->GetFilePath(), CFile::modeRead | CFile::shareDenyNone))
		return FALSE;
	try{
		uint32 nSize = m_pPartfile->GetFileSize();
		CString strExtension = CString(_tcsrchr(m_pPartfile->GetFileName(), _T('.')));
		CString strPreviewName = m_pPartfile->GetTempPath() + _T("\\") + m_pPartfile->GetFileName().Mid(0, 5) + _T("_preview") + strExtension;
		bool bFullSized = true;
		if (!strExtension.CompareNoCase(_T(".mpg")) || !strExtension.CompareNoCase(_T(".mpeg")))
			bFullSized = false;
		if (!destFile.Open(strPreviewName, CFile::modeWrite | CFile::shareDenyWrite | CFile::modeCreate))
			return FALSE;
		srcFile.SeekToBegin();
		if (bFullSized)
			destFile.SetLength(nSize);
		destFile.SeekToBegin();
		BYTE abyBuffer[4096];
		uint32 nRead;
		while (destFile.GetPosition()+4096 < PARTSIZE*2){
			nRead = srcFile.Read(abyBuffer,4096);
			destFile.Write(abyBuffer,nRead);
		}
		srcFile.Seek(-(PARTSIZE*2),CFile::end);
		uint32 nToGo =PARTSIZE*2;
		if (bFullSized)
			destFile.Seek(-(PARTSIZE*2),CFile::end);
		do{
			nRead = (nToGo - 4096 < 1)? nToGo:4096;
			nToGo -= nRead;
			nRead = srcFile.Read(abyBuffer,4096);
			destFile.Write(abyBuffer,nRead);
		}
		while (nToGo);
		destFile.Close();
		srcFile.Close();
		m_pPartfile->m_bPreviewing = false;

		SHELLEXECUTEINFO SE;
		memset(&SE,0,sizeof(SE));
		SE.fMask = SEE_MASK_NOCLOSEPROCESS ;
		SE.lpVerb = _T("open");
		
		CString path;
		if (!m_player.IsEmpty())
		{
			TCHAR shortPath[512]; //Cax2 short path for vlc
			GetShortPathName(strPreviewName, shortPath, ARRSIZE(shortPath));

			path=thePrefs.GetVideoPlayer();
			int pos = path.ReverseFind(_T('\\'));
			if (pos == -1)
				path.Empty();
			else
				path = path.Left(pos + 1);
			SE.lpFile = m_player.GetBuffer();
			SE.lpParameters=shortPath;
			SE.lpDirectory=path.GetBuffer();
		}
		else
			SE.lpFile = strPreviewName.GetBuffer();
		SE.nShow = SW_SHOW;
		SE.cbSize = sizeof(SE);
		ShellExecuteEx(&SE);
		if (SE.hProcess){
			WaitForSingleObject(SE.hProcess, INFINITE);
			CloseHandle(SE.hProcess);
		}
		CFile::Remove(strPreviewName);
	}	
	catch(CFileException* error){
		m_pPartfile->m_bPreviewing = false;
		error->Delete();
	}
	return TRUE;
}

void CPreviewThread::SetValues(CPartFile* pPartFile, CString player)
{
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
	FILE* readFile = _tfsopen(strFilePath, _T("r"), _SH_DENYWR);
	if (readFile != NULL)
	{
		CString name, url, sbuffer;
		while (!feof(readFile))
		{
			TCHAR buffer[1024];
			if (_fgetts(buffer, ARRSIZE(buffer), readFile) == NULL)
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
						UINT uMinCompletedSize = 0;
						UINT uMinStartOfFile = 0;
						CStringArray astrExtensions;
						CString strParams = sbuffer.Tokenize(_T(";"), iPos);
						while (!strParams.IsEmpty())
						{
							int iPosParam = 0;
							CString strId = strParams.Tokenize(_T("="), iPosParam);
							if (!strId.IsEmpty())
							{
								CString strValue = strParams.Tokenize(_T("="), iPosParam);
								if (strId.CompareNoCase(_T("Ext")) == 0)
								{
									if (!strValue.IsEmpty())
									{
										if (strValue[0] != _T('.'))
											strValue = _T('.') + strValue;
										astrExtensions.Add(strValue);
									}
								}
								else if (strId.CompareNoCase(_T("MinSize")) == 0)
								{
									if (!strValue.IsEmpty())
										_stscanf(strValue, _T("%u"), &uMinCompletedSize);
								}
								else if (strId.CompareNoCase(_T("MinStart")) == 0)
								{
									if (!strValue.IsEmpty())
										_stscanf(strValue, _T("%u"), &uMinStartOfFile);
								}
							}
							strParams = sbuffer.Tokenize(_T(";"), iPos);
						}

						SPreviewApp svc;
						svc.strTitle = strTitle;
						svc.strCommand = strCommand;
						svc.strCommandArgs = pszCommandArgs;
						svc.strCommandArgs.Trim();
						svc.astrExtensions.Append(astrExtensions);
						svc.uMinCompletedSize = uMinCompletedSize;
						svc.uMinStartOfFile = uMinStartOfFile;
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

void CPreviewApps::UpdateApps()
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
}

int CPreviewApps::GetAllMenuEntries(CMenu& rMenu, const CPartFile* file)
{
	UpdateApps();

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

	CString strCommand = svc.strCommand;
	ExpandEnvironmentStrings(strCommand);
	ExpandEnvironmentStrings(strArgs);
	ExpandEnvironmentStrings(strCommandDir);

	TRACE("Starting preview application:\n");
	TRACE("  Command =%s\n", strCommand);
	TRACE("  Args    =%s\n", strArgs);
	TRACE("  Dir     =%s\n", strCommandDir);
	ShellExecute(NULL, _T("open"), strCommand, strArgs, strCommandDir, SW_SHOWNORMAL);
}

int CPreviewApps::GetPreviewApp(const CPartFile* file)
{
	LPCTSTR pszExt = PathFindExtension(file->GetFileName());
	if (pszExt == NULL)
		return -1;

	UpdateApps();

	int iApp = -1;
	for (int i = 0; iApp == -1 && i < m_aApps.GetCount(); i++)
	{
		const SPreviewApp& rApp = m_aApps.GetAt(i);
		for (int j = 0; j < rApp.astrExtensions.GetCount(); j++)
		{
			if (rApp.astrExtensions.GetAt(j).CompareNoCase(pszExt) == 0) {
				iApp = i;
				break;
			}
		}
	}

	return iApp;
}

CPreviewApps::ECanPreviewRes CPreviewApps::CanPreview(const CPartFile* file)
{
	int iApp = GetPreviewApp(file);
	if (iApp == -1)
		return NotHandled;

	const SPreviewApp* pApp = &m_aApps.GetAt(iApp);
	if (pApp->uMinCompletedSize != 0)
	{
		if (file->GetCompletedSize() < pApp->uMinCompletedSize)
			return No;
	}

	if (pApp->uMinStartOfFile != 0)
	{
		if (!file->IsComplete(0, pApp->uMinStartOfFile, false))
			return No;
	}

	return Yes;
}

bool CPreviewApps::Preview(CPartFile* file)
{
	int iApp = GetPreviewApp(file);
	if (iApp == -1)
		return false;
	RunApp(file, MP_PREVIEW_APP_MIN + iApp);
	return true;
}
