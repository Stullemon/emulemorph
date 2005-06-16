// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

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
//#define _WIN32_IE 0x0400		// 0x0400 == Internet Explorer 4.0 -> Comctl32.dll v4.71
#define _WIN32_IE 0x0560		// 0x0560 == Internet Explorer 5.6 -> Comctl32.dll v5.8 (same as MFC internally used)
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#define _ATL_ALL_WARNINGS
#define _AFX_ALL_WARNINGS
// Disable some warnings which get fired with /W4 for Windows/MFC/ATL headers
#pragma warning(disable:4127) // conditional expression is constant

#ifdef _DEBUG
#define	_ATL_DEBUG
#define _ATL_DEBUG_QI
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC IDispatch & ClassFactory support
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxole.h>			// MFC OLE support

#include <winsock2.h>
#define _WINSOCKAPI_
#include <afxsock.h>		// MFC support for Windows Sockets
#include <afxdhtml.h>

#include <afxmt.h>			// MFC Multithreaded Extensions (Syncronization Objects)
#include <afxdlgs.h>		// MFC Standard dialogs
#include <..\src\mfc\afximpl.h>
#include <atlcoll.h>
#include <afxcoll.h>
#include <afxtempl.h>


#ifndef EWX_FORCEIFHUNG
#define EWX_FORCEIFHUNG			0x00000010
#endif

#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring
#endif

#ifndef LAYOUT_RTL
#define LAYOUT_RTL				0x00000001 // Right to left
#endif

#ifndef COLOR_HOTLIGHT
#define COLOR_HOTLIGHT			26
#endif

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED           0x00080000
#endif

#ifndef LWA_COLORKEY
#define LWA_COLORKEY            0x00000001
#endif

#ifndef LWA_ALPHA
#define LWA_ALPHA               0x00000002
#endif

#ifndef HDF_SORTUP
#define HDF_SORTUP              0x0400
#endif

#ifndef HDF_SORTDOWN
#define HDF_SORTDOWN            0x0200
#endif



// Enable warnings which were disabled for Windows/MFC/ATL headers
#pragma warning(default:4127) // conditional expression is constant

// when using warning level 4
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union (not worth to mess with, it's due to MIDL created code)
#pragma warning(disable:4238) // nonstandard extension used : class rvalue used as lvalue
#pragma warning(disable:4244) // conversion from 'type1' to 'type2', possible loss of data
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

typedef	CArray<CStringA> CStringAArray;
typedef	CStringArray CStringWArray;

#define _TWINAPI(fname)	fname "W"

extern "C" int __cdecl __ascii_stricmp(const char * dst, const char * src);

//MORPH START - Added by SiRoB, Optimizer inspired from espania
//#include <wchar.h>
#include ".\Optimizer\Optimize.h" // Commander - Added: Optimizer [ePlus]
#define memcpy(a, b, c)	memcpy_optimized(a, b, c)
#define memset(a, b, c) memset_optimized(a, b, c)
#define memzero(a, b) memzero_optimized(a, b)
//MORPH END   - Added by SiRoB, Optimizer inspired from espania