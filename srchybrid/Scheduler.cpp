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
#include "Scheduler.h"
#include "OtherFunctions.h"
#include "ini2.h"
#include "Preferences.h"
#include "DownloadQueue.h"
#ifndef _CONSOLE
#include "emuledlg.h"
#include "MenuCmds.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CScheduler::CScheduler(){
	LoadFromFile();
	SaveOriginals();
	m_iLastCheckedMinute=60;
}

CScheduler::~CScheduler(){
	SaveToFile();
	RemoveAll();
}

int CScheduler::LoadFromFile(){

	CString strName;
	CString temp;

	strName.Format("%spreferences.ini",thePrefs.GetConfigDir());
	CIni ini(strName, "Scheduler");
	
	uint8 max=ini.GetInt("Count",0);
	uint8 count=0;

	while (count<max) {
		strName.Format("Schedule#%i",count);
		temp=ini.GetString("Title","",strName);
		if (temp!="") {
			Schedule_Struct* news= new Schedule_Struct();
			news->title=temp;
			news->day=ini.GetInt("Day",0);
			news->enabled=ini.GetBool("Enabled");
			news->time=ini.GetInt("StartTime");
			news->time2=ini.GetInt("EndTime");
			ini.SerGet(true, news->actions,
				ARRSIZE(news->actions), "Actions");
			ini.SerGet(true, news->values,
				ARRSIZE(news->values), "Values");

			AddSchedule(news);
			count++;
		} else break;
	}
	return count;
}

void CScheduler::SaveToFile(){
	if (theApp.scheduler->GetCount()==0) return;

	CString strName;
	CString temp;
	Schedule_Struct* schedule;

	strName.Format("%spreferences.ini",thePrefs.GetConfigDir());

	CIni ini(strName, "Scheduler");

	ini.WriteInt("Count", GetCount());
	
	for (uint8 i=0; i<GetCount();i++) {
		schedule=theApp.scheduler->GetSchedule(i);

		strName.Format("Schedule#%i",i);
		ini.WriteString("Title",schedule->title,strName);
		ini.WriteInt("Day",schedule->day);
		ini.WriteInt("StartTime",schedule->time);
		ini.WriteInt("EndTime",schedule->time2);
		ini.WriteBool("Enabled",schedule->enabled);

		ini.SerGet(false, schedule->actions,
			ARRSIZE(schedule->actions), "Actions");
		ini.SerGet(false, schedule->values,
			ARRSIZE(schedule->values), "Values");
	}
}

void CScheduler::RemoveSchedule(uint8 index){
	
	if (index>=schedulelist.GetCount()) return;

	Schedule_Struct* todel;
	todel=schedulelist.GetAt(index);
	delete todel;
	schedulelist.RemoveAt(index);
}

void CScheduler::RemoveAll(){
	while( schedulelist.GetCount()>0 )
		RemoveSchedule(0);
}

uint8 CScheduler::AddSchedule(Schedule_Struct* schedule) {
	schedulelist.Add(schedule);
	return schedulelist.GetCount()-1;
}

int CScheduler::Check(bool forcecheck){
	if (!thePrefs.IsSchedulerEnabled()
		|| theApp.scheduler->GetCount()==0
		|| !theApp.emuledlg->IsRunning()) return -1;

	Schedule_Struct* schedule;
	CTime tNow=CTime(safe_mktime(CTime::GetCurrentTime().GetLocalTm()));
	
	if (!forcecheck && tNow.GetMinute()==m_iLastCheckedMinute) return -1;

	m_iLastCheckedMinute=tNow.GetMinute();
	theApp.scheduler->RestoreOriginals();

	for (uint8 si=0;si<theApp.scheduler->GetCount();si++) {
		schedule=theApp.scheduler->GetSchedule(si);
		if (schedule->actions[0]==0 || !schedule->enabled) continue;

		// check day of week
		if (schedule->day!=DAY_DAYLY) {
			int dow=tNow.GetDayOfWeek();
			switch (schedule->day) {
				case DAY_MO : if (dow!=2) continue;
					break;
				case DAY_DI : if (dow!=3) continue;
					break;
				case DAY_MI : if (dow!=4) continue;
					break;
				case DAY_DO : if (dow!=5) continue;
					break;
				case DAY_FR : if (dow!=6) continue;
					break;
				case DAY_SA : if (dow!=7) continue;
					break;
				case DAY_SO : if (dow!=1) continue;
					break;
				case DAY_MO_FR : if (dow==7 || dow==1 ) continue;
					break;
				case DAY_MO_SA : if (dow==1) continue;
					break;
				case DAY_SA_SO : if (dow>=2 && dow<=6) continue;
			}
		}

		//check time
		uint8 h1,h2,m1,m2;
		CTime t1=CTime(schedule->time);
		CTime t2=CTime(schedule->time2);
		h1=t1.GetHour();	h2=t2.GetHour();
		m1=t1.GetMinute();	m2=t2.GetMinute();
		int it1,it2, itn;
		it1=h1*60 + m1;
		it2=h2*60 + m2;
		itn=tNow.GetHour()*60 + tNow.GetMinute();
		if (it1<=it2) { // normal timespan
			if ( !(itn>=it1 && itn<it2) ) continue;
		} else {		   // reversed timespan (23:30 to 5:10)  now 10
			if ( !(itn>=it1 || itn<it2)) continue;
		}

		// ok, lets do the actions of this schedule
		ActivateSchedule(si,schedule->time2==0);
	}

	return -1;
}

void CScheduler::SaveOriginals() {
	original_upload=thePrefs.GetMaxUpload();
	original_download=thePrefs.GetMaxDownload();
	original_connections=thePrefs.GetMaxConnections();
	original_cons5s=thePrefs.GetMaxConperFive();
	original_sources=thePrefs.GetMaxSourcePerFile();
	
	//EastShare START - Added by Pretender, add USS settings in scheduler tab
	original_ussmaxping=thePrefs.GetDynUpPingLimit();
	original_ussgoup=thePrefs.GetDynUpGoingUpDivider();
	original_ussgodown=thePrefs.GetDynUpGoingDownDivider();
	original_ussminup=thePrefs.GetMinUpload();
	//EastShare END - Added by Pretender, add USS settings in scheduler tab
}

void CScheduler::RestoreOriginals() {
	thePrefs.SetMaxUpload(original_upload);
	thePrefs.SetMaxDownload(original_download);
	thePrefs.SetMaxConnections(original_connections);
	thePrefs.SetMaxConsPerFive(original_cons5s);
	thePrefs.SetMaxSourcesPerFile(original_sources);

	//EastShare START - Added by Pretender, add USS settings in scheduler tab
	thePrefs.SetDynUpPingLimit(original_ussmaxping);
	thePrefs.SetDynUpGoingUpDivider(original_ussgoup);
	thePrefs.SetDynUpGoingDownDivider(original_ussgodown);
	thePrefs.SetMinUpload(original_ussminup);
	//EastShare END - Added by Pretender, add USS settings in scheduler tab
}

void CScheduler::ActivateSchedule(uint8 index,bool makedefault) {
	Schedule_Struct* schedule=GetSchedule(index);

	for (uint8 ai=0;ai<16;ai++) {
		if (schedule->actions[ai]==0) break;
		if (schedule->values[ai]=="" /* maybe ignore in some future cases...*/ ) continue;

		switch (schedule->actions[ai]) {
			case 1 : thePrefs.SetMaxUpload(atoi(schedule->values[ai]));
				if (makedefault) original_upload=atoi(schedule->values[ai]); 
				break;
			case 2 : thePrefs.SetMaxDownload(atoi(schedule->values[ai]));
				if (makedefault) original_download=atoi(schedule->values[ai]);
				break;
			case 3 : thePrefs.SetMaxSourcesPerFile(atoi(schedule->values[ai]));
				if (makedefault) original_sources=atoi(schedule->values[ai]);
				break;
			case 4 : thePrefs.SetMaxConsPerFive(atoi(schedule->values[ai]));
				if (makedefault) original_cons5s=atoi(schedule->values[ai]);
				break;
			case 5 : thePrefs.SetMaxConnections(atoi(schedule->values[ai]));
				if (makedefault) original_connections=atoi(schedule->values[ai]);
				break;
			
			case 6 : theApp.downloadqueue->SetCatStatus(atoi(schedule->values[ai]),MP_STOP);break;
			case 7 : theApp.downloadqueue->SetCatStatus(atoi(schedule->values[ai]),MP_RESUME);break;

			//EastShare START - Added by Pretender, add USS settings in scheduler tab
			case 8 : thePrefs.SetDynUpPingLimit(atoi(schedule->values[ai]));
				if (makedefault) original_sources=atoi(schedule->values[ai]);
				break;
			case 9 : thePrefs.SetDynUpGoingUpDivider(atoi(schedule->values[ai]));
				if (makedefault) original_sources=atoi(schedule->values[ai]);
				break;
			case 10 : thePrefs.SetDynUpGoingDownDivider(atoi(schedule->values[ai]));
				if (makedefault) original_sources=atoi(schedule->values[ai]);
				break;
			case 11 : thePrefs.SetMinUpload(atoi(schedule->values[ai]));
				if (makedefault) original_sources=atoi(schedule->values[ai]);
				break;
			//EastShare END - Added by Pretender, add USS settings in scheduler tab
		}
	}
}