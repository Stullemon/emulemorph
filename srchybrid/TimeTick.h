#if !defined(TIMETICKER_H)
#define TIMETICKER_H

// TimeTick.h : interface of the CTimeTick class
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright © 2001, Stefan Belopotocan, http://welcome.to/BeloSoft
//
/////////////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CTimeTick

class CTimeTick
{
	CTimeTick(const CTimeTick& d);
	CTimeTick& operator=(const CTimeTick& d);

public:	
	CTimeTick();
	~CTimeTick();

	// Operations
	void Start();
	float Tick();
	bool  isPerformanceCounter() {return m_nPerformanceFrequency;}

	// Implementation
protected:
	static __int64 GetPerformanceFrequency();
	static float GetTimeInMilliSeconds(__int64 nTime);

	// Data
private:
	static __int64 m_nPerformanceFrequency;

	LARGE_INTEGER m_nTimeElapsed;
	LARGE_INTEGER m_nTime;
};

#endif // !defined(TIMETICKER_H)