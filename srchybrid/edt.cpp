// enkeyDev(th1) -EDT- Estimation of Download Time
//
// Estimates dowload time in two ways:
// 1) recording the "entry" score for the upload process to begin, then
//    with the knowledge of currenct score and rate (points per 100 secs)
//    estimates the time to reach the entry score; uses linear interpolation
//    to predict the entry score at entry time
// 2) estimating the number of users per time unit that starts the upload and,
//    weighted by rate, evaluating the average period for the user to start upload;
// If method 1 gives a senseless result, method 2 is used.

#include "stdafx.h"
#include "emule.h"
#include "edt.h"

CEdt::CEdt()
{
	ResetWaitingPeriodCache();
}

void CEdt::ResetWaitingPeriodCache()
{
	m_last_server_time = 0;
	m_last_period_time = 0;
	m_period_cache_next = 0;
	m_period_cache_size = 0;
	m_cached_avg_period = 0;
}

void CEdt::AddWaitingPeriod()
{
	// avoid collecting stats when disconnected and reset when reconnected
    if (theApp.stat_serverConnectTime < m_last_server_time)
		m_last_period_time = 0;
	if (!theApp.stat_serverConnectTime)
		return;
	m_last_server_time = theApp.stat_serverConnectTime;

	// evaluate delta
	if (!m_last_period_time)
	{
		m_last_period_time = ::GetTickCount();
		return;
	}
	uint32 period = ::GetTickCount() - m_last_period_time;
	m_last_period_time = ::GetTickCount();

	// save statistic
	m_period[m_period_cache_next] = period;
	m_period_cache_next = (m_period_cache_next + 1) % EDT_PERIOD_CACHE;
	if (m_period_cache_size < EDT_PERIOD_CACHE) ++m_period_cache_size;

	// prepare average for use
	int i;
	m_cached_avg_period = 0;
	for (i = 0; i < m_period_cache_size; ++i) m_cached_avg_period += m_period[i];
	m_cached_avg_period /= m_period_cache_size;
}

float CEdt::WaitingPeriod(uint32 rate)
{
	return m_cached_avg_period * m_cache_totalrate / rate;
}

bool CEdt::EstimateWaitingPeriod(uint32 curr_waittime, uint32 rate, uint32 &est_time, uint32 &est_err)
{
	float est_safe;
	uint32 queue_places;

	if ((m_period_cache_size < EDT_MIN_PERIOD_CACHE) || !rate) return false;
	m_cache_totalrate = TotalRate();
	// TIME ESTIMATION
	try
	{
		queue_places = QueuePlacesBefore(WaitingPeriod(rate) - curr_waittime);
		est_safe = m_cached_avg_period * queue_places;
		est_safe /= 1000;
		if (est_safe < 0)
			est_time = 0;
		else if (est_safe > EDT_INFINITE_TIME)
			est_time = EDT_INFINITE_TIME;
		else
			est_time = est_safe;
	}
	catch(...)
	{
		return false;
	}
	// ERROR ESTIMATION
	try
	{
		est_safe = m_cached_avg_period * sqrt((float)queue_places);
		est_safe /= 1000;
		if (est_safe < 0)
			est_err = 0;
		else if (est_safe > EDT_INFINITE_ERR)
			est_err = EDT_INFINITE_ERR;
		else
			est_err = est_safe;
	}
	catch(...)
	{
		return false;
	}
	return true;
}

void CEdt::EstimateTime(CUpDownClient * client, uint32 &est_time, uint32 &est_err)
{
	uint32 curr_waittime = ::GetTickCount() - client->GetWaitStartTime();
	uint32 rate = client->GetScore(false,false,true);

	est_time = 0;
	est_err = EDT_UNDEFINED;
	if (EstimateWaitingPeriod(curr_waittime, rate, est_time, est_err)) return;
	est_time = 0;
	est_err = EDT_UNDEFINED;
}

CString CEdt::FormatEDT(CUpDownClient * client)
{
	return FormatEDT(client->GetDownloadTimeVal(), client->GetDownloadTimeErr(), client->GetDownloadTime());
}

CString CEdt::FormatEDT(uint32 avg_time, uint32 err_time, CTime start_time)
{
	CString Sbuffer;
	Sbuffer.Empty();

	Sbuffer += "EDT: ";
	if (err_time == EDT_UNDEFINED)
		Sbuffer += "undefined";
	else
	{
		if (avg_time == EDT_INFINITE_TIME)
			Sbuffer += "too long time";
		else if (err_time == EDT_INFINITE_ERR)
			Sbuffer += "very inaccurate";
		else if ((avg_time + err_time) < EDT_IMMINENT)
			Sbuffer += "imminent";
		else if (err_time < (avg_time/100))
		{
			if (avg_time < EDT_EDW_THRESHOLD)
				Sbuffer += start_time.Format("%H.%M");
			else
				Sbuffer += CastSecondsToHM(avg_time);
		}
		else if (err_time < avg_time)
		{
			if (avg_time < EDT_EDW_THRESHOLD)
			{
				CTime min_time = start_time - CTimeSpan(err_time);
				CTime max_time = start_time + CTimeSpan(err_time);
				Sbuffer += min_time.Format("%H.%M") + "-" + max_time.Format("%H.%M");
			}
			else
			{
				Sbuffer += CastSecondsToHM(avg_time-err_time) + " to " + CastSecondsToHM(avg_time+err_time);
			}
		}
		else
		{
			Sbuffer += "before ";
			if ((avg_time + err_time) < EDT_EDW_THRESHOLD)
			{
				CTime max_time = start_time + CTimeSpan(err_time);
				Sbuffer += max_time.Format("%H.%M");
			}
			else
			{
				Sbuffer += CastSecondsToHM(avg_time+err_time);
			}
		}
	}
	return Sbuffer;
}

uint32 CEdt::TotalRate()
{
	uint32 total_rate = 0;
	uint32 cur_rate;
	POSITION pos;
	for (pos = theApp.uploadqueue->waitinglist.GetHeadPosition();pos != 0;theApp.uploadqueue->waitinglist.GetNext(pos))
	{
		CUpDownClient* cur_client =	theApp.uploadqueue->waitinglist.GetAt(pos);
		cur_rate = cur_client->GetScore(false, false, true);
		total_rate += (cur_rate >= 0x0FFFFFFF ? 0 : cur_rate); // ignore friend slot
	}
	for (pos = theApp.uploadqueue->uploadinglist.GetHeadPosition();pos != 0;theApp.uploadqueue->uploadinglist.GetNext(pos))
	{
		CUpDownClient* cur_client =	theApp.uploadqueue->uploadinglist.GetAt(pos);
		cur_rate = cur_client->GetScore(false, false, true);
		total_rate += (cur_rate >= 0x0FFFFFFF ? 0 : cur_rate); // ignore friend slot
	}
	return total_rate;
}

uint32 CEdt::QueuePlacesBefore(float threshold_time)
{
	uint32 ret_place;
	ret_place = 0;
	float cur_time;
	float cur_period;
	uint32 sys_time = ::GetTickCount();
	uint32 slots = theApp.uploadqueue->uploadinglist.GetCount();
    POSITION pos;

	for (pos = theApp.uploadqueue->waitinglist.GetHeadPosition();pos != 0;theApp.uploadqueue->waitinglist.GetNext(pos))
	{
		CUpDownClient* cur_client =	theApp.uploadqueue->waitinglist.GetAt(pos);
		try
		{
			cur_period = WaitingPeriod(cur_client->GetScore(false, false, true));
			cur_time = cur_period - (sys_time - cur_client->GetWaitStartTime());
			cur_period += (slots * m_cached_avg_period);
            if (cur_time < threshold_time)
			{
				++ret_place;
				ret_place += (uint32)((threshold_time - cur_time) / cur_period);
			}
		}
		catch(...) {}
	}
	for (pos = theApp.uploadqueue->uploadinglist.GetHeadPosition();pos != 0;theApp.uploadqueue->uploadinglist.GetNext(pos))
	{
		CUpDownClient* cur_client =	theApp.uploadqueue->uploadinglist.GetAt(pos);
		try
		{
			cur_period = WaitingPeriod(cur_client->GetScore(false, false, true));
			cur_time = cur_period + (slots * m_cached_avg_period) - (sys_time - cur_client->m_dwUploadTime);
			cur_period += (slots * m_cached_avg_period);
            if (cur_time < threshold_time)
			{
				++ret_place;
				ret_place += (uint32)((threshold_time - cur_time) / cur_period);
			}
		}
		catch(...) {}
	}
	return ret_place;
}
