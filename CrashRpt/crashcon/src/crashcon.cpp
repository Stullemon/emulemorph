// crashcon.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <process.h>

#ifndef _CRASH_RPT_
#include "../../crashrpt/include/crashrptDL.h"
//#pragma comment(lib, "../../crashrpt/lib/crashrpt.lib")
#endif

int main(int argc, char* argv[])
{
	HMODULE CrashRptDLL = GetInstanceDL();
	if(InstallDL(CrashRptDLL, NULL, NULL, "Message")) {
		printf("CrashRpt available\n");
	} else {
		printf("CrashRpt NOT available\n");
	}

#ifdef _DEBUG
   printf("Press a ENTER to simulate a null pointer exception...\n");
   getchar();
/*   __try {
		__asm {
			int 0x3
		}
//      RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
   } __except(GenerateErrorReportEx(lpvState, GetExceptionInformation(), NULL), 1){}*/
   GenerateErrorReportDL(CrashRptDLL, NULL, NULL);
#else
   printf("Press a ENTER to generate a null pointer exception...\n");
   getchar();
   int *p = 0;
   *p = 0;
#endif // _DEBUG
   printf("should not reach this...\n");
   getchar();
	return 0;
}

