// enkeyDev(th1) -EDT- Estimation of Download Time

#include "stdafx.h"
#include "opcodes.h"
#include "types.h"

#pragma once

class CEdt
{
private:

	// Waiting period estimation
	uint32	m_period[EDT_PERIOD_CACHE];
	int		m_period_cache_next;
	int		m_period_cache_size;

	uint32	m_last_server_time;
	uint32	m_last_period_time;

	float	m_cached_avg_period;

	float	m_cache_totalrate;

	float WaitingPeriod(uint32 rate);
	uint32 TotalRate();
	uint32 QueuePlacesBefore(float threshold_time);
	bool EstimateWaitingPeriod(uint32 curr_waittime, uint32 rate, uint32 &est_time, uint32 &est_err);

public:

	CEdt();
	void ResetWaitingPeriodCache();
	void AddWaitingPeriod();
	void EstimateTime(CUpDownClient * client, uint32 &est_time, uint32 &est_err);
	CString FormatEDT(uint32 avg_time, uint32 err_time, CTime start_time);
	CString FormatEDT(CUpDownClient * client);
};

