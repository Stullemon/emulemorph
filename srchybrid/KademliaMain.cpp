#include "StdAfx.h"
#include "KademliaMain.h"
#include "emule.h"
#include "KademliaWnd.h"
#include "KadContactListCtrl.h"
#include "KadSearchListCtrl.h"
#include "Kademlia/Kademlia/kademlia.h"
#include "Kademlia/net/kademliaudplistener.h"

CKademliaMain::CKademliaMain(void)
{
	status = new Status();
	status->m_connected = false;
	status->m_firewalled = true;
	status->m_ip = 0;
	status->m_tcpport = 0;
	status->m_totalFile = 0;
	status->m_totalStore = 0;
	status->m_udpport = 0;
	status->m_kademliaUsers = 0;
}

CKademliaMain::~CKademliaMain(void)
{
	delete status;
}

void CKademliaMain::setStatus(Status* val)
{
	if( val && (status->m_firewalled != val->m_firewalled || status->m_ip != val->m_ip || status->m_udpport != val->m_udpport || status->m_tcpport != val->m_tcpport)){
		delete status;
		status = val;
		theApp.emuledlg->ShowConnectionState();
		theApp.emuledlg->ShowUserCount();
	}
	else if (status->m_connected != val->m_connected){
		delete status;
		status = val;
		theApp.emuledlg->ShowConnectionState();
		theApp.emuledlg->ShowUserCount();
		if(status->m_connected)
			AddLogLine(true, "Kademlia %s", GetResString(IDS_CONNECTED));
		else
			AddLogLine(true, "Kademlia %s", GetResString(IDS_DISCONNECTED));
	}
	else{
		status->m_kademliaUsers = val->m_kademliaUsers;
		status->m_totalFile = val->m_totalFile;
		status->m_totalStore = val->m_totalStore;
		theApp.emuledlg->ShowUserCount();
		delete val;
	}
}

DWORD CKademliaMain::GetThreadID(){
	return Kademlia::CTimer::getThreadID();
}

void CKademliaMain::Connect(){
	if(!Kademlia::CTimer::getThreadID()){
		theApp.emuledlg->kademliawnd->contactList->Hide();
		theApp.emuledlg->kademliawnd->searchList->Hide();
		Kademlia::CPrefs* startupKadPref = new Kademlia::CPrefs();
		startupKadPref->setTCPPort(theApp.glob_prefs->GetPort());
		startupKadPref->setUDPPort(theApp.glob_prefs->GetKadUDPPort());
		Kademlia::CUInt128 clientID;
		clientID.setValue((uchar*)theApp.glob_prefs->GetUserHash());
		startupKadPref->setClientHash(clientID);
		Kademlia::CKademlia::start(startupKadPref);
		Kademlia::CKademlia::setSharedFileList(theApp.sharedfiles);
		theApp.emuledlg->kademliawnd->contactList->Visable();
		theApp.emuledlg->kademliawnd->searchList->Visable();
		theApp.emuledlg->ShowConnectionState();
	}
}

UINT AFX_CDECL KadStopFunc(LPVOID pvParams)
{
	DWORD dwFlags = (DWORD)pvParams;
	Kademlia::CKademlia::stop();
	// if that thread was started during application shutdown, the 'theApp.emuledlg' may get
	// deleted while this thread is still running.
	if ((dwFlags & 1) && (theApp.emuledlg != NULL && theApp.emuledlg->IsRunning())){
		theApp.emuledlg->kademliawnd->contactList->Visable();
		theApp.emuledlg->kademliawnd->searchList->Visable();
		theApp.emuledlg->kademliawnd->GetDlgItem(IDC_KADCONNECT)->EnableWindow(true);
		theApp.emuledlg->toolbar.EnableButton(IDC_TOOLBARBUTTON+0);
		//TODO: Test this alot.. The connect button may still get confused which state it should be in.
		theApp.emuledlg->ShowConnectionState();
	}
	return 0;
}

void CKademliaMain::DisConnect(){
	if(Kademlia::CTimer::getThreadID()){
		if (theApp.emuledlg->IsRunning()){
			theApp.emuledlg->toolbar.EnableButton(IDC_TOOLBARBUTTON+0, false);
			theApp.emuledlg->kademliawnd->GetDlgItem(IDC_KADCONNECT)->EnableWindow(false);
			theApp.emuledlg->kademliawnd->contactList->Hide();
			theApp.emuledlg->kademliawnd->searchList->Hide();
		}

		if (theApp.emuledlg->IsRunning()){
			CWinThread* pKadStopThread = new CWinThread(KadStopFunc, (LPVOID)(theApp.emuledlg->IsRunning() ? 1 : 0));
			if (!pKadStopThread->CreateThread()){
				delete pKadStopThread;
				theApp.emuledlg->kademliawnd->contactList->Visable();
				theApp.emuledlg->kademliawnd->searchList->Visable();
				theApp.emuledlg->kademliawnd->GetDlgItem(IDC_KADCONNECT)->EnableWindow(true);
				theApp.emuledlg->toolbar.EnableButton(IDC_TOOLBARBUTTON+0);
				theApp.emuledlg->ShowConnectionState();
				ASSERT(0);
				return;
			}
		}
		else{
			// this is at least needed for a debug build. if we want to stop the kad threads and properly
			// free all memory to detect mem leaks, we have to wait until the kad threads finished and performed
			// their cleanup code. this takes a noticeable amount of time and could maybe avoided for a release build.
			// though, even for a release build there should be some special shutdown code which waits at least as long
			// as the kad threads have closed their sockets.
			//
			// a fast way (for a release build only; and only for application shutdown) could be to properly let the
			// kad threads exit the message loops, close the sockets, save the files (all this should not take too
			// much time) in a synchronized way and after that just asynchronously kill the kad threads, not caring
			// about memory (will be freed by system).
			Kademlia::CKademlia::stop();
		}
	}
	//Please leave this in a bit for testing to see if it does lockup.. 
	if(Kademlia::CTimer::getThreadID()){}
	status->m_connected = false;
	status->m_firewalled = true;
	status->m_kademliaUsers = 0;
	theApp.emuledlg->ShowConnectionState();
}

void CKademliaMain::Bootstrap(CString ip,uint16 port) {
	Kademlia::CKademlia::getUDPListener()->bootstrap(ip,port);
}

void CKademliaMain::Bootstrap(uint32 ip,uint16 port) {
	Kademlia::CKademlia::getUDPListener()->bootstrap(ip,port);
}