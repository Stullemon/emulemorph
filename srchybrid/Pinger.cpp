/*---------------------------------------------------------------------
*
*  Some code in this file has been copied from a ping demo that was
*  created by Bob Quinn, 1997          http://www.sockets.com
*
* As some general documenation about how the ping is implemented,
* here is the description Bob Quinn wrote about the ping demo.
*
* <--- Cut --->
*
* Description:
*  This is a ping program that uses Microsoft's ICMP.DLL functions 
*  for access to Internet Control Message Protocol.  This is capable
*  of doing "ping or "traceroute", although beware that Microsoft 
*  discourages the use of these APIs.
*
*  Tested with MSVC 5 compile with "cl ms_icmp.c /link ws2_32.lib"
*  from the console (if you've run VCVARS32.BAT batch file that
*  ships with MSVC to set the proper environment variables)
*
* NOTES:
* - With both "Don't Fragment" and "Router Alert" set, the 
*    IP don't fragment bit is set, but not the router alert option.
* - The ICMP.DLL calls are not redirected to the TCP/IP service
*    provider (what's interesting about this is that the setsockopt()
*    IP_OPTION flag can't do Router Alert, but this API can ...hmmm.
* - Although the IcmpSendEcho() docs say it can return multiple
*    responses, if I receive multiple responses (e.g. sending to
*    a limited or subnet broadcast address) IcmpSendEcho() only
*    returns one.  Interesting that NT4 and Win98 don't respond
*    to broadcast pings.
* - using winsock.h  WSOCK32.LIB and version 1.1 works as well as 
*    using winsock2.h WS2_32.LIB  and version 2.2
*
* Some Background:
*
* The standard Berkeley Sockets SOCK_RAW socket type, is normally used
* to create ping (echo request/reply), and sometimes traceroute applications
* (the original traceroute application from Van Jacobson used UDP, rather
* than ICMP). Microsoft's WinSock version 2 implementations for NT4 and 
* Windows 95 support raw sockets, but none of their WinSock version 1.1
* implementations (WFWG, NT3.x or standard Windows 95) did.
*
* Microsoft has their own API for an ICMP.DLL that their ping and tracert
* applications use (by the way, they are both non-GUI text-based console
* applications. This is a proprietary API, and all function calls that 
* involve network functions operate in blocking mode. They still include 
* it with WinSock 2 implementations.
*
* There is little documentation available (I first found it in the Win32
* SDK in \MSTOOLS\ICMP, and it exists on the MS&nbsp;Developers' Network
* CD-ROM now, also). Microsoft disclaims this API about as strongly as 
* possible.  The README.TXT that accompanies it says:
*
* [DISCLAIMER]
* 
* We have had requests in the past to expose the functions exported from
* icmp.dll. The files in this directory are provided for your convenience
* in building applications which make use of ICMPSendEcho(). Notice that
* the functions in icmp.dll are not considered part of the Win32 API and
* will not be supported in future releases. Once we have a more complete
* solution in the operating system, this DLL, and the functions it exports,
* will be dropped.    
*      
* [DOCUMENTATION]     
*
* The ICMPSendEcho() function sends an ICMP echo request to the specified
* destination IP address and returns any replies received within the timeout
* specified. The API is synchronous, requiring the process to spawn a thread
* before calling the API to avoid blocking. An open IcmpHandle is required
* for the request to complete. IcmpCreateFile() and IcmpCloseHandle() 
* functions are used to create and destroy the context handle.
*
* <--- End cut --->
*/

#include "stdafx.h"

#include "Pinger.h"
#include "Emule.h"

#define BUFSIZE     8192
#define DEFAULT_LEN 0
#define TIMEOUT     3000

/*---------------------------------------------------------
 * IcmpSendEcho() Error Strings
 * 
 * The values in the status word returned in the ICMP Echo 
 *  Reply buffer after calling IcmpSendEcho() all have a
 *  base value of 11000 (IP_STATUS_BASE).  At times,
 *  when IcmpSendEcho() fails outright, GetLastError() will 
 *  subsequently return these error values also.
 *
 * Two Errors value defined in ms_icmp.h are missing from 
 *  this string table (just to simplify use of the table):
 *    "IP_GENERAL_FAILURE (11050)"
 *    "IP_PENDING (11255)"
 */
#define MAX_ICMP_ERR_STRING  IP_STATUS_BASE + 22
char *aszSendEchoErr[] = {
    "IP_STATUS_BASE (11000)",
    "IP_BUF_TOO_SMALL (11001)",
    "IP_DEST_NET_UNREACHABLE (11002)",
    "IP_DEST_HOST_UNREACHABLE (11003)",
    "IP_DEST_PROT_UNREACHABLE (11004)",
    "IP_DEST_PORT_UNREACHABLE (11005)",
    "IP_NO_RESOURCES (11006)",
    "IP_BAD_OPTION (11007)",
    "IP_HW_ERROR (11008)",
    "IP_PACKET_TOO_BIG (11009)",
    "IP_REQ_TIMED_OUT (11010)",
    "IP_BAD_REQ (11011)",
    "IP_BAD_ROUTE (11012)",
    "IP_TTL_EXPIRED_TRANSIT (11013)",
    "IP_TTL_EXPIRED_REASSEM (11014)",
    "IP_PARAM_PROBLEM (11015)",
    "IP_SOURCE_QUENCH (11016)",
    "IP_OPTION_TOO_BIG (11017)",
    "IP_BAD_DESTINATION (11018)",
    "IP_ADDR_DELETED (11019)",
    "IP_SPEC_MTU_CHANGE (11020)",
    "IP_MTU_CHANGE (11021)",
    "IP_UNLOAD (11022)"
};

Pinger::Pinger() {
    WSADATA wsaData;

    int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (nRet) {
        theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSPINGERWSASFAI), nRet);
        return;
    }

    // Open ICMP.DLL
    hICMP_DLL = LoadLibrary("ICMP.DLL");
    if (hICMP_DLL == 0) {
        theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSPINGERLOADLIB));
        return;
    }

    // Get pointers to ICMP.DLL functions
    lpfnIcmpCreateFile  = (IcmpCreateFile*)GetProcAddress(hICMP_DLL,"IcmpCreateFile");
    lpfnIcmpCloseHandle = (IcmpCloseHandle*)GetProcAddress(hICMP_DLL,"IcmpCloseHandle");
    lpfnIcmpSendEcho    = (IcmpSendEcho*)GetProcAddress(hICMP_DLL,"IcmpSendEcho");
    if ((!lpfnIcmpCreateFile) || 
        (!lpfnIcmpCloseHandle) || 
        (!lpfnIcmpSendEcho)) {

        theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSPINGERGPAFAIL));
        return;
    }

    // Open the ping service
    hICMP = (HANDLE) lpfnIcmpCreateFile();
    if (hICMP == INVALID_HANDLE_VALUE) {
        theApp.emuledlg->QueueDebugLogLine(false, GetResString(IDS_USSPINGERICFFAIL));
        PIcmpErr(GetLastError());
        return;
    }

    // Init IPInfo structure
    stIPInfo.Tos      = 0;
    stIPInfo.Flags    = 0;
    stIPInfo.OptionsSize = 0;
    stIPInfo.OptionsData = NULL;
}

Pinger::~Pinger() {
    // Close the ICMP handle
    BOOL fRet = lpfnIcmpCloseHandle(hICMP);
    if (fRet == FALSE) {
        int nErr = GetLastError();

        theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSPINGERICMPENDERR));
        PIcmpErr(GetLastError());
    }

    // Shut down...
    FreeLibrary(hICMP_DLL);

    WSACleanup();
}

PingStatus Pinger::Ping(uint32 lAddr, uint32 ttl, bool doLog) {
    PingStatus returnValue;
    IN_ADDR stDestAddr;
    char achRepData[sizeof(ICMPECHO) + BUFSIZE];

    // Address is assumed to be ok
    stDestAddr.s_addr = lAddr;
    stIPInfo.Ttl = ttl;

    // Send the ICMP Echo Request and read the Reply
    DWORD dwReplyCount = lpfnIcmpSendEcho(hICMP, 
                                    stDestAddr.s_addr,
                                    0, // databuffer
                                    0, // DataLen, length of databuffer
                                    &stIPInfo, 
                                    achRepData, 
                                    sizeof(achRepData), 
                                    TIMEOUT
                         );

    if (dwReplyCount != 0) {
        IN_ADDR stDestAddr;

        stDestAddr.s_addr = *(u_long *)achRepData;

        returnValue.success = true;
        returnValue.status = *(DWORD *) &(achRepData[4]);
        returnValue.delay = *(u_long *) &(achRepData[8]);
        returnValue.destinationAddress = stDestAddr.s_addr;
        returnValue.ttl = (returnValue.status != IP_SUCCESS)?ttl:(*(char *)&(achRepData[20]))&0x00FF;

        if(doLog) {
            theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSPINGERPINGREPLY),
                                                        inet_ntoa(stDestAddr),
                                                        *(u_long *) &(achRepData[12]),
                                                        returnValue.delay, 
                                                        returnValue.ttl);
        }
    } else {
        DWORD lastError = GetLastError();
        returnValue.success = false;
        returnValue.error = lastError;
    }

    return returnValue;
}


void Pinger::PIcmpErr(int nICMPErr) {
    int  nErrIndex = nICMPErr - IP_STATUS_BASE;

    if ((nICMPErr > MAX_ICMP_ERR_STRING) || 
        (nICMPErr < IP_STATUS_BASE+1)) {

        // Error value is out of range, display normally
        theApp.emuledlg->QueueDebugLogLine(false,"(%d) ", nICMPErr);
        DisplayErr(nICMPErr);
    } else {

        // Display ICMP Error String
        theApp.emuledlg->QueueDebugLogLine(false,"%s", aszSendEchoErr[nErrIndex]);
    }
}

void Pinger::DisplayErr(int nWSAErr) {
    char* lpMsgBuf; // Pending: was LPVOID

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        nWSAErr,
        MAKELANGID(LANG_NEUTRAL, 
        SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,    
        NULL );   
    theApp.emuledlg->QueueDebugLogLine(false,lpMsgBuf);

    // Free the buffer
    LocalFree(lpMsgBuf);
}

