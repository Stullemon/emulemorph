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

// Preview.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "Preview.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CPreview

IMPLEMENT_DYNCREATE(CPreviewThread, CWinThread)

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
		CString strPreviewName = CString(theApp.glob_prefs->GetTempDir())+ CString("\\") + CString(m_pPartfile->GetFileName()).Mid(0,5) + CString("_preview") + strExtension;
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
		MEMSET(&SE,0,sizeof(SE));
		SE.fMask = SEE_MASK_NOCLOSEPROCESS ;
		SE.lpVerb = "open";
		
		CString path;
		if (m_player.GetLength()>0) {

			char shortPath[512]; //Cax2 short path for vlc
			GetShortPathName(strPreviewName,shortPath,512);

			path=theApp.glob_prefs->GetVideoPlayer();
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
		OUTPUT_DEBUG_TRACE();
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

BEGIN_MESSAGE_MAP(CPreviewThread, CWinThread)
END_MESSAGE_MAP()


// CPreview message handlers
