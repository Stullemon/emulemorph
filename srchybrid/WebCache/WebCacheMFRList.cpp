#include "StdAfx.h"
#include "SharedFileList.h"
#include "KnownFile.h"
#include "emule.h"
#include "WebCacheMFRList.h"
#include "otherfunctions.h"
#include "DownloadQueue.h"
#include "Preferences.h"
#include "UpDownClient.h"
#include "log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CWebCacheMFRList::AddFiles(CSafeMemFile* data, CUpDownClient* client)
{
	if ( this->client && thePrefs.GetLogWebCacheEvents()) 
		AddDebugLogLine(false, _T("RemoveAll on MFR list of client %s, %d file(s) removed, reason: AddFiles"), this->client->DbgGetClientInfo(), length);
	RemoveAll();

	uint8 nrOfRequestedFiles = data->ReadUInt8();
	length = nrOfRequestedFiles;
	this->client = client;
	if ( thePrefs.GetLogWebCacheEvents()) 
		AddDebugLogLine(false, _T("AddFiles on MFR list of client %s, %d file(s) to be added"), this->client->DbgGetClientInfo(), length);

	while ( nrOfRequestedFiles-- > 0 )
	{
		byte temp[16];
		data->ReadHash16(temp);

		CWebCacheMFRReqFile* newFile = new CWebCacheMFRReqFile();
		memcpy(newFile->fileID, temp, 16);
		newFile->partCount = data->ReadUInt16();
		newFile->partStatus.SetSize(newFile->partCount);

		bool sourceAdded = false;
		CString reason;
		CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(newFile->fileID);
		if (reqfile
			&& reqfile->IsPartFile()
			&& reqfile->GetFileSize() > PARTSIZE) // TODO: check download status?
		{
//			if (thePrefs.GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount()) // always add webcache-buddies
			try
			{
				sourceAdded = theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile, client, true); // ignore global dead list since this client is definitely alive
			}
			catch (CString _reason)
			{
				reason = _reason;
			}
		}

		uint16 done = 0;
		while (done != newFile->partCount)
		{
			uint8 toread = data->ReadUInt8();
			for (sint32 i = 0;i != 8;i++)
			{
				newFile->partStatus.SetAt(done++, ((toread>>i)&1)? 1:0);
				if (done == newFile->partCount)
					break;
			}
		}
		reqFiles.AddTail(newFile);

		if (thePrefs.GetLogWebCacheEvents()) 
		{
			char* psz = new char[newFile->partCount + 1];
			for (int i = 0; i < newFile->partCount; i++)
				psz[i] = newFile->partStatus.GetAt(i) ? '#' : 'o';
			psz[i] = '\0';
			if (sourceAdded)
				AddDebugLogLine(false, _T("Added file %s to MFR list of client %s, client added as source, partstatus: %hs"), reqfile ? reqfile->GetFileName() : _T("?"), client->DbgGetClientInfo(), psz);
			else
				AddDebugLogLine(false, _T("Added file %s to MFR list of client %s, client not added as source (reason: %s), partstatus: %hs"), reqfile ? reqfile->GetFileName() : _T("?"), client->DbgGetClientInfo(), reason == _T("") ? _T("client already in the source list") : reason, psz);

			delete[] psz;
		}
	}
	expirationTick = GetTickCount() + WC_MAX_MFR_AGE;
}

CWebCacheMFRList::~CWebCacheMFRList(void)
{
	RemoveAll();
}

bool CWebCacheMFRList::IsPartAvailable(uint16 part, const byte* fileID)
{
	CheckExpiration();
	//if (reqFiles.IsEmpty())
	//	return true;

	for (POSITION pos = reqFiles.GetHeadPosition(); pos != NULL; reqFiles.GetNext(pos))	// file hashes loop
		if(!md4cmp(fileID, reqFiles.GetAt(pos)->fileID))	// file hash found
			return (reqFiles.GetAt(pos)->partStatus[part] != 0);
	return true;	// if the file is not found in the hash list, then the client didn't request it, so he doesn't need it
}

void CWebCacheMFRList::RemoveAll()
{
	//if (reqFiles.IsEmpty())
	//	return;

	for (POSITION pos = reqFiles.GetHeadPosition(); pos; reqFiles.GetNext(pos))
	{
//		delete[] reqFiles.GetAt(pos)->partStatus;
		delete reqFiles.GetAt(pos);
	}
	reqFiles.RemoveAll();
}

void CWebCacheMFRList::CheckExpiration()
{
	if (GetTickCount() > expirationTick && !reqFiles.IsEmpty() && client->IsEd2kClient())
	{
		if (thePrefs.GetLogWebCacheEvents()) 
			AddDebugLogLine(false, _T("RemoveAll on MFR list of client %s, %d file(s) removed, reason: CheckExpiration"), client->DbgGetClientInfo(), length);
		RemoveAll();
	}
}