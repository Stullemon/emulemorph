# Microsoft Developer Studio Project File - Name="upnpLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=upnpLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "upnpLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "upnpLib.mak" CFG="upnpLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "upnpLib - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "upnpLib - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "upnpLib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "UPNPLIB_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "C:\upnpDev\libupnp-1.2.1\upnp\src\inc" /I "C:\upnpDev\libupnp-1.2.1\upnp\inc" /I "C:\pThreads\PTHREADS\pthreads" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "UPNPLIB_EXPORTS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib pthreadVCE.lib /nologo /dll /machine:I386 /libpath:"C:\pThreads\PTHREADS\pthreads"

!ELSEIF  "$(CFG)" == "upnpLib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "UPNPLIB_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "C:\upnpDev\libupnp-1.2.1\ixml\src\inc" /I "C:\upnpDev\libupnp-1.2.1\upnp\inc" /I "C:\upnpDev\libupnp-1.2.1\upnp\src\inc" /I "C:\pThreads\PTHREADS\pthreads" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "UPNPLIB_EXPORTS" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib pthreadVCE.lib ws2_32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"C:\pThreads\PTHREADS\pthreads"

!ENDIF 

# Begin Target

# Name "upnpLib - Win32 Release"
# Name "upnpLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "ixml"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\ixml\src\attr.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\document.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\element.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\ixml.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\ixmlmembuf.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\ixmlparser.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\namedNodeMap.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\node.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\nodeList.c
# End Source File
# End Group
# Begin Group "threadutil"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\threadutil\src\FreeList.c
# End Source File
# Begin Source File

SOURCE=..\threadutil\src\iasnprintf.c
# End Source File
# Begin Source File

SOURCE=..\threadutil\src\LinkedList.c
# End Source File
# Begin Source File

SOURCE=..\threadutil\src\ThreadPool.c
# End Source File
# Begin Source File

SOURCE=..\threadutil\src\TimerThread.c
# End Source File
# End Group
# Begin Group "upnp"

# PROP Default_Filter ".c"
# Begin Group "api"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\upnp\src\api\config.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\api\upnpapi.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\api\upnptools.c
# End Source File
# End Group
# Begin Group "gena"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\upnp\src\gena\gena_callback2.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\gena\gena_ctrlpt.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\gena\gena_device.c
# End Source File
# End Group
# Begin Group "genlib"

# PROP Default_Filter ".c"
# Begin Group "client_table"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\upnp\src\genlib\client_table\client_table.c
# End Source File
# End Group
# Begin Group "miniserver"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\upnp\src\genlib\miniserver\miniserver.c
# End Source File
# End Group
# Begin Group "net"

# PROP Default_Filter ".c"
# Begin Group "http"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\upnp\src\genlib\net\http\httpparser.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\net\http\httpreadwrite.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\net\http\parsetools.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\net\http\statcodes.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\net\http\webserver.c
# End Source File
# End Group
# Begin Group "uri"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\upnp\src\genlib\net\uri\uri.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\upnp\src\genlib\net\sock.c
# End Source File
# End Group
# Begin Group "service_table"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\upnp\src\genlib\service_table\service_table.c
# End Source File
# End Group
# Begin Group "util"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\upnp\src\genlib\util\membuffer.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\util\strintmap.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\util\upnp_timeout.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\util\util.c
# End Source File
# End Group
# End Group
# Begin Group "soap"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\upnp\src\soap\soap_ctrlpt.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\soap\soap_device.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\soap\soaplib.h
# End Source File
# End Group
# Begin Group "ssdp"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\upnp\src\ssdp\ssdp_ctrlpt.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\ssdp\ssdp_device.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\ssdp\ssdp_server.c
# End Source File
# End Group
# Begin Group "urlconfig"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\upnp\src\urlconfig\urlconfig.c
# End Source File
# End Group
# Begin Group "uuid"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\upnp\src\uuid\md5.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\uuid\sysdep.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\uuid\uuid.c
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=.\upnpLib.cpp

!IF  "$(CFG)" == "upnpLib - Win32 Release"

# ADD CPP /I "C:\upnpDev\libupnp-1.2.1\ixml\src\inc"

!ELSEIF  "$(CFG)" == "upnpLib - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
