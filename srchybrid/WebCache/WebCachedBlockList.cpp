#include "StdAfx.h"
#include "WebCachedBlockList.h"
#include "WebCachedBlock.h"
#include "OtherFunctions.h"
#include "WebCacheProxyClient.h"
#include "resource.h" // needed for emuledlg.h
#include "eMuleDlg.h"
#include "TransferWnd.h"
#include "eMule.h"
#include "WebCacheSocket.h" //JP block-selection
#include "opcodes.h" //JP HR2MS needed in GetNextBlockToDownload()
#include "Log.h"
#define MAX_WCBLOCKLIST_SIZE 2000
#define MAX_TIME_TO_KEEP_OHCB_IN_LIST 60*60*1000  //this is currently only used for Stopped WC-Block-List

CWebCachedBlockList WebCachedBlockList; // global
CStoppedWebCachedBlockList StoppedWebCachedBlockList; //jp list for stopped files


////////CWebCachedBlockList////////////////////////////////////////////////////
bool CWebCachedBlockList::IsFull()
{
	return GetCount() >= MAX_WCBLOCKLIST_SIZE;
}

//debug
POSITION CWebCachedBlockList::AddTail( CWebCachedBlock* newelement )
{
	if (thePrefs.GetLogWebCacheEvents())
	AddDebugLogLine( false, _T("CWebCachedBlockList::AddTail called, queue length: %u"), GetCount() + 1 ); //jp +1 because we are adding an element
	return CStdWebCachedBlockList::AddTail( newelement );
}


void CWebCachedBlockList::TryToDL()
{
	while( !IsEmpty() ) {
		CWebCachedBlock* block = GetNextBlockToDownload(); //JP download from same proxy first
		if( block->IsValid() ) {
			block->DownloadIfPossible();
			return;
		} else {
			delete block;
		}
	}
	if( SINGLEProxyClient && SINGLEProxyClient->GetRequestFile() ) {
		theApp.emuledlg->transferwnd->downloadlistctrl.RemoveSource(SINGLEProxyClient,SINGLEProxyClient->GetRequestFile());
		theApp.emuledlg->transferwnd->downloadclientsctrl.RemoveClient(SINGLEProxyClient); // MORPH - Added by Commander, DownloadClientsCtrl ProxyClient
		POSITION pos = SINGLEProxyClient->GetRequestFile()->srclist.Find(SINGLEProxyClient);
		if (pos)
			SINGLEProxyClient->GetRequestFile()->srclist.RemoveAt(pos);
		SINGLEProxyClient->SetRequestFile(0);
	}
}

CWebCachedBlockList::CWebCachedBlockList(void)
{
}

CWebCachedBlockList::~CWebCachedBlockList(void)
{
	while( !IsEmpty() ) {
		CWebCachedBlock* block = RemoveHead();
		delete block;
	}
}

//JP download from same proxy first START
CWebCachedBlock* CWebCachedBlockList::GetNextBlockToDownload()
{
if (!SINGLEProxyClient->m_pWCDownSocket)
	return WebCachedBlockList.RemoveHead(); //No WCDownSocket, return Head

	SOCKADDR_IN sockAddr = {0};
	int nSockAddrLen = sizeof(sockAddr);
	
	if (!SINGLEProxyClient->m_pWCDownSocket->GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen))
		return WebCachedBlockList.RemoveHead(); // can't get the IP of the proxy return Head
	
	POSITION pos = GetHeadPosition();
	int i = 0;
	while (i < GetCount())
	{
		CWebCachedBlock* cur_block = GetAt(FindIndex(i));
		if (::GetTickCount() - cur_block->m_uTime > HR2MS(1)) //if a block is older than one hour download that one first
 			{
 				CWebCachedBlock* ToReturn = GetAt(FindIndex(i));
 				RemoveAt(FindIndex(i));
 				return ToReturn;
 			}

		if (sockAddr.sin_addr.S_un.S_addr == cur_block->GetProxyIp()) //if a block has the same proxy IP download that one so we don't have to reconnect
		{
			CWebCachedBlock* ToReturn = GetAt(FindIndex(i));
			RemoveAt(FindIndex(i));
			return ToReturn; //matching block found, remove it from the list and return it
		}
		else
			i++;
	}
	return WebCachedBlockList.RemoveHead(); //no matching block found, return head
}
//JP download from same proxy first END

////////CStoppedWebCachedBlockList////////////////////////////////////////////////////
bool CStoppedWebCachedBlockList::IsFull()
{
	return GetCount() >= MAX_WCBLOCKLIST_SIZE;
}

//debug
POSITION CStoppedWebCachedBlockList::AddTail( CWebCachedBlock* newelement )
{
	if (thePrefs.GetLogWebCacheEvents())
	AddDebugLogLine( false, _T("CStoppedWebCachedBlockList::AddTail called, queue length: %u"), GetCount() + 1 ); //jp +1 because we are adding an element
	return CStdWebCachedBlockList::AddTail( newelement );
}


CStoppedWebCachedBlockList::CStoppedWebCachedBlockList(void)
{
}

CStoppedWebCachedBlockList::~CStoppedWebCachedBlockList(void)
{
	while( !IsEmpty() ) {
		CWebCachedBlock* block = RemoveHead();
		delete block;
	}
}

void CStoppedWebCachedBlockList::CleanUp()
{
POSITION pos = GetHeadPosition();
int i = 0;
while (i < GetCount())
{
		CWebCachedBlock* cur_block = GetAt(FindIndex(i));
		if (cur_block->m_uTime < GetTickCount()- MAX_TIME_TO_KEEP_OHCB_IN_LIST)
		{
			RemoveAt(FindIndex(i));
			delete cur_block;
		if (thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine( false, _T("Old Chunk Removed from Stopped List!!!!"));
		}
		else
			i++;
}
}
