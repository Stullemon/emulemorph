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
#include "MorphVersionList.h"
#include "emule.h"
#include "Preferences.h" // for thePrefs
#include "emuledlg.h" // for theApp.emuledlg
#include "log.h" // for log

CMorphVersionList theMorphVerList;

CMorphVersionList::CMorphVersionList(void){}
CMorphVersionList::~CMorphVersionList(void){}

void CMorphVersionList::LoadList()
{
	SettingsList daten;
	CString datafilepath;
	datafilepath.Format(_T("%s\\%s.txt"), thePrefs.GetConfigDir(), _T("ValidMorphs"));

	CString strLine;
	CStdioFile f;
	if (!f.Open(datafilepath, CFile::modeReadWrite | CFile::typeText))
		return;
	while(f.ReadString(strLine))
	{
		if (strLine.GetAt(0) == _T('#'))
			continue;
		int pos = strLine.Find(_T('\0'));
		if (pos == -1)
			continue;
		CString strData = strLine.Left(pos);
		CSettingsData* newdata = new CSettingsData(_tstol(strData));
		daten.AddTail(newdata);
	}
	f.Close();

	m_uListLength = daten.GetCount()-1;
	
	if(m_uListLength < CemuleApp::m_nMVersionMjr)
	{
		m_bListValid = false;
		return;
	}

	POSITION pos = daten.GetHeadPosition();
	if(!pos)
		return;

	m_bListValid = ((CSettingsData*)daten.GetAt(pos))->dwData != 0;

	for (int i = 1; i < m_uListLength+1; i++)
	{
		daten.GetNext(pos);

		m_uMorphSubVer[i] = ((CSettingsData*)daten.GetAt(pos))->dwData;
	}

	while (!daten.IsEmpty()) 
		delete daten.RemoveHead();
}

void CMorphVersionList::SaveList()
{
	CString datafilepath;
	datafilepath.Format(_T("%s\\%s.txt"), thePrefs.GetConfigDir(), _T("ValidMorphs"));
	
	CString strLine;
	CStdioFile f;

	if (!f.Open(datafilepath, CFile::modeCreate | CFile::modeWrite | CFile::typeText))
		return;

	f.WriteString(_T("#Valid Morph versions:\n"));
	f.WriteString(strLine);
	strLine.Format(_T("%ld\n"), m_bListValid);
	f.WriteString(strLine);

	for (int i = 1; i < m_uListLength+1; i++)
	{
		strLine.Format(_T("%ld\n"), m_uMorphSubVer[i]);
		f.WriteString(strLine);
	}

	f.Close();
}
