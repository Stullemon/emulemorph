// CrashRptTest.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "CrashRptTest.h"
#include "CrashRptTestDlg.h"
#include <direct.h>

#ifndef _CRASH_RPT_
#include "../../crashrpt/src/crashrpt.h"
#pragma comment(lib, "../../crashrpt/lib/crashrpt")
#endif // _CRASH_RPT_

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCrashRptTestApp

BEGIN_MESSAGE_MAP(CCrashRptTestApp, CWinApp)
	//{{AFX_MSG_MAP(CCrashRptTestApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCrashRptTestApp construction

CCrashRptTestApp::CCrashRptTestApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CCrashRptTestApp object

CCrashRptTestApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CCrashRptTestApp initialization


BOOL WINAPI CrashCallback(LPVOID lpvState)
{
   AddFileEx(lpvState, "dummy.log", "Dummy Log File");
   AddFileEx(lpvState, "dummy.ini", "Dummy INI File");

   return TRUE;
}

BOOL CCrashRptTestApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

   m_lpvState = InstallEx(CrashCallback, NULL, NULL);

   CCrashRptTestDlg dlg;
   dlg.DoModal();

   return FALSE;
}

void CCrashRptTestApp::generateErrorReport()
{
    GenerateErrorReportEx(m_lpvState, NULL, NULL);
//   __try {
//      RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
//   } __except(GenerateErrorReport(m_lpvState, GetExceptionInformation())){}
}