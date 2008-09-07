// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <emule_site_config.h>

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#if _MSC_VER >= 1500   //vs2008?
#ifndef WINVER
#define WINVER 0x0501		// at least xp ... for vs2008. not good but.... compil
#endif
#else

#ifndef WINVER
#define WINVER 0x0400			// 0x0400 == Windows 98 and Windows NT 4.0 (because of '_WIN32_WINDOWS=0x0410')
#endif

#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NN or later.                   
#define _WIN32_WINNT WINVER	// Change this to the appropriate value.
#endif

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows NN or later.
#define _WIN32_WINDOWS WINVER   // Change this to the appropriate value.
#endif

#ifndef _WIN32_IE
//#define _WIN32_IE 0x0400		// 0x0400 == Internet Explorer 4.0 -> Comctl32.dll v4.71
#define _WIN32_IE 0x0560		// 0x0560 == Internet Explorer 5.6 -> Comctl32.dll v5.8 (same as MFC internally used)
#endif

// netfinity: Backward compability of new VC++ 2005 features
#if _MSC_VER < 1400
#define nullptr reinterpret_cast<void*>(0)
#endif


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit
#define _ATL_ALL_WARNINGS
#define _AFX_ALL_WARNINGS
// Disable some warnings which get fired with /W4 for Windows/MFC/ATL headers
#pragma warning(disable:4127) // conditional expression is constant




// Disable some warnings which are only generated when using "/Wall"
#pragma warning(disable:4061) // enumerate in switch of enum is not explicitly handled by a case label
#pragma warning(disable:4062) // enumerate in switch of enum is not handled
#pragma warning(disable:4191) // 'type cast' : unsafe conversion from <this> to <that>
#pragma warning(disable:4217) // <func>: member template functions cannot be used for copy-assignment or copy-construction
#pragma warning(disable:4263) // <func> member function does not override any base class virtual member function
#pragma warning(disable:4264) // <func>: no override available for virtual member function from base <class>; function is hidden
#pragma warning(disable:4265) // <class>: class has virtual functions, but destructor is not virtual
#pragma warning(disable:4529) // forming a pointer-to-member requires explicit use of the address-of operator ('&') and a qualified name
#pragma warning(disable:4548) // expression before comma has no effect; expected expression with side-effect
#pragma warning(disable:4555) // expression has no effect; expected expression with side-effect
#pragma warning(disable:4619) // #pragma warning : there is no warning number <n>
#pragma warning(disable:4625) // <class> : copy constructor could not be generated because a base class copy constructor is inaccessible
#pragma warning(disable:4626) // <class> : assignment operator could not be generated because a base class copy constructor is inaccessible
#pragma warning(disable:4640) // construction of local static object is not thread-safe
#pragma warning(disable:4668) // <name>  is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#pragma warning(disable:4710) // function not inlined
#pragma warning(disable:4711) // function <func> selected for automatic inline expansion
#pragma warning(disable:4820) // <n> bytes padding added after member <member>
#pragma warning(disable:4917) // a GUID can only be associated with a class, interface or namespace
#pragma warning(disable:4928) // illegal copy-initialization; more than one user-defined conversion has been implicitly applied

#if _MSC_VER>=1400

// _CRT_SECURE_NO_DEPRECATE - Disable all warnings for not using "_s" functions.
//
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

// _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES - Overloads all standard string functions (e.g. strcpy) with "_s" functions
// if, and only if, the size of the output buffer is known at compile time (so, if it is a static array). If there is
// a buffer overflow during runtime, it will throw an exception.
//
#ifndef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#endif

// _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT - This is a cool CRT feature but does not make sense for our code.
// With our existing code we could get exceptions which are though not justifiable because we explicitly
// terminate all our string buffers. This define could be enabled for debug builds though.
//
//#ifndef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT
//#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT 1
//#endif

#if !defined(_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES) || (_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES==0)
#ifndef _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_NON_CONFORMING_SWPRINTFS
#endif
#endif//!defined(_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES) || (_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES==0)

#ifndef _USE_32BIT_TIME_T
#define _USE_32BIT_TIME_T
#endif

#endif//_MSC_VER>=1400

#ifdef _DEBUG
#define _ATL_DEBUG
#define _ATL_DEBUG_QI
#endif

#include <afxwin.h>			// MFC core and standard components
#include <afxext.h>			// MFC extensions
#include <afxdisp.h>		// MFC IDispatch & ClassFactory support

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxole.h>			// MFC OLE support

#include <winsock2.h>
#define _WINSOCKAPI_
#include <afxsock.h>		// MFC support for Windows Sockets
#include <afxsock.h>		// MFC socket extensions

#include <afxrich.h>		// MFC rich edit classes

#include <atlbase.h>
extern CComModule _Module;

#include <afxdhtml.h>


#include <afxmt.h>			// MFC Multithreaded Extensions (Syncronization Objects)
#include <afxdlgs.h>		// MFC Standard dialogs
#include <..\src\mfc\afximpl.h>
#include <atlcoll.h>
#include <afxcoll.h>
#include <afxtempl.h>
#include <math.h>


#ifndef EWX_FORCEIFHUNG
#define EWX_FORCEIFHUNG			0x00000010
#endif

#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL			0x00400000L // Right to left mirroring
#endif

#ifndef LAYOUT_RTL
#define LAYOUT_RTL				0x00000001 // Right to left
#endif

#ifndef COLOR_HOTLIGHT
#define COLOR_HOTLIGHT			26
#endif

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED			0x00080000
#endif

#ifndef LWA_COLORKEY
#define LWA_COLORKEY			0x00000001
#endif

#ifndef LWA_ALPHA
#define LWA_ALPHA				0x00000002
#endif

#ifndef HDF_SORTUP
#define HDF_SORTUP				0x0400
#endif

#ifndef HDF_SORTDOWN
#define HDF_SORTDOWN			0x0200
#endif

#ifndef COLOR_GRADIENTACTIVECAPTION
#define COLOR_GRADIENTACTIVECAPTION 27
#endif



#include "types.h"

/*
#if _MSC_VER<=1400
// Enable warnings which were disabled for Windows/MFC/ATL headers
#pragma warning(default:4127) // conditional expression is constant
#pragma warning(default:4548) // expression before comma has no effect; expected expression with side-effect
#if _MSC_VER==1310
#pragma warning(default:4555) // expression has no effect; expected expression with side-effect
#endif
#endif
*/

// when using warning level 4
#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union (not worth to mess with, it's due to MIDL created code)
#pragma warning(disable:4238) // nonstandard extension used : class rvalue used as lvalue
#if _MSC_VER>=1400
#pragma warning(disable:4127) // conditional expression is constant
#endif


#define ARRSIZE(x)	(sizeof(x)/sizeof(x[0]))

//RM => OMG
#pragma warning (disable:4996)
#pragma warning (disable:4238)
//RM <= OMG

#if _MSC_VER<1400
#define malloc(s)		  _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define calloc(c, s)	  _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define realloc(p, s)	  _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define _expand(p, s)	  _expand_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p)			  _free_dbg(p, _NORMAL_BLOCK)
#define _msize(p)		  _msize_dbg(p, _NORMAL_BLOCK)
#endif

typedef CArray<CStringA> CStringAArray;
typedef CStringArray CStringWArray;

#define _TWINAPI(fname)	fname "W"

extern "C" int __cdecl __ascii_stricmp(const char * dst, const char * src);

#if _MSC_VER>=1400
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
#endif

inline BOOL afxIsWin95()
{
#if _MFC_VER>=0x0900
	return FALSE;
#else
	return afxData.bWin95;
#endif
}
