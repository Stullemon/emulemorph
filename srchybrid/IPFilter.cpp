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
#include "emule.h"
#include "IPFilter.h"
#include "otherfunctions.h"
#include "Preferences.h"
#include "HttpDownloadDlg.h"//MORPH START added by Yun.SF3: Ipfilter.dat update
#include "emuleDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CIPFilter::CIPFilter(){
	lasthit="";
	LoadFromFile();
}

CIPFilter::~CIPFilter(){
	RemoveAllIPs();
}

bool CIPFilter::AddBannedIPRange(uint32 IPfrom,uint32 IPto,uint8 filter, CString desc){
	IPRange_Struct* newFilter=new IPRange_Struct();

	newFilter->IPstart=IPfrom;
	newFilter->IPend=IPto;
	newFilter->filter=filter;
	newFilter->description=desc;

	// no filters with same startrange allowed for quick key-search (for now)
	bool discard=(IsFiltered(htonl(IPfrom),-1));
	if (discard)
		delete newFilter;
	else
		iplist[IPfrom]=newFilter;

	return !discard;
}

int CIPFilter::LoadFromFile(){
	CString sbuffer,sbuffer2,sbuffer3,sbuffer4;
	int pos,filtercounter;
	uint32 ip1,ip2;
	uint8 filter;
	char buffer[1024];
	int lenBuf = 1024;
	filtercounter=0;

	RemoveAllIPs();

	FILE* readFile= fopen(CString(theApp.glob_prefs->GetConfigDir())+"ipfilter.dat", "r");
	if (readFile!=NULL) {
		while (!feof(readFile)) {
			if (fgets(buffer,lenBuf,readFile)==0) break;
			sbuffer=buffer;
			
			// ignore comments & too short lines
			if (sbuffer.GetAt(0) == '#' || sbuffer.GetAt(0) == '/' || sbuffer.GetLength()<5)
				continue;
			
			// get & test & process IP range
			pos=sbuffer.Find(',');
			if (pos==-1) continue;
			sbuffer2=sbuffer.Left(pos).Trim();
			pos=sbuffer2.Find("-");
			if (pos==-1) continue;
			sbuffer3=sbuffer2.Left(pos).Trim();
			sbuffer4=sbuffer2.Right(sbuffer2.GetLength()-pos-1).Trim();

			ip1=ip2=0;
			int counter = 0;
			CString temp;
			bool skip=false;

			for( int i = 0; i < 4; i++){
				sbuffer3.Tokenize(".",counter);
				if( counter == -1 ){ skip=true;break;}
			}
			counter=0;
			for( int i = 0; i < 4; i++){
				sbuffer4.Tokenize(".",counter);
				if( counter == -1 ){ skip=true;break;}
			}
			if (skip) continue;
			
			// Elandal: Avoids floating point math on 2's power
			// removes warning regarding implicit cast
			counter=0;
			for(int i=0; i<4; i++){ 
				temp = sbuffer3.Tokenize(".",counter);
				ip1 += atoi(temp) * (1L << (8*(3-i)));
			}
			counter=0;
			for(int i=0; i<4; i++){ 
				temp = sbuffer4.Tokenize(".",counter);
				ip2 += atoi(temp) * (1L << (8*(3-i)));
			}

			// filter
			pos=sbuffer.Find(",",pos+1);
			int pos2=sbuffer.Find(",",pos+1);
			if (pos2==-1) continue;
			sbuffer2=sbuffer.Mid(pos+1,pos2-pos-1).Trim();
			filter=atoi(sbuffer2);

			sbuffer2=sbuffer.Right(sbuffer.GetLength()-pos2-1);
			if (sbuffer2.GetAt(sbuffer2.GetLength()-1)==10) sbuffer2.Delete(sbuffer2.GetLength()-1);

			// add a filter
			if (AddBannedIPRange(ip1,ip2,filter,sbuffer2 ))
				filtercounter++;

		}
		fclose(readFile);
	}
	//MORPH START - Added by IceCream, Ipfilter logline at launch is now fixed
	theApp.emuledlg->AddLogLine(false, GetResString(IDS_IPFILTERLOADED),filtercounter);
	//MORPH END   - Added by IceCream, Ipfilter logline at launch is now fixed
	return filtercounter;
}

void CIPFilter::SaveToFile(){
	// nope
}

void CIPFilter::RemoveAllIPs(){
	IPRange_Struct* search;
	
	std::map<uint32, IPRange_Struct*>::const_iterator it;
	for ( it = iplist.begin(); it != iplist.end(); ++it ) {
		search=(*it).second;
		delete search;
	}

	iplist.clear();
}

bool CIPFilter::IsFiltered(uint32 IP2test){
	return IsFiltered(IP2test,theApp.glob_prefs->GetIPFilterLevel());
}

bool CIPFilter::IsFiltered(uint32 IP2test,int level){
	if (iplist.size()==0 || IP2test==0) return false;

	IPRange_Struct* search;
	IP2test=htonl(IP2test);

	std::map<uint32, IPRange_Struct*>::const_iterator it=iplist.upper_bound(IP2test);
	it--;
	do {
		search=(*it).second;
		if (search->IPend<IP2test) return false;
		if (level==-1 && search->IPstart==IP2test ) return true;
		if (search->IPstart<=IP2test && search->IPend>=IP2test && search->filter<level ) {
			lasthit=search->description;
			return true;
		}
		it--;
	} while (it!=iplist.begin());
	return false;
}
//MORPH START added by Yun.SF3: Ipfilter.dat update
void CIPFilter::UpdateIPFilterURL()
{
	char buffer[5];
	int lenBuf = 5;
	CString sbuffer;
	CString strURL = theApp.glob_prefs->GetUpdateURLIPFilter();
	CString strTempFilename;
	strTempFilename.Format(CString(theApp.glob_prefs->GetAppDir())+"ipfilter.txt");
	FILE* readFile= fopen(strTempFilename, "r");
	CHttpDownloadDlg dlgDownload;
	dlgDownload.m_sURLToDownload = strURL;
	dlgDownload.m_sFileToDownloadInto = strTempFilename;
	if (dlgDownload.DoModal() != IDOK)
	{
		theApp.emuledlg->AddLogLine(true, "Error downloading %s", strURL);
		return;
	}
	readFile= fopen(strTempFilename, "r");
	fgets(buffer,lenBuf,readFile);
		//return false;
	sbuffer = buffer;
	sbuffer = sbuffer.Trim();
	fclose(readFile);
	remove(strTempFilename);
	CString IPFilterURL;
	//IPFilterURL = "http://pages.xtn.net/~charliey/FoX/Morph/ipfilter.dat";
	IPFilterURL = theApp.glob_prefs->GetUpdateURLIPFilter().TrimRight(".txt") + ".dat";
	strTempFilename.Format(CString(theApp.glob_prefs->GetConfigDir())+"ipfilter.dat");
	readFile= fopen(strTempFilename, "r");
	// Mighty Knife: cleanup - removed that nasty signed-unsigned-message
	if ((theApp.glob_prefs->GetIPfilterVersion()< (uint32) atoi(sbuffer)) || (readFile == NULL)) {
		theApp.glob_prefs->SetIpfilterVersion(atoi(sbuffer));
	// [end] Mighty Knife
	if (readFile!=NULL) {
		fclose(readFile);
		remove(strTempFilename);
	}
	CHttpDownloadDlg dlgDownload;
	dlgDownload.m_sURLToDownload = IPFilterURL;
	dlgDownload.m_sFileToDownloadInto = strTempFilename;
	if (dlgDownload.DoModal() != IDOK)
	{
		theApp.emuledlg->AddLogLine(true,GetResString(IDS_UPDATEIPFILTERERROR));
	}
	else
	{
	int count=LoadFromFile();
	}
	}
}
//MORPH END added by Yun.SF3: Ipfilter.dat update
