#include "stdafx.h"
#include "mdump.h"
#if _MSC_VER < 1300
#define DECLSPEC_DEPRECATED
// VC6: change this path to your Platform SDK headers
#include "M:\\dev7\\vs\\devtools\\common\\win32sdk\\include\\dbghelp.h"			// must be XP version of file
#else
// VC7: ships with updated headers
#include "dbghelp.h"
#endif

// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
										 CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
										 CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
										 CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
										);


LPCSTR MiniDumper::m_szAppName;

MiniDumper::MiniDumper( LPCSTR szAppName )
{
	// if this assert fires then you have two instances of MiniDumper
	// which is not allowed
	ASSERT( m_szAppName==NULL );

	m_szAppName = szAppName ? strdup(szAppName) : "Application";

	::SetUnhandledExceptionFilter( TopLevelFilter );
}
MiniDumper::~MiniDumper()
{
	delete[] m_szAppName;
}

LONG MiniDumper::TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo )
{
	LONG retval = EXCEPTION_CONTINUE_SEARCH;

	// firstly see if dbghelp.dll is around and has the function we need
	// look next to the EXE first, as the one in System32 might be old 
	// (e.g. Windows 2000)
	HMODULE hDll = NULL;
	char szDbgHelpPath[_MAX_PATH];

	if (GetModuleFileName( NULL, szDbgHelpPath, _MAX_PATH ))
	{
		char *pSlash = _tcsrchr( szDbgHelpPath, '\\' );
		if (pSlash)
		{
			_tcscpy( pSlash+1, "DBGHELP.DLL" );
			hDll = ::LoadLibrary( szDbgHelpPath );
		}
	}

	if (hDll==NULL)
	{
		// load any version we can
		hDll = ::LoadLibrary( "DBGHELP.DLL" );
	}

	LPCTSTR szResult = NULL;

	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			char szDumpPath[_MAX_PATH];
			char szScratch [_MAX_PATH];

			// work out a good place for the dump file
				::GetModuleFileName(0,szDumpPath, 490);
				LPTSTR pszFileName = _tcsrchr(szDumpPath, '\\') + 1;
				*pszFileName = '\0';
			//if (!GetTempPath( _MAX_PATH, szDumpPath ))
			//	_tcscpy( szDumpPath, "c:\\temp\\" );

			_tcscat( szDumpPath, m_szAppName );
			_tcscat( szDumpPath, ".dmp" );

			// ask the user if they want to save a dump file
			if (::MessageBox( NULL, "eMule crashed :-( A diagnostic file can be created which will help the author to resolve this problem.\n This file will be saved on your Disk (and not sent). Do you want to create this file now?", m_szAppName, MB_YESNO )==IDYES)
			{
				// create the file
				HANDLE hFile = ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
											FILE_ATTRIBUTE_NORMAL, NULL );

				if (hFile!=INVALID_HANDLE_VALUE)
				{
					_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

					ExInfo.ThreadId = ::GetCurrentThreadId();
					ExInfo.ExceptionPointers = pExceptionInfo;
					ExInfo.ClientPointers = NULL;

					// write the dump
					BOOL bOK = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL );
					if (bOK)
					{
						sprintf( szScratch, "Saved dump file to '%s'\nPlease send this file together with a bugreport to ornis@emuleserver.net\nThank you for helping to improve eMule", szDumpPath );
						szResult = szScratch;
						retval = EXCEPTION_EXECUTE_HANDLER;
					}
					else
					{
						sprintf( szScratch, "Failed to save dump file to '%s' (error %d)", szDumpPath, GetLastError() );
						szResult = szScratch;
					}
					::CloseHandle(hFile);
				}
				else
				{
					sprintf( szScratch, "Failed to create dump file '%s' (error %d)", szDumpPath, GetLastError() );
					szResult = szScratch;
				}
			}
		}
		else
		{
			szResult = "DBGHELP.DLL too old";
		}
	}
	else
	{
		szResult = "DBGHELP.DLL not found";
	}

	if (szResult)
		::MessageBox( NULL, szResult, m_szAppName, MB_OK );
	::ExitProcess(0);
	return retval;
}
