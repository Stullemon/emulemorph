#ifndef __DFLT_EXCEPTION_HANDLERS_H__
#define __DFLT_EXCEPTION_HANDLERS_H__

#pragma once

#ifdef _DEBUG
#define	CATCH_DFLT_ALL(fname)
#else
#define	CATCH_DFLT_ALL(fname) \
	catch(...){ \
		if (thePrefs.GetVerbose() && theApp.emuledlg) \
			theApp.emuledlg->AddDebugLogLine(false, _T("Unknown exception in ") fname); \
		ASSERT(0); \
	}
#endif

// This type of "last chance" exception handling is to be used at least in several callback functions to avoid memory leaks.
// It is *not* thought as a proper handling of exceptions in general! 
// -> Use explicit exception handlers where needed!
#define CATCH_DFLT_EXCEPTIONS(fname) \
	catch(CException* e){ \
		TCHAR szError[1024]; \
		e->GetErrorMessage(szError, ARRSIZE(szError)); \
		const CRuntimeClass* pRuntimeClass = e->GetRuntimeClass(); \
		LPCTSTR pszClassName = (pRuntimeClass) ? pRuntimeClass->m_lpszClassName : NULL; \
		if (!pszClassName) \
			pszClassName = _T("CException"); \
		if (thePrefs.GetVerbose() && theApp.emuledlg) \
			theApp.emuledlg->AddDebugLogLine(false, _T("Unknown %s exception in ") fname _T(" - %s"), pszClassName, szError); \
		e->Delete(); \
	} \
	catch(CString strError){ \
		if (thePrefs.GetVerbose() && theApp.emuledlg) \
			theApp.emuledlg->AddDebugLogLine(false, _T("Unknown CString exception in ") fname _T(" - %s"), strError); \
	}

#endif//!__DFLT_EXCEPTION_HANDLERS_H__
