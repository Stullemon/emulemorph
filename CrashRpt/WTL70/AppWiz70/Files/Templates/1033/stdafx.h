// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

// Change these values to use different versions
[!if WTL_COM_SERVER]
#define WINVER		0x0400
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400
#define _RICHEDIT_VER	0x0100

#define _ATL_APARTMENT_THREADED

[!else]
#define WINVER		0x0400
//#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400
#define _RICHEDIT_VER	0x0100

[!endif]
#include <atlbase.h>
#include <atlapp.h>

[!if WTL_COM_SERVER]
extern CServerAppModule _Module;

// This is here only to tell VC7 Class Wizard this is an ATL project
#ifdef ___VC7_CLWIZ_ONLY___
CComModule
CExeModule
#endif

[!else]
extern CAppModule _Module;

[!endif]
[!if WTL_ENABLE_AX || WTL_COM_SERVER]
#include <atlcom.h>
[!endif]
[!if WTL_ENABLE_AX]
#include <atlhost.h>
[!endif]
#include <atlwin.h>
[!if WTL_ENABLE_AX]
#include <atlctl.h>
[!endif]
[!if WTL_USE_CPP_FILES]

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
[!if WTL_USE_CMDBAR]
#include <atlctrlw.h>
[!endif]
[!endif]
