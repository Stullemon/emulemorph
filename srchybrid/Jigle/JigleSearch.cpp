#include "stdafx.h"
#include "emule.h"
#include "SearchDlg.h"
#include "SearchList.h"
#include "JigleSearch.h"
#include "soapH.h"
#include "soapJigleService-1.0.nsmap"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// eMule has to *ensure* that the "gzip compression" is compiled. Do NOT change this!!
#if !defined(_DEBUG_SOAP) && (!defined(WITH_GZIP) || !defined(WITH_ZLIB))
#error "You have to define WITH_GZIP and WITH_ZLIB to complile this module for usage in the eMule project!"
#endif

// MOD Note: Do NOT change this part
#define MAX_JIGLE_RESULTS		100
// MOD Note: end

#define TYPECODE_AUDIO			0x0001  // Identifies audio files.
#define TYPECODE_VIDEO			0x0002  // Identifies video files.
#define TYPECODE_IMAGE			0x0004  // Identifies image files.
#define TYPECODE_PROGRAM		0x0008  // Identifies program files.
#define TYPECODE_DOCUMENT		0x0010  // Identifies document files.
#define TYPECODE_COLLECTION		0x0020  // Identifies file collections (deprecated?).

#define MATCH_KEYWORDS			0x0100  // Match keywords instead of fulltext when searching.
#define SORT_ATTR_SIZE			0x0200  // Sort search results by file size.
#define SORT_ATTR_RELEVANCE		0x0400  // Sort search results by relevance.
#define SORT_ASCENDING			0x0800  // Sort search results in ascending order.

#define GET_FILENAMES			0x1000  // Include each files filename list in the search results.
#define GET_SERVERS				0x2000  // Include each files server list in the search results.
#define GET_SR_RELEASEID		0x4000  // Include the ShareReactor release ID of files in the search results.


bool CSearchDlg::DoNewJigleSearch(SSearchParams* pParams)
{
	// to search the Jigle database we have to specify either
	//	*) a file type (TYPECODE_xxx)
	//	--or--
	//	*) a file extension
	if (GetResString(IDS_SEARCH_ANY) == pParams->strFileType && pParams->strExtension.IsEmpty()){
		AfxMessageBox(_T("If you want to search the Jigle database without specifying a file type, you have to specify at least a file extension!"));
		GetDlgItem(IDC_EDITSEARCHEXTENSION)->SetFocus();
		return false;
	}

	if (!m_pJigleThread){
		m_pJigleThread = new CJigleSOAPThread;
		if (!m_pJigleThread->CreateThread()){
			delete m_pJigleThread;
			m_pJigleThread = NULL;
			return false;
		}
	}

	JIGLE_SEARCH_REQUEST* pReq = new JIGLE_SEARCH_REQUEST;
	// Set Jigle search parameters
	pReq->strPhrase = pParams->strExpression;
	pReq->strExt = pParams->strExtension;
	pReq->iAvail = pParams->iAvailability <= 2 ? 3 : pParams->iAvailability;
	pReq->llMinSize = (pParams->ulMinSize == 0) ?  1I64 : pParams->ulMinSize;
	pReq->llMaxSize = (pParams->ulMaxSize == 0) ? -1I64 : pParams->ulMaxSize;
	pReq->iOffset = 0;
	//pReq->iTotal = 10;	// this will put more load on the server than querying for 100 results!
	pReq->iTotal = MAX_JIGLE_RESULTS;

	pReq->iOptions = GET_FILENAMES | (pParams->bMatchKeywords ? MATCH_KEYWORDS : 0);
	//TODO: use 'GetE2DKFileTypeSearchTerm'
	CString strResultType = pParams->strFileType;
	if (GetResString(IDS_SEARCH_AUDIO) == strResultType)
	{
		pReq->iOptions |= TYPECODE_AUDIO;
	}
	else if (GetResString(IDS_SEARCH_VIDEO) == strResultType)
	{
		pReq->iOptions |= TYPECODE_VIDEO;
	}
	else if (GetResString(IDS_SEARCH_PRG) == strResultType)
	{
		strResultType = GetResString(IDS_SEARCH_ANY);
		pReq->iOptions |= TYPECODE_PROGRAM;
	}
	else if (GetResString(IDS_SEARCH_PICS) == strResultType)
	{
		pReq->iOptions |= TYPECODE_IMAGE;
	}
	else if (GetResString(IDS_SEARCH_CDIMG) == strResultType)
	{
		pReq->iOptions |= TYPECODE_PROGRAM;
	}
	else if (GetResString(IDS_SEARCH_ARC) == strResultType)
	{
		pReq->iOptions |= TYPECODE_PROGRAM;
	}
	else
	{
		//TODO: Support "Doc" types
		ASSERT( GetResString(IDS_SEARCH_ANY) == strResultType );
		ASSERT( !pReq->strExt.IsEmpty() );
	}

	m_nSearchID++;
	pParams->dwSearchID = m_nSearchID;
	theApp.searchlist->NewSearch(&searchlistctrl, strResultType, m_nSearchID);
	canceld = false;
	globsearch = false;
	CreateNewTab(pParams);

	pReq->hWnd = AfxGetMainWnd()->GetSafeHwnd();
	// 'iTotalSearchResults' is used as an extra 'safety' counter for the nr. of search results, to avoid the worst case 
	// of querying the server in an endless loop because of false 'offset' and 'total' fields in the search results.
	pReq->iTotalSearchResults = 0; 
	pReq->nSearchID = m_nSearchID;

	m_pJigleThread->PostThreadMessage(WM_JIGLE_SEARCH_REQUEST, 0, (LPARAM)pReq);
	return true;
}

static int __cdecl Compare_api__Filename(const void* p1, const void* p2)
{
	const api__Filename* pFile1 = *(const api__Filename**)p1;
	const api__Filename* pFile2 = *(const api__Filename**)p2;
	return pFile2->a - pFile1->a;
}

static void Free(api__searchResponse* res)
{
	if (res)
	{
		if (res->_r)
		{
			if (res->_r->m)
			{
				const api__File* pFileHash = res->_r->m->__ptr;
				int iFileHashs = res->_r->m->__size; 
				while (iFileHashs--)
				{
					if (pFileHash->n)
					{
						delete pFileHash->n->__ptr;
						delete pFileHash->n;
					}
					if (pFileHash->v)
						delete pFileHash->v;
					pFileHash++;
				}
				delete res->_r->m->__ptr;
				delete res->_r->m;
			}
			delete res->_r;
		}
	}
}

LRESULT CSearchDlg::ProcessJigleSearchResponse(WPARAM wParam, LPARAM lParam)
{
	JIGLE_SEARCH_RESPONSE* pRes = (JIGLE_SEARCH_RESPONSE*)lParam;
	struct soap* soap = pRes->soap;

	bool bMore = false;
	if (pRes->iResult)
	{
		Debug("Jigle search response: Error\n");
		CString strError;
		if (soap->error)
		{
			//Examples for gSOAP errors
			//
			//Connecting to non existing host (e.g. http://xyz.com/soap)
			//----------------------------------------------------------
			//SOAP fault: SOAP-ENV:Client
			//"Host not found"
			//Detail: TCP get host by name failed int tcp_connect()
			//
			//Connecting to a non existing directory on the server (e.g. http://jigle.com/soap404)
			//------------------------------------------------------------------------------------
			//SOAP fault: SOAP-ENV:Server
			//"HTTP error"
			//Detail: HTTP/1.0 404 Not Found

			const char **s = soap_faultdetail(soap);
			if (!*soap_faultcode(soap))
				soap_set_fault(soap);
			if (!*soap_faultstring(soap))
				*soap_faultstring(soap) = "";

			strError.Format(_T("Failed to search the jigle database!\r\n\r\n")
							_T("SOAP fault: %hs\r\n\"%hs\""), 
							*soap_faultcode(soap), 
							*soap_faultstring(soap));
			if (s && *s)
			{
				strError += _T("\r\nDetail: ");
				strError += *s;
			}

			if (*soap_faultcode(soap) && stricmp(*soap_faultcode(soap), "SOAP-ENV:Server")==0)
				strError += _T("\r\n\r\nVisit http://jigle.com/soap for more details about the error.");
		}
		AfxMessageBox(strError, MB_ICONERROR);
		Free(pRes->res);
	}
	else if (pRes->res && pRes->res->_r)
	{ 
		if (pRes->res->_r->m)
		{
			Debug("Jigle search response\n  offset=%u  total=%u  matches=%u\n", pRes->res->_r->o, pRes->res->_r->t, pRes->res->_r->m->__size);
			int iResultsAdded = 0;
			for (int iFileHash = 0; iFileHash < pRes->res->_r->m->__size; iFileHash++)
			{
				const api__File* pFileHash = pRes->res->_r->m->__ptr + iFileHash;
				uchar aucFileHash[16];
				if (!canceld && strmd4(pFileHash->h, aucFileHash))
				{
					//Debug("  size=%9u  hash=%s\n", (ULONG)pFileHash->size, pFileHash->hash);
					if (pFileHash->n)
					{
						// sort filenames by availability
						CTypedPtrArray<CPtrArray, const api__Filename*> aFileNames;
						aFileNames.SetSize(pFileHash->n->__size);
						for (int iFileName = 0; iFileName < aFileNames.GetSize(); iFileName++)
							aFileNames[iFileName] = pFileHash->n->__ptr + iFileName;
						qsort(aFileNames.GetData(), aFileNames.GetSize(), sizeof(aFileNames[0]), Compare_api__Filename);

						bool bAddedFile = false;
						for (int iFileName = 0; iFileName < aFileNames.GetSize(); iFileName++)
						{
							const api__Filename* pFileName = aFileNames[iFileName];
							//Debug("    avail=%5u  name=%s\n", pFileName->avail, pFileName->text);

							CSearchFile* pSearchFile = new CSearchFile(m_nSearchID, aucFileHash, 
									pFileHash->s, pFileName->t, pFileHash->t, pFileName->a);
							if (theApp.searchlist->AddToList(pSearchFile))
								bAddedFile = true;
						}
						if (bAddedFile) // if we added at least one file, we count this hash+size as a search result
							iResultsAdded++;
					}
				}
				if (pFileHash->n)
				{
					delete pFileHash->n->__ptr;
					delete pFileHash->n;
				}
			}

			pRes->pReq->iTotalSearchResults += iResultsAdded;
			Debug("  iResultsAdded=%u  iTotalSearchResults=%u\n", iResultsAdded, pRes->pReq->iTotalSearchResults);

			if (   !canceld 
				&& pRes->pReq->iTotalSearchResults < MAX_JIGLE_RESULTS 
				&& pRes->res->_r->o + pRes->res->_r->m->__size < pRes->res->_r->t)
			{
				pRes->pReq->iOffset = pRes->res->_r->o + pRes->res->_r->m->__size;
				pRes->pReq->iTotal = MAX_JIGLE_RESULTS - pRes->pReq->iTotalSearchResults;

				// because we are post-filtering the search results, it's more efficient to query the server for at
				// least 10-20 entries instead of only 1-3 (which may all be dropped because of local filtering)!
				const iMinSearchReqEntries = 20;
				if (pRes->pReq->iTotal < iMinSearchReqEntries)
					pRes->pReq->iTotal = iMinSearchReqEntries;

				bMore = true;
			}
			delete pRes->res->_r->m->__ptr;
			delete pRes->res->_r->m;
		}
		else
			Debug("Jigle search response: Success: <empty>\n");
		delete pRes->res->_r;
	}
	else
	{
		Debug("Jigle search response: Not found\n");
	}

	soap_set_recv_logfile(soap, NULL);
	soap_set_sent_logfile(soap, NULL);
	soap_set_test_logfile(soap, NULL);
	soap_end(pRes->soap); // remove all temporary and deserialized data

	JIGLE_SEARCH_REQUEST* pReq = pRes->pReq;
	delete pRes->res;
	delete pRes->soap;
	delete pRes;

	if (!bMore || (m_pJigleThread && !m_pJigleThread->PostThreadMessage(WM_JIGLE_SEARCH_REQUEST, 0, (LPARAM)pReq)))
	{
		if (m_pJigleThread)
		{
			m_pJigleThread->PostThreadMessage(WM_QUIT, 0, 0);
			m_pJigleThread = NULL;
		}

		delete pReq;
		GetDlgItem(IDC_STARTS)->EnableWindow(true);
		CWnd* pWndFocus = GetFocus();
		GetDlgItem(IDC_CANCELS)->EnableWindow(false);
		if (pWndFocus == GetDlgItem(IDC_CANCELS))
			GetDlgItem(IDC_SEARCHNAME)->SetFocus();
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// CJigleSOAPThread

IMPLEMENT_DYNAMIC(CJigleSOAPThread, CWinThread)

CJigleSOAPThread::CJigleSOAPThread()
{
}

BOOL CJigleSOAPThread::InitInstance()
{
	BOOL bTerminate = FALSE;
	while (!bTerminate)
	{
		UINT uNumEvents = 0;
		DWORD dwEventIdx;
		dwEventIdx = MsgWaitForMultipleObjects(uNumEvents, NULL, FALSE, INFINITE, QS_SENDMESSAGE | QS_POSTMESSAGE | QS_TIMER);
		if (dwEventIdx == (DWORD)-1)
			ASSERT(0);
		else if (dwEventIdx == WAIT_TIMEOUT)
			ASSERT(0);
		else if (dwEventIdx == WAIT_OBJECT_0 + uNumEvents)
		{
			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_JIGLE_SEARCH_REQUEST)
				{
					struct soap* soap = new struct soap;
					soap_init2(soap, SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE);
#ifndef _DEBUG_SOAP
					soap_set_recv_logfile(soap, NULL);
					soap_set_sent_logfile(soap, NULL);
					soap_set_test_logfile(soap, NULL);
#endif
					JIGLE_SEARCH_REQUEST* pReq = (JIGLE_SEARCH_REQUEST*)msg.lParam;

					// MOD Note: A Jigle search expression must follow those rules - Do NOT change this!
					// - The client must announce gzip functionality to the Jigle server with the appropriate HTTP header. (see. 'WITH_GZIP')
					// - The max. nr. of results to be returned for one search request must be specified as <=100.
					// - The search expression must either contain one of the TYPECODE_xxx flags or an filename extension.
					// - Be gentle to the Jigle server!
					// MOD Note: end
					ASSERT( pReq->iTotal <= 100 );
					ASSERT( !pReq->strExt.IsEmpty() || (pReq->iOptions & (TYPECODE_AUDIO | TYPECODE_VIDEO | TYPECODE_IMAGE | TYPECODE_PROGRAM | TYPECODE_DOCUMENT)) );

					api__searchResponse* r = new api__searchResponse;
					r->_r = NULL;
					Debug("Jigle search request\n  offset=%u  total=%u\n", pReq->iOffset, pReq->iTotal);
					int iResult = soap_call_api__search(soap, "http://jigle.com/soap", "", 
							const_cast<char*>((const char*)pReq->strPhrase), 
							const_cast<char*>((const char*)pReq->strExt), 
							pReq->iAvail, pReq->llMinSize, pReq->llMaxSize, pReq->iOffset, pReq->iTotal, pReq->iOptions, r);
					if (iResult)
					{
						JIGLE_SEARCH_RESPONSE* pRes = new JIGLE_SEARCH_RESPONSE;
						pRes->iResult = iResult;
						pRes->soap = soap;
						pRes->res = r;
						pRes->pReq = pReq;
						::PostMessage(pReq->hWnd, WM_JIGLE_SEARCH_RESPONSE, iResult, (LPARAM)pRes);
					}
					else if (r->_r)
					{ 
						JIGLE_SEARCH_RESPONSE* pRes = new JIGLE_SEARCH_RESPONSE;
						pRes->iResult = iResult;
						pRes->soap = soap;
						pRes->res = r;
						pRes->pReq = pReq;
						::PostMessage(pReq->hWnd, WM_JIGLE_SEARCH_RESPONSE, iResult, (LPARAM)pRes);
					}
					else
					{
						// Not found
						delete r;
						JIGLE_SEARCH_RESPONSE* pRes = new JIGLE_SEARCH_RESPONSE;
						pRes->iResult = iResult;
						pRes->soap = soap;
						pRes->res = NULL;
						pRes->pReq = pReq;
						::PostMessage(pReq->hWnd, WM_JIGLE_SEARCH_RESPONSE, iResult, (LPARAM)pRes);
					}
				}
				else if (msg.message == WM_QUIT)
				{
					bTerminate = TRUE;
					break;
				}
				else
					ASSERT(0);
			}
		}
		else
			ASSERT(0);
	}

	return FALSE;
}
