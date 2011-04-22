//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "emuledlg.h"
#include "MenuCmds.h"

// Mighty Knife: additional scheduling events
#include "PPgBackup.h"
#include "ipfilter.h"
#include "fakecheck.h"
#include "ip2country.h"
#include "log.h"
#include "SharedFilesWnd.h"  // MORPH add reload shared files on schedule
// [end] Mighty Knife

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


CScheduler::CScheduler(){
	LoadFromFile();
	SaveOriginals();
	m_iLastCheckedMinute=60;
}

CScheduler::~CScheduler(){
	// Mighty Knife: Saving of scheduler events is now done by SaveSettings!
	// -> much cleaner. This should be changed for the constructor, too...
	/*
	SaveToFile();
	*/
	// [end] Mighty Knife
	RemoveAll();
}

int CScheduler::LoadFromFile(){

	CString strName;
	CString temp;

	strName.Format(_T("%spreferences.ini"), thePrefs.GetMuleDirectory(EMULE_CONFIGDIR));
	CIni ini(strName, _T("Scheduler"));
	
	UINT max=ini.GetInt(_T("Count"),0);
	UINT count=0;

	while (count<max) {
		strName.Format(_T("Schedule#%i"),count);
		temp=ini.GetString(_T("Title"),_T(""),strName);
		if (temp!=_T("")) {
			Schedule_Struct* news= new Schedule_Struct();
			news->title=temp;
			news->day=ini.GetInt(_T("Day"),0);
			news->enabled=ini.GetBool(_T("Enabled"));
			news->time=ini.GetInt(_T("StartTime"));
			news->time2=ini.GetInt(_T("EndTime"));
			ini.SerGet(true, news->actions,
				ARRSIZE(news->actions), _T("Actions"));
			ini.SerGet(true, news->values,
				ARRSIZE(news->values), _T("Values"));

			AddSchedule(news);
			count++;
		} else break;
	}

	return count;
}

void CScheduler::SaveToFile(){

	CString temp;
	Schedule_Struct* schedule;

	CIni ini(thePrefs.GetConfigFile(), _T("Scheduler"));
	ini.WriteInt(_T("Count"), GetCount());
	
	for (uint8 i=0; i<GetCount();i++) {
		schedule=theApp.scheduler->GetSchedule(i);

		temp.Format(_T("Schedule#%i"),i);
		ini.WriteString(_T("Title"),schedule->title,temp);
		ini.WriteInt(_T("Day"),schedule->day);
		ini.WriteInt(_T("StartTime"),schedule->time);
		ini.WriteInt(_T("EndTime"),schedule->time2);
		ini.WriteBool(_T("Enabled"),schedule->enabled);

		ini.SerGet(false, schedule->actions,
			ARRSIZE(schedule->actions), _T("Actions"));
		ini.SerGet(false, schedule->values,
			ARRSIZE(schedule->values), _T("Values"));
	}
}

void CScheduler::RemoveSchedule(int index){
	
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

int CScheduler::AddSchedule(Schedule_Struct* schedule) {
	schedulelist.Add(schedule);
	return schedulelist.GetCount()-1;
}

int CScheduler::Check(bool forcecheck){
	if (!thePrefs.IsSchedulerEnabled()
		|| theApp.scheduler->GetCount()==0
		|| !theApp.emuledlg->IsRunning()) return -1;

	Schedule_Struct* schedule;
	struct tm tmTemp;
	CTime tNow = CTime(safe_mktime(CTime::GetCurrentTime().GetLocalTm(&tmTemp)));
	
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
		UINT h1,h2,m1,m2;
		CTime t1=CTime(schedule->time);
		CTime t2=CTime(schedule->time2);
		h1=t1.GetHour();	h2=t2.GetHour();
		m1=t1.GetMinute();	m2=t2.GetMinute();
		int it1,it2, itn;
		it1=h1*60 + m1;
		it2=h2*60 + m2;

		// Mighty Knife: handling of one-time-events.
		// if start-time=end-time, this is an event that should
		// occur once in that minute. In that case we add 1 to it2 to make sure
		// it happens (because of the timespan-comparison below, which happens
		// once per minute, too...)
		if (it2==it1) it2++;
		// [end] Mighty Knife

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
	original_sources=thePrefs.GetMaxSourcePerFileDefault();
	
	//EastShare START - Added by Pretender, add USS settings in scheduler tab
	original_ussmaxping=thePrefs.GetDynUpPingToleranceMilliseconds();
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
	thePrefs.SetDynUpPingToleranceMilliseconds(original_ussmaxping);
	thePrefs.SetDynUpGoingUpDivider(original_ussgoup);
	thePrefs.SetDynUpGoingDownDivider(original_ussgodown);
	thePrefs.SetMinUpload(original_ussminup);
	//EastShare END - Added by Pretender, add USS settings in scheduler tab
}

void CScheduler::ActivateSchedule(int index,bool makedefault) {
	Schedule_Struct* schedule=GetSchedule(index);

	for (int ai=0;ai<16;ai++) {
		if (schedule->actions[ai]==0) break;
		if (schedule->values[ai]==_T("") /* maybe ignore in some future cases...*/ ) continue;

		switch (schedule->actions[ai]) {
			case 1 :
				thePrefs.SetMaxUpload(_tstoi(schedule->values[ai]));
				if (makedefault)
					original_upload=(uint16)_tstoi(schedule->values[ai]); 
				break;
			case 2 :
				thePrefs.SetMaxDownload(_tstoi(schedule->values[ai]));
				if (makedefault)
					original_download=(uint16)_tstoi(schedule->values[ai]);
				break;
			case 3 :
				thePrefs.SetMaxSourcesPerFile(_tstoi(schedule->values[ai]));
				if (makedefault)
					original_sources=_tstoi(schedule->values[ai]);
				break;
			case 4 :
				thePrefs.SetMaxConsPerFive(_tstoi(schedule->values[ai]));
				if (makedefault)
					original_cons5s=_tstoi(schedule->values[ai]);
				break;
			case 5 :
				thePrefs.SetMaxConnections(_tstoi(schedule->values[ai]));
				if (makedefault)
					original_connections=_tstoi(schedule->values[ai]);
				break;
			case 6 :
				theApp.downloadqueue->SetCatStatus(_tstoi(schedule->values[ai]),MP_STOP);
				break;
			case 7 :
				theApp.downloadqueue->SetCatStatus(_tstoi(schedule->values[ai]),MP_RESUME);
				break;

			//EastShare START - Added by Pretender, add USS settings in scheduler tab
			case 8 :
				thePrefs.SetDynUpPingToleranceMilliseconds(_tstoi(schedule->values[ai]));
				if (makedefault)
					original_sources=_tstoi(schedule->values[ai]);
				break;
			case 9 :
				thePrefs.SetDynUpGoingUpDivider(_tstoi(schedule->values[ai]));
				if (makedefault)
					original_sources=_tstoi(schedule->values[ai]);
				break;
			case 10 :
				thePrefs.SetDynUpGoingDownDivider(_tstoi(schedule->values[ai]));
				if (makedefault)
					original_sources=_tstoi(schedule->values[ai]);
				break;
			case 11 :
				thePrefs.SetMinUpload(_tstoi(schedule->values[ai]));
				if (makedefault)
					original_sources=_tstoi(schedule->values[ai]);
				break;
			//EastShare END - Added by Pretender, add USS settings in scheduler tab

			// Mighty Knife: additional scheduling events
			case ACTION_BACKUP : {
					// Save everything before backup
					AddLogLine (false,GetResString (IDS_SCHED_BACKUP_LOG));
					theApp.emuledlg->SaveSettings (false);

					// backup everything
					theApp.ppgbackup->Backup(_T("*.ini"), false);
					theApp.ppgbackup->Backup(_T("*.dat"), false);
					theApp.ppgbackup->Backup(_T("*.met"), false);
				} break;
			case ACTION_UPDIPCONF : {
					AddLogLine (false,GetResString (IDS_SCHED_UPDATE_IPCONFIG_LOG));
					//MORPH START - Added by Stulle, New IP Filter by Ozzy [Stulle/Ozzy]
					/*
					theApp.ipfilter->UpdateIPFilterURL();
					*/
					theApp.emuledlg->CheckIPFilter();
					//MORPH END   - Added by Stulle, New IP Filter by Ozzy [Stulle/Ozzy]
				} break;
			case ACTION_UPDFAKES : {
					AddLogLine (false,GetResString (IDS_SCHED_UPDATE_FAKES_LOG));
					theApp.FakeCheck->DownloadFakeList();
				} break;
			case ACTION_UPDCOUNTRY : {
					AddLogLine (false,GetResString (IDS_SCHED_UPDATE_COUNTRY_LOG));
					theApp.ip2country->UpdateIP2CountryURL();
				} break;
			case ACTION_RELOAD : { // leuk_he reload on schedule
					AddLogLine (false,GetResString (IDS_SCHED_RELOAD));
					//theApp.sharedfiles->Reload(); // also update GUI... safely
					theApp.emuledlg->sharedfileswnd->SendMessage(WM_COMMAND, IDC_RELOADSHAREDFILES);
				} break;
			// [end] Mighty Knife

		}
	}
}


// MORPH START leuk_he automatic weekly ipfilter/fakefilter update. 
bool CScheduler::HasWeekly(int par_action) // MORPH 
{
	if (theApp.scheduler->GetCount()==0) return false; 
	Schedule_Struct* curschedule;

	for (uint8 si=0;si< theApp.scheduler->GetCount();si++) {
		curschedule=  GetSchedule(si);
		if (curschedule->actions[0]==0 || !curschedule->enabled) continue;
		if (curschedule->day!=DAY_DAYLY) { // nt daily, so must be weekly ( or montly, good also) 
			for (int ai=0;ai<16;ai++) {
				if (curschedule->actions[ai]==par_action) 
					return true;
			}
		}
	}
	// not found? then schedule does not exit.
    return false;
}


void    CScheduler::SetWeekly(int action,bool activate) // MORPH leuk_he : automatic weekly ipfilter/fakefilter update. 
{
	bool Currentactivated = HasWeekly(action);
    if (  Currentactivated == activate) 
		 return; // nothing to do. 

	if ( ( Currentactivated == false )&& (activate == true)) { // must we isnert a new? 
     	Schedule_Struct* newschedule=new Schedule_Struct();
		struct tm tmTemp;
	    CTime tNow = CTime(safe_mktime(CTime::GetCurrentTime().GetLocalTm(&tmTemp)));
	
	    newschedule->day=tNow.GetDayOfWeek();  // 
	    newschedule->enabled=true;
	    newschedule->time=time(NULL);
	    newschedule->time2=time(NULL);
	    newschedule->title=GetResString(IDS_SCHEDTEXT);
	    newschedule->ResetActions();
		newschedule->actions[0]=action;
		newschedule->values[0]=L"update";
		AddSchedule(newschedule);
		thePrefs.scheduler=true; // enable scheduler
	}
	if ((Currentactivated == true )&& (activate == false)) { // we must delete

	Schedule_Struct* curschedule;
	for (uint8 si=0;si< GetCount();si++) {
		curschedule=  GetSchedule(si);
		if (curschedule->actions[0]==0 || !curschedule->enabled) continue;
		if (curschedule->day!=DAY_DAYLY) { // nt daily, so must be weekly ( or montly, good also) 
			for (int ai=0;ai<16;ai++) {
				if (curschedule->actions[ai]==action) {
					RemoveSchedule(si);
				    return ;
				}
			}
		}
	}
	// not found? then schedule does not exist. ASSERT()?;
    return ;
	}
};

// MORPH END leuk_he automatic weekly ipfilter/fakefilter update. 
