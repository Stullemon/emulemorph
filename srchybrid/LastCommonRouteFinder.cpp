//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include "emule.h"
#include "Opcodes.h"
#include "LastCommonRouteFinder.h"
#include "Server.h"
#include "OtherFunctions.h"
#include "UpDownClient.h"
#include "Preferences.h"
#include "Pinger.h"
#ifndef _CONSOLE
#include "emuledlg.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


LastCommonRouteFinder::LastCommonRouteFinder() {
    minUpload = 1;
    maxUpload = _UI32_MAX;
    m_upload = _UI32_MAX;
    m_CurUpload = 1;

    m_iPingToleranceMilliseconds = 200;
    m_bUseMillisecondPingTolerance = false;
    m_iNumberOfPingsForAverage = 0;
    m_pingAverage = 0;
    m_lowestPing = 0;
    m_LowestInitialPingAllowed = 20;
    pingDelaysTotal = 0;

    m_state = "";

    needMoreHosts = false;

    threadEndedEvent = new CEvent(0, 1);
    newTraceRouteHostEvent = new CEvent(0, 0);
    prefsEvent = new CEvent(0, 0);

    m_enabled = false;
    doRun = true;
    AfxBeginThread(RunProc, (LPVOID)this);
}

LastCommonRouteFinder::~LastCommonRouteFinder() {
    delete threadEndedEvent;
    delete newTraceRouteHostEvent;
    delete prefsEvent;
}

bool LastCommonRouteFinder::AddHostsToCheck(CTypedPtrList<CPtrList, CServer*> &list) {
    if(needMoreHosts) {
        addHostLocker.Lock();
        
        if(needMoreHosts) {
            if(list.GetCount() >= 10) {
                hostsToTraceRoute.RemoveAll();

                uint32 startPos = rand()/(RAND_MAX/list.GetCount());

                POSITION pos = list.GetHeadPosition();
                for(uint32 skipCounter = startPos; skipCounter < (uint32)list.GetCount() && pos != NULL; skipCounter++) {
                    list.GetNext(pos);
                }

                uint32 tryCount = 0;
                while(pos != NULL && hostsToTraceRoute.GetCount() < 10 && tryCount <= (uint32)list.GetCount()) {
                    tryCount++;
                    CServer* server = list.GetNext(pos);

                    uint32 ip = server->GetIP();

                    if(IsGoodIP(ip, true)) {
                        hostsToTraceRoute.AddTail(ip);
                    }

//                    if(pos == NULL) {
//                        POSITION pos = list.GetHeadPosition();
//                    }
                }
            }

            if(hostsToTraceRoute.GetCount() >= 10) {
                needMoreHosts = false;

                // Signal that there's hosts to fetch.
                newTraceRouteHostEvent->SetEvent();

                addHostLocker.Unlock();
                return true; // got enough hosts
            } else {
                addHostLocker.Unlock();
                return false; // didn't get enough hosts
            }
        } else {
            addHostLocker.Unlock();
            return true; // allready got enough hosts, don't need more
        }
    } else {
        return true; // allready got enough hosts, don't need more
    }
}

bool LastCommonRouteFinder::AddHostsToCheck(CUpDownClientPtrList &list) {
    if(needMoreHosts) {
        addHostLocker.Lock();
        
        if(needMoreHosts) {
            if(list.GetCount() >= 10) {
                hostsToTraceRoute.RemoveAll();

                uint32 startPos = rand()/(RAND_MAX/list.GetCount());

                POSITION pos = list.GetHeadPosition();
                for(uint32 skipCounter = startPos; skipCounter < (uint32)list.GetCount() && pos != NULL; skipCounter++) {
                    list.GetNext(pos);
                }

                uint32 tryCount = 0;
                while(pos != NULL && hostsToTraceRoute.GetCount() < 10 && tryCount <= (uint32)list.GetCount()) {
                    tryCount++;
                    CUpDownClient* client = list.GetNext(pos);

                    uint32 ip = client->GetIP();

                    if(IsGoodIP(ip, true)) {
                        hostsToTraceRoute.AddTail(ip);
                    }

//                    if(pos == NULL) {
//                        POSITION pos = list.GetHeadPosition();
//                    }
                }
            }

            if(hostsToTraceRoute.GetCount() >= 10) {
                needMoreHosts = false;

                // Signal that there's hosts to fetch.
                newTraceRouteHostEvent->SetEvent();

                addHostLocker.Unlock();
                return true; // got enough hosts
            } else {
                addHostLocker.Unlock();
                return false; // didn't get enough hosts
            }
        } else {
            addHostLocker.Unlock();
            return true; // allready got enough hosts, don't need more
        }
    } else {
        return true; // allready got enough hosts, don't need more
    }
}


CurrentPingStruct LastCommonRouteFinder::GetCurrentPing() {
    CurrentPingStruct returnVal;

    if(m_enabled) {
        pingLocker.Lock();
        returnVal.state = m_state;
        returnVal.latency = m_pingAverage;
        returnVal.lowest = m_lowestPing;
        pingLocker.Unlock();
    } else {
        returnVal.state = "";
        returnVal.latency = 0;
        returnVal.lowest = 0;
    }

    return returnVal;
}

//uint32 LastCommonRouteFinder::GetPingedHost() {
//    return 0;
//}

bool LastCommonRouteFinder::AcceptNewClient() {
    return acceptNewClient || !m_enabled; // if enabled, then return acceptNewClient, otherwise return true
}

//MORPH - Changed by SiRoB, Log Flag to trace or not the USS activities
/*
void LastCommonRouteFinder::SetPrefs(bool pEnabled, uint32 pCurUpload, uint32 pMinUpload, uint32 pMaxUpload, bool pUseMillisecondPingTolerance, double pPingTolerance, uint32 pPingToleranceMilliseconds, uint32 pGoingUpDivider, uint32 pGoingDownDivider, uint32 pNumberOfPingsForAverage, uint64 pLowestInitialPingAllowed) {
*/
void LastCommonRouteFinder::SetPrefs(bool pEnabled, uint32 pCurUpload, uint32 pMinUpload, uint32 pMaxUpload, bool pUseMillisecondPingTolerance, double pPingTolerance, uint32 pPingToleranceMilliseconds, uint32 pGoingUpDivider, uint32 pGoingDownDivider, uint32 pNumberOfPingsForAverage, uint64 pLowestInitialPingAllowed, bool IsUSSLog) {
	bool sendEvent = false;

    prefsLocker.Lock();

    if(pMinUpload <= 1024) {
        minUpload = 1024;
    } else {
        minUpload = pMinUpload;
    }

    if(pMaxUpload != 0) {
        maxUpload = pMaxUpload;
        if(maxUpload < minUpload) {
            minUpload = maxUpload;
        }
    } else {
        maxUpload = _UI32_MAX;
    }

    if(pEnabled && m_enabled == false) {
        sendEvent = true;
        // this will show the area for ping info in status bar.
		theApp.emuledlg->SetStatusBarPartsSize();
    } else if(pEnabled == false) {
        if(m_enabled) {
            // this will remove the area for ping info in status bar.
			theApp.emuledlg->SetStatusBarPartsSize();
        }
        prefsEvent->ResetEvent();
    }

    if(m_bUseMillisecondPingTolerance != pUseMillisecondPingTolerance) {
        // this will resize the area for ping info in status bar.
        theApp.emuledlg->SetStatusBarPartsSize();
    }

    m_enabled = pEnabled;
    m_bUseMillisecondPingTolerance = pUseMillisecondPingTolerance;
    m_pingTolerance = pPingTolerance;
    m_iPingToleranceMilliseconds = pPingToleranceMilliseconds;
    m_goingUpDivider = pGoingUpDivider;
    m_goingDownDivider = pGoingDownDivider;
    m_CurUpload = pCurUpload;
    m_iNumberOfPingsForAverage = pNumberOfPingsForAverage;
    m_LowestInitialPingAllowed = pLowestInitialPingAllowed;
	
	m_bIsUSSLog = IsUSSLog; //MORPH - Added by SiRoB, Log Flag to trace or not the USS activities
    uploadLocker.Lock();

    if (m_upload > maxUpload || pEnabled == false) {
        m_upload = maxUpload;
    }

    uploadLocker.Unlock();
    prefsLocker.Unlock();

    if(sendEvent) {
        prefsEvent->SetEvent();
    }
}

uint32 LastCommonRouteFinder::GetUpload() {
    uint32 returnValue;

    uploadLocker.Lock();

    returnValue = m_upload;

    uploadLocker.Unlock();

    return returnValue;
}

void LastCommonRouteFinder::SetUpload(uint32 newValue) {
    uploadLocker.Lock();

    m_upload = newValue;

    uploadLocker.Unlock();
}

/**
 * Make the thread exit. This method will not return until the thread has stopped
 * looping.
 */
void LastCommonRouteFinder::EndThread() {
    // signal the thread to stop looping and exit.
    doRun = false;

    prefsEvent->SetEvent();
    newTraceRouteHostEvent->SetEvent();

    // wait for the thread to signal that it has stopped looping.
    threadEndedEvent->Lock();
}

/**
 * Start the thread. Called from the constructor in this class.
 *
 * @param pParam
 *
 * @return
 */
UINT AFX_CDECL LastCommonRouteFinder::RunProc(LPVOID pParam) {
	DbgSetThreadName("LastCommonRouteFinder");
    LastCommonRouteFinder* lastCommonRouteFinder = (LastCommonRouteFinder*)pParam;

    return lastCommonRouteFinder->RunInternal();
}

/**
 * @return always returns 0.
 */
UINT LastCommonRouteFinder::RunInternal() {
    Pinger pinger;
    bool hasSucceededAtLeastOnce = false;

    while(doRun) {
        // wait for updated prefs
        prefsEvent->Lock();

        bool enabled = m_enabled;

        // retry loop. enabled will be set to false in end of this loop, if to many failures (tries too large)
        while(doRun && enabled) {
            bool foundLastCommonHost = false;
            uint32 lastCommonHost = 0;
            uint32 lastCommonTTL = 0;
            uint32 hostToPing = 0;

            bool useUdp;

            hostsToTraceRoute.RemoveAll();

            pingDelays.RemoveAll();
            pingDelaysTotal = 0;

            pingLocker.Lock();
            m_pingAverage = 0;
            m_lowestPing = 0;
            m_state = "Preparing...";
            pingLocker.Unlock();

            // Calculate a good starting value for the upload control. If the user has entered a max upload value, we use that. Otherwise 10 KBytes/s
            int startUpload = (maxUpload != _UI32_MAX)?maxUpload:10*1024;

            bool atLeastOnePingSucceded = false;
            while(doRun && enabled && foundLastCommonHost == false) {
                uint32 traceRouteTries = 0;
                while(doRun && enabled && foundLastCommonHost == false && (traceRouteTries < 5 || hasSucceededAtLeastOnce && traceRouteTries < _UI32_MAX) && hostsToTraceRoute.GetCount() < 10) {
                    traceRouteTries++;

                    lastCommonHost = 0;

                    if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
						theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSCOLLECTINGHOSTS), traceRouteTries);

                    addHostLocker.Lock();
                    needMoreHosts = true;
                    addHostLocker.Unlock();

                    // wait for hosts to traceroute
                    newTraceRouteHostEvent->Lock();

					if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
						theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSENOUGHHOSTS));

                    {
                        POSITION pos = hostsToTraceRoute.GetHeadPosition();
                        int counter = 0;
                        while(pos != NULL) {
                            counter++;
                            uint32 hostToTraceRoute = hostsToTraceRoute.GetNext(pos);
                            IN_ADDR stDestAddr;
                            stDestAddr.s_addr = hostToTraceRoute;

							if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
								theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSHOSTLISTING), counter, inet_ntoa(stDestAddr));
                        }
                    }

                    // find the last common host, using traceroute

					if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
						theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSFINDLASTCOMMON));

                    // for the tracerouting phase (preparing...) we need to disable uploads so we get a faster traceroute and better ping values.
					if (!m_bUseMillisecondPingTolerance){ //Added by SiRoB
						SetUpload(512);
	                    Sleep(SEC2MS(1));
					}//Added by SiRoB

                    if(m_enabled == false) {
                        enabled = false;
                    }

                    bool failed = false;

                    uint32 curHost = 0;
                    for(uint32 ttl = 1; doRun && enabled && (curHost != 0 && ttl <= 64 || curHost == 0 && ttl < 5) && foundLastCommonHost == false && failed == false; ttl++) {
                        if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
							theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Pinging for TTL %i...", ttl);

						useUdp = false; // PENDING: Get default value from prefs?

                        curHost = 0;
						PingStatus pingStatus = {0};

                        if(m_enabled == false) {
                            enabled = false;
                        }

                        uint32 lastSuccedingPingAddress = 0;
                        bool firstLoop = true;
                        POSITION pos = hostsToTraceRoute.GetHeadPosition();
                        while(doRun && enabled && failed == false && pos != NULL &&
                              (
                               firstLoop ||
                               pingStatus.success && pingStatus.destinationAddress == curHost ||
                               pingStatus.success == false && pingStatus.error == IP_REQ_TIMED_OUT
                              )
                             ) {

                            firstLoop = false;

                            lastSuccedingPingAddress = 0;

                            POSITION lastPos = pos;

                            // this is the current address we send ping to, in loop below.
                            // PENDING: Don't confuse this with curHost, which is unfortunately almost
                            // the same name. Will rename one of these variables as soon as possible, to
                            // get more different names.
                            uint32 curAddress = hostsToTraceRoute.GetNext(pos);

                            pingStatus.success = false;
                            for(int counter = 0; doRun && enabled && counter < 3 && (pingStatus.success == false || pingStatus.success == true && pingStatus.status != IP_SUCCESS && pingStatus.status != IP_TTL_EXPIRED_TRANSIT); counter++) {
                            	pingStatus = pinger.Ping(curAddress, ttl, m_bIsUSSLog, useUdp); //MORPH - Modified by SiRoB, USS log debug
                                if(doRun && enabled &&
									(
									pingStatus.success == false ||
                                    pingStatus.success == true && pingStatus.status != IP_SUCCESS && pingStatus.status != IP_TTL_EXPIRED_TRANSIT
                                   ) &&
                                   counter < 3-1) {
                                    IN_ADDR stDestAddr;
                                    stDestAddr.s_addr = curAddress;
									if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
										theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSCANTPING), counter+1, ttl, inet_ntoa(stDestAddr), pingStatus.error);
                                    pinger.PIcmpErr(pingStatus.error);

                                    Sleep(1000);

                                    if(m_enabled == false) {
                                        enabled = false;
                                    }

                                    // trying other ping method
                                    useUdp = !useUdp;
                                }
                            }

                            if(pingStatus.success == true && pingStatus.status == IP_TTL_EXPIRED_TRANSIT) {
                                if(curHost == 0) {
                                    curHost = pingStatus.destinationAddress;
                                }
                                atLeastOnePingSucceded = true;
                                lastSuccedingPingAddress = curAddress;
                            } else {
                                // failed to ping this host for some reason.
                                // Or we reached the actual host we are pinging. We don't want that, since it is too close.
                                // Remove it.
                                IN_ADDR stDestAddr;
                                stDestAddr.s_addr = curAddress;
                                if(pingStatus.success == true && pingStatus.status == IP_SUCCESS) {
									if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
										theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSHOSTTOOCLOSE), ttl, inet_ntoa(stDestAddr), pingStatus.status);

                                    hostsToTraceRoute.RemoveAt(lastPos);
                                    failed = true;
                                } else if(pingStatus.success == true) {
                                    if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
										theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Unknown pinging error! (TTL: %i IP: %s status: %i). Reason follows. Changing ping method to see if it helps.", ttl, inet_ntoa(stDestAddr), pingStatus.status);
                                    pinger.PIcmpErr(pingStatus.status);
                                    useUdp = !useUdp;
                                } else {
                                    if(pingStatus.error != IP_REQ_TIMED_OUT) {
										if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
											theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSHOSTTOOCLOSE), ttl, inet_ntoa(stDestAddr), pingStatus.error);
                                        pinger.PIcmpErr(pingStatus.error);

                                        hostsToTraceRoute.RemoveAt(lastPos);
                                        failed = true;
                                    } else {
										if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
											theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Timeout when pinging a host! (TTL: %i IP: %s Error: %i). Keeping host. Error info follows.", ttl, inet_ntoa(stDestAddr), pingStatus.error);
                                        pinger.PIcmpErr(pingStatus.error);
                                    }
                                }
                            }
                        }

                        if(failed == false) {
                            if(curHost != 0) {
                                if(pingStatus.success) {
                                    if(pingStatus.destinationAddress == curHost) {
                                    IN_ADDR stDestAddr;
                                    stDestAddr.s_addr = curHost;
									if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
										theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSHOSTTTL), ttl, inet_ntoa(stDestAddr));

                                    lastCommonHost = curHost;
                                    lastCommonTTL = ttl;
                                    } else if(lastSuccedingPingAddress != 0) {
                                    foundLastCommonHost = true;
                                    hostToPing = lastSuccedingPingAddress;

                                    IN_ADDR stDestAddr;
                                    stDestAddr.s_addr = hostToPing;
                                        CString hostToPingString = inet_ntoa(stDestAddr);

                                        if(lastCommonHost != 0) {
                                            if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
												theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Found differing host at TTL %i: %s. This will be the host to ping.", ttl, hostToPingString);
                                        } else {
                                            IN_ADDR stLastCommonHost;
                                            stLastCommonHost.s_addr = curHost;
                                            CString lastCommonHostString = inet_ntoa(stLastCommonHost);
    
                                    
                                            lastCommonHost = curHost;
                                            lastCommonTTL = ttl;
                                            if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
												theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Found differing host at TTL %i, but last ttl couldn't be pinged so we don't know last common host. Taking a chance and using first differing ip as last commonhost. Host to ping: %s. Faked LastCommonHost: %s", ttl, hostToPingString, lastCommonHostString);
                                        }
                                    } else {
                                        // Shouldn't happen. This would mean first ping for a ttl failed, and that it would also be a differing host
                                        // on that ttl. How can 1 single host be differing?
										if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
                                        	theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: This should never happen. If it does, add tracing here.");
                                        enabled = false;
                                        foundLastCommonHost = false;
                                    }
                                } else {
                                    enabled = false;
                                    foundLastCommonHost = false;
                                }
                            } else {
                                if(ttl < 4) {
									if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
										theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Could perform no ping at all at TTL %i. Trying next ttl.", ttl);
                                } else {
									if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
										theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Could perform no ping at all at TTL %i. Giving up.", ttl);
                                }
                                lastCommonHost = 0;
                            }
                        }
                    }

                    if(foundLastCommonHost == false && traceRouteTries >= 3) {
                        theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Tracerouting failed several times. Waiting a few minutes before trying again.");

                        pingLocker.Lock();
                        m_state = "Waiting...";
                        pingLocker.Unlock();

                        prefsEvent->Lock(3*60*1000);
                    }
                }

				if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
					theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Done tracerouting. Evaluating results.");

                if(foundLastCommonHost == true) {
                    IN_ADDR stLastCommonHostAddr;
                    stLastCommonHostAddr.s_addr = lastCommonHost;

                    // log result
                    
                    if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
						theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Found last common host. LastCommonHost: %s @ TTL: %i", inet_ntoa(stLastCommonHostAddr), lastCommonTTL);

                    IN_ADDR stHostToPingAddr;
                    stHostToPingAddr.s_addr = hostToPing;
					if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
						theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Found last common host. HostToPing: %s", inet_ntoa(stHostToPingAddr));
                } else {
					if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
						theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSTRACEFAILDIS));
                    enabled = false;

					pingLocker.Lock();
                    m_state = "Error.";
                    pingLocker.Unlock();
                    // PENDING: this may not be thread safe
                    thePrefs.SetDynUpEnabled(false);
                }
            }

            if(m_enabled == false) {
                enabled = false;
            }


            if(doRun && enabled) {
				if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
					theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSSTARTLOWPING));
            }

            // PENDING:
            prefsLocker.Lock();
            uint64 lowestInitialPingAllowed = m_LowestInitialPingAllowed;
            prefsLocker.Unlock();

            uint32 initial_ping = _I32_MAX;

            bool foundWorkingPingMethod = false;
            // finding lowest ping
            for(int initialPingCounter = 0; doRun && enabled && initialPingCounter < 10; initialPingCounter++) {
                Sleep(200);

                PingStatus pingStatus = pinger.Ping(hostToPing, lastCommonTTL, m_bIsUSSLog, useUdp); //MORPH - Modified by SiRoB, USS log debug

                 if (pingStatus.success && pingStatus.status == IP_TTL_EXPIRED_TRANSIT) {
                    foundWorkingPingMethod = true;

					if(pingStatus.delay > 0 && pingStatus.delay < initial_ping) {
                        initial_ping = max(pingStatus.delay, lowestInitialPingAllowed);
                    }
                } else {
                    if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
						theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: %s-Ping #%i failed. Reason follows", useUdp?"UDP":"ICMP", initialPingCounter);
                    pinger.PIcmpErr(pingStatus.error);


                    if(!pingStatus.success && !foundWorkingPingMethod) {
                        // trying other ping method
                        useUdp = !useUdp;
                    }
                }

                if(m_enabled == false) {
                    enabled = false;
                }
            }

            // Set the upload to a good starting point
			if(!m_bUseMillisecondPingTolerance){
				SetUpload(startUpload);
				Sleep(SEC2MS(1));
			}
            DWORD initTime = ::GetTickCount();

            // if all pings returned 0, initial_ping will not have been changed from default value.
            // then set initial_ping to lowestInitialPingAllowed
            if(initial_ping == _I32_MAX) {
                initial_ping = lowestInitialPingAllowed;
            }

            uint32 upload = 0;

            hasSucceededAtLeastOnce = true;

            if(doRun && enabled) {
                if(initial_ping > lowestInitialPingAllowed) {
                    if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
						theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Lowest ping: %i ms", initial_ping);
                } else {
                    if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
						theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Lowest ping: %i ms. (Filtered lower values. Lowest ping is never allowed to go under %i ms)", initial_ping, lowestInitialPingAllowed);
                }

                prefsLocker.Lock();
                upload = m_CurUpload;

                if(upload < minUpload) {
                    upload = minUpload;
                }
                if(upload > maxUpload) {
                    upload = maxUpload;
                }
                prefsLocker.Unlock();
            }

            if(m_enabled == false) {
                enabled = false;
            }

            if(doRun && enabled) {
                if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
					theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Done with preparations. Starting control of upload speed.");
            }

            pingLocker.Lock();
            m_state = "";
            pingLocker.Unlock();

            // There may be several reasons to start over with tracerouting again.
            // Currently we only restart if we get an unexpected ip back from the
            // ping at the set TTL.
            bool restart = false;

            DWORD lastLoopTick = ::GetTickCount();
            DWORD lastUploadReset = 0;

            while(doRun && enabled && restart == false) {
                DWORD ticksBetweenPings = 1000;
                if(upload > 0) {
                    // ping packages being 64 bytes, this should use 1% of bandwidth (one hundredth of bw).
                    ticksBetweenPings = (64*100*1000)/upload;

                    if(ticksBetweenPings < 125) {
                        // never ping more than 8 packages a second
                        ticksBetweenPings = 125;
                    } else if(ticksBetweenPings > 1000) {
                        ticksBetweenPings = 1000;
                    }
                }

                DWORD curTick = ::GetTickCount();

                DWORD timeSinceLastLoop = curTick-lastLoopTick;
                if(timeSinceLastLoop < ticksBetweenPings) {
                    //theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Sleeping %i ms, timeSinceLastLoop %i ms ticksBetweenPings %i ms", ticksBetweenPings-timeSinceLastLoop, timeSinceLastLoop, ticksBetweenPings);
                    Sleep(ticksBetweenPings-timeSinceLastLoop);
                } else {
                    //theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Skipped sleeping. timeSinceLastLoop %i ms ticksBetweenPings %i ms", timeSinceLastLoop, ticksBetweenPings);
                }

                lastLoopTick = curTick;

                prefsLocker.Lock();
                double pingTolerance = m_pingTolerance;
                uint32 pingToleranceMilliseconds = m_iPingToleranceMilliseconds;
                bool useMillisecondPingTolerance = m_bUseMillisecondPingTolerance;
                uint32 goingUpDivider = m_goingUpDivider;
                uint32 goingDownDivider = m_goingDownDivider;
                uint32 numberOfPingsForAverage = m_iNumberOfPingsForAverage;
                lowestInitialPingAllowed = m_LowestInitialPingAllowed; // PENDING
                prefsLocker.Unlock();

                {
                    DWORD tempTick = ::GetTickCount();

                    if(tempTick - initTime < SEC2MS(60)) {
                        goingUpDivider = 1;
                        goingDownDivider = 1;
                    } else if(/*tempTick - lastUploadReset > SEC2MS(5) ||*/ tempTick - initTime < SEC2MS(61)) {
                        lastUploadReset = tempTick;
                        prefsLocker.Lock();
                        upload = m_CurUpload;
                        prefsLocker.Unlock();
                    }
                }

                uint32 soll_ping = initial_ping*pingTolerance;
                // EastShare START - Add by TAHO, USS limit
				if ( useMillisecondPingTolerance ) {
                    soll_ping = pingToleranceMilliseconds; 
				}else{
					soll_ping = initial_ping*pingTolerance; // ZZ, USS
				}
                // EastShare END - Add by TAHO, USS limit

                uint32 raw_ping = soll_ping; // this value will cause the upload speed not to change at all.

                bool pingFailure = false;        
                for(uint64 pingTries = 0; doRun && enabled && (pingTries == 0 || pingFailure) && pingTries < 60; pingTries++) {
                    // ping the host to ping
                    PingStatus pingStatus = pinger.Ping(hostToPing, lastCommonTTL, false, useUdp);

                    if(pingStatus.success && pingStatus.status == IP_TTL_EXPIRED_TRANSIT) {
                        if(pingStatus.destinationAddress != lastCommonHost) {
                            // something has changed about the topology! We got another ip back from this ttl than expected.
                            // Do the tracerouting again to figure out new topology
                            IN_ADDR stLastCommonHostAddr;
                            stLastCommonHostAddr.s_addr = lastCommonHost;
                            CString lastCommonHostAddressString = inet_ntoa(stLastCommonHostAddr);

                            IN_ADDR stDestinationAddr;
                            stDestinationAddr.s_addr = pingStatus.destinationAddress;
                            CString destinationAddressString = inet_ntoa(stDestinationAddr);

                            if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
								theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Network topology has changed. TTL: %i Expected ip: %s Got ip: %s Will do a new traceroute.", lastCommonTTL, lastCommonHostAddressString, destinationAddressString);
                            restart = true;
                        }

                        raw_ping = (uint32)pingStatus.delay;

                        if(pingFailure) {
                            // only several pings in row should fails, the total doesn't count, so reset for each successful ping
                            pingFailure = false;

                            //theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Ping #%i successful. Continuing.", pingTries);
						}
                    } else {
                        raw_ping = soll_ping*3+initial_ping*3; // this value will cause the upload speed be lowered.

                        pingFailure = true;

						if(pingTries > 3) {
                            Sleep(1000);
                        }

                        //theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: %s-Ping #%i failed. Reason follows", useUdp?"UDP":"ICMP", pingTries);
                        //pinger.PIcmpErr(pingStatus.error);
                    }
                }

                if(pingFailure) {
                    theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: No response to pings for a long time. Restarting...");
                    restart = true;
                }

                if(restart == false) {
		            if(raw_ping > 0 && raw_ping < initial_ping && initial_ping > lowestInitialPingAllowed) {
						if(m_bIsUSSLog) //MORPH - Added by SiRoB, USS log debug
							theApp.emuledlg->QueueDebugLogLine(false,GetResString(IDS_USSNEWLOWEST), max(raw_ping,1), initial_ping);
			            initial_ping = max(raw_ping, lowestInitialPingAllowed);
		            }

                    pingDelaysTotal += raw_ping;
                    pingDelays.AddTail(raw_ping);
                    while((uint32)pingDelays.GetCount() > numberOfPingsForAverage) {
                        uint32 pingDelay = pingDelays.RemoveHead();
                        pingDelaysTotal -= pingDelay;
                    }

                    uint32 normalized_ping = 0;
                    if((pingDelaysTotal/pingDelays.GetCount()) > initial_ping) {
		                normalized_ping = (pingDelaysTotal/pingDelays.GetCount()) - initial_ping;
                    }

                    pingLocker.Lock();
                    m_pingAverage = pingDelaysTotal/pingDelays.GetCount();
                    m_lowestPing = initial_ping;
                    pingLocker.Unlock();

		            // Calculate Waiting Time
		            sint64 hping = (soll_ping - (sint64)normalized_ping);
            		
                    // Calculate change of upload speed
                    if(hping < 0) {
                        // lower the speed
                        sint64 ulDiff = hping*1024*10 / (sint64)(goingDownDivider*initial_ping);
                        acceptNewClient = false;

                        //theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Down! Ping cur %i ms. Ave %I64i ms %i values. New Upload %i + %I64i = %I64i", raw_ping, pingDelaysTotal/pingDelays.GetCount(), pingDelays.GetCount(), upload, ulDiff, upload+ulDiff);
                        // prevent underflow
                        if(upload > -ulDiff) {
                            upload += ulDiff;
                        } else {
                            upload = 0;
                        }
                    } else if(hping > 0) {
                        // raise the speed
                        uint64 ulDiff = hping*1024*10 / (uint64)(goingUpDivider*initial_ping);
                        acceptNewClient = true;

                        //theApp.emuledlg->QueueDebugLogLine(false,"UploadSpeedSense: Up! Ping cur %i ms. Ave %I64i ms %i values. New Upload %i + %I64i = %I64i", raw_ping, pingDelaysTotal/pingDelays.GetCount(), pingDelays.GetCount(), upload, ulDiff, upload+ulDiff);
                        // prevent overflow
                        if(_I32_MAX-upload > ulDiff) {
                            upload += ulDiff;
                        } else {
                            upload = _I32_MAX;
                        }
                    }

                    //if(abs(ulDiff) > 20) {
                    //    uint64 divider = abs(ulDiff)/20;
                    //    int tempNumberOfPingsForAverage = numberOfPingsForAverage/divider;

                    //    while(pingDelays.GetCount() > tempNumberOfPingsForAverage) {
                    //        uint32 pingDelay = pingDelays.RemoveHead();
                    //        pingDelaysTotal -= pingDelay;
                    //    }
                    //}

                    prefsLocker.Lock();
                    if (upload < minUpload) {
                        upload = minUpload;
                        acceptNewClient = true;
                    }
                    if (upload > maxUpload) {
                        upload = maxUpload;
                    }
                    prefsLocker.Unlock();

                    SetUpload(upload);

                    if(m_enabled == false) {
                        enabled = false;
                    }
                }
            }
        }
    }

    // Signal that we have ended.
    threadEndedEvent->SetEvent();

    return 0;
}