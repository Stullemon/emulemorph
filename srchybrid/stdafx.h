// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifndef WINVER
#define WINVER 0x0400			// 0x0400 == Windows 98 and Windows NT 4.0 (because of '_WIN32_WINDOWS=0x0410')
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400		// 0x0400 == Windows NT 4.0
#endif						

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410   // 0x0410 == Windows 98
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0400		// 0x0400 == Internet Explorer 4.0 -> Comctl32.dll v4.71
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit
#define _ATL_ALL_WARNINGS
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC IDispatch & ClassFactory support
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxole.h>			// MFC OLE support

//<<< eWombat [WINSOCK2]
#include "system\afxsock.h"		// MFC-Socket-Erweiterungen
//>>> eWombat [WINSOCK2]
#include <afxdhtml.h>

#include <afxmt.h>			// MFC Multithreaded Extensions (Syncronization Objects)
#include <afxdlgs.h>		// MFC Standard dialogs
#include <..\src\mfc\afximpl.h>
#include <atlcoll.h>
#include <afxcoll.h>
#include <afxtempl.h>

//TODO: To be removed and properly resolved in the sources!!
#pragma warning(disable:4200) // nonstandard extension used : zero-sized array in struct/union
#pragma warning(disable:4244) // 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4800) // 'type' : forcing value to bool 'true' or 'false' (performance warning)

// whenn using warning level 4
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4238) // nonstandard extension used : class rvalue used as lvalue
#pragma warning(disable:4211) // nonstandard extension used : redefined extern to static
#pragma warning(disable:4389) // signed/unsigned mismatch

#include "types.h"

#define ARRSIZE(x)	(sizeof(x)/sizeof(x[0]))

#ifdef _DEBUG
#define malloc(s)         _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define calloc(c, s)      _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define realloc(p, s)     _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define _expand(p, s)     _expand_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p)           _free_dbg(p, _NORMAL_BLOCK)
#define _msize(p)         _msize_dbg(p, _NORMAL_BLOCK)
#endif

#ifdef _UNICODE
typedef	CArray<CStringA,CStringA> CStringAArray;
#else
typedef	CStringArray CStringAArray;
#endif

#ifdef _UNICODE
#define _TWINAPI(fname)	fname "W"
#else
#define _TWINAPI(fname)	fname "A"
#endif

extern "C" int __cdecl __ascii_stricmp(const char * dst, const char * src);

//MORPH START - Added by SiRoB, Optimizer inspired from espania
#include <wchar.h>
#include ".\Optimizer\Optimize.h" // Commander - Added: Optimizer [ePlus]
#define memcpy(a, b, c)	memcpy_optimized(a, b, c)
#define memset(a, b, c) memset_optimized(a, b, c)
#define memzero(a, b) memzero_optimized(a, b)
//MORPH END   - Added by SiRoB, Optimizer inspired from espania