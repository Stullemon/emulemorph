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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CFakecheck::CFakecheck(){
	lasthit="";
	LoadFromFile();
}

CFakecheck::~CFakecheck(){
	RemoveAllFakes();
}

void CFakecheck::AddFake(CString Hash,uint32 Lenght,CString Realtitle){
	Fakes_Struct* newFilter=new Fakes_Struct();

	newFilter->Hash=Hash;
	newFilter->Lenght=Lenght;
	newFilter->RealTitle=Realtitle;
	Fakelist[Hash]=newFilter;
}

int CFakecheck::LoadFromFile(){
	CString sbuffer,sbuffer2,sbuffer3,sbuffer4;
	int pos,fakecounter;
	CString Hash,Title;
	uint32 Lenght;
	char buffer[1024];
	int lenBuf = 1024;
	fakecounter=0;
	RemoveAllFakes();
	FILE* readFile= fopen(CString(theApp.glob_prefs->GetConfigDir())+"fakes.dat", "r");
	if (readFile!=NULL) {
		while (!feof(readFile)) {
			if (fgets(buffer,lenBuf,readFile)==0) break;
			sbuffer=buffer;
			if (sbuffer.GetAt(0) == '#' || sbuffer.GetAt(0) == '/' || sbuffer.GetLength()<5)
				continue;
			pos=sbuffer.Find(',');
			if (pos==-1) continue;
			Hash=sbuffer.Left(pos).Trim();
			Hash.MakeUpper();
			int pos2=sbuffer.Find(",",pos+1);
			if (pos2==-1) continue;
			sbuffer2=sbuffer.Mid(pos+1,pos2-pos-1).Trim();
			Lenght=atoi(sbuffer2);
			sbuffer2=sbuffer.Mid(pos2+1,sbuffer.GetLength()-pos2-2);
			Title =	sbuffer2;
			AddFake(Hash,Lenght,Title);
			fakecounter++;
		}
		fclose(readFile);
		return fakecounter;
	} else {
		return 0;
	}

}

void CFakecheck::RemoveAllFakes(){
	Fakes_Struct* search;
	
	map<CString, Fakes_Struct*>::const_iterator it;
	for ( it = Fakelist.begin(); it != Fakelist.end(); ++it ) {
		search=(*it).second;
		delete search;
	}

	Fakelist.clear();
}

CString CFakecheck::IsFake(CString Hash2test, uint32 lenght){
	if (Fakelist.size()==0) return ""; //MORPH - Modified by IceCream, return a CString
	Fakes_Struct* search;
	
	map<CString, Fakes_Struct*>::const_iterator it=Fakelist.upper_bound(Hash2test);
	it--;
	do {
		search=(*it).second;
		if (search->Hash == Hash2test && search->Lenght == lenght) {
			lasthit=search->RealTitle;
			return lasthit;
		} else
			return "OK";
		it--;
	} while (it!=Fakelist.begin());
	return ""; //MORPH - Modified by IceCream, return a CString
}
bool CFakecheck::DownloadFakeList(){
	char buffer[5];
	int lenBuf = 5;
	CString sbuffer;
	CString strURL = theApp.glob_prefs->GetUpdateURLFakeList();
	CString strTempFilename;
	strTempFilename.Format(CString(theApp.glob_prefs->GetAppDir())+"fakes.txt");
	FILE* readFile= fopen(strTempFilename, "r");
	CHttpDownloadDlg dlgDownload;
	dlgDownload.m_sURLToDownload = strURL;
	dlgDownload.m_sFileToDownloadInto = strTempFilename;
	if (dlgDownload.DoModal() != IDOK)
	{
		theApp.emuledlg->AddLogLine(true, "Error downloading %s", strURL);
		return false;
	}
	readFile= fopen(strTempFilename, "r");
	fgets(buffer,lenBuf,readFile);
		//return false;
	sbuffer = buffer;
	sbuffer = sbuffer.Trim();
	fclose(readFile);
	remove(strTempFilename);
	CString FakeCheckURL;
	//FakeCheckURL = "http://www.emuleitor.com/downloads/Morph/fakes.dat";
	FakeCheckURL = theApp.glob_prefs->GetUpdateURLFakeList().TrimRight(".txt")+".dat";
	strTempFilename.Format(CString(theApp.glob_prefs->GetConfigDir())+"fakes.dat");
	readFile= fopen(strTempFilename, "r");
	// Mighty Knife: cleanup - removed that nasty signed-unsigned-message
	if ((theApp.glob_prefs->GetFakesDatVersion() < (uint32) atoi(sbuffer))) {
		theApp.glob_prefs->SetFakesDatVersion(atoi(sbuffer));
	if (readFile!=NULL) {
		fclose(readFile);
		remove(strTempFilename);
	}
	CHttpDownloadDlg dlgDownload;
	dlgDownload.m_sURLToDownload = FakeCheckURL;
	dlgDownload.m_sFileToDownloadInto = strTempFilename;
	if (dlgDownload.DoModal() != IDOK)
	{
		theApp.emuledlg->AddLogLine(true,GetResString(IDS_FAKECHECKUPERROR));
		return false;
		}
	}
	return (LoadFromFile()>0);
}