// stdafx.h : Includedatei für Standardsystem-Includedateien,
// oder häufig verwendete, projektspezifische Includedateien,
// die nur in unregelmäßigen Abständen geändert werden.

#pragma once

//TODO: To be removed and properly resolved in the sources!!
#pragma warning(disable:4200) // nonstandard extension used : zero-sized array in struct/union
#pragma warning(disable:4244) // 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4800) // 'type' : forcing value to bool 'true' or 'false' (performance warning)

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Selten verwendete Teile der Windows-Header nicht einbinden
#endif

// Ändern Sie folgende Definitionen für Plattformen, die älter als die unten angegebenen sind.
// Unter MSDN finden Sie die neuesten Informationen über die entsprechenden Werte für die unterschiedlichen Plattformen.

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

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// einige CString-Konstruktoren sind explizit

// Deaktiviert das Ausblenden von einigen häufigen und oft ignorierten Warnungen
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC-Kern- und -Standardkomponenten
#include <afxext.h>         // MFC-Erweiterungen
#include <afxdisp.h>        // MFC-Automatisierungsklassen

#include <afxdtctl.h>		// MFC-Unterstützung für allgemeine Steuerelemente von Internet Explorer 4
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC-Unterstützung für allgemeine Windows-Steuerelemente
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <winsock2.h>
#define _WINSOCKAPI_
#include <afxsock.h>		// MFC-Socket-Erweiterungen
#include <afxdhtml.h>

#include <afxmt.h>		//mutexes
#include <afxdlgs.h>
#include <..\src\mfc\afximpl.h>
#include <atlcoll.h>
#include "cpu_optimize.h" //eWombat [OnTheFly] Optimizer//Morph End - added by AndCycle, On the fly processor optimization(eWombat)

#define ARRSIZE(x)	(sizeof(x)/sizeof(x[0]))

#ifdef _DEBUG
#define malloc(s)         _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define calloc(c, s)      _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define realloc(p, s)     _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define _expand(p, s)     _expand_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p)           _free_dbg(p, _NORMAL_BLOCK)
#define _msize(p)         _msize_dbg(p, _NORMAL_BLOCK)
#endif
