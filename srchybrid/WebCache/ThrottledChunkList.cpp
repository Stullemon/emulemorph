// jp Don't request chunks for which we are currently receiving proxy sources
// jp list that contains chunks that should not be requested during regular downloads because we are currently receiving proxy-sources for it

#include "StdAfx.h"
#include "ThrottledChunkList.h" //jp throttled chunks
#include "OtherFunctions.h" //jp throttled chunks
#include "Preferences.h" //jp logging
#include "Log.h"

#define THROTTLED_CHUNK_LIST_SIZE 100 
#define THROTTLETIME	1000*60*10	// chunks are throttled for 10 minutes

CThrottledChunkList ThrottledChunkList; // global


void CThrottledChunkList::AddToList( ThrottledChunk newelement )
{
if (!CheckList(newelement, true)) AddTail(newelement);
return;
}

POSITION CThrottledChunkList::AddTail( ThrottledChunk newelement )
{
	if (thePrefs.GetLogWebCacheEvents())
	AddDebugLogLine( false, _T("Chunk %u added to throttled chunk list, length: %u"),newelement.ChunkNr, GetCount() );
	return CStdThrottledChunkList::AddTail( newelement );
}

bool CThrottledChunkList::CheckList(ThrottledChunk tocompare, bool checktime)
{	

	// jp remove all unneeded chunks from list
	uint32 cutofftime = GetTickCount() - THROTTLETIME;
	while ((GetCount()>100) //don't let the list get too long
		||(GetCount()> 0) && (GetHead().timestamp < cutofftime)) // remove old chunks
	{
		RemoveHead();
	}

	// jp compare the chunk with the list
	POSITION pos = GetHeadPosition();
	for (int i=0; i < GetCount(); i++)
	{
	if (Compare(GetNext(pos), tocompare, checktime)) return true;
	}
	return false;
}

bool CThrottledChunkList::Compare(ThrottledChunk oldchunk, ThrottledChunk tocompare, bool checktime)
{

	if (oldchunk.ChunkNr==tocompare.ChunkNr
		&& md4cmp(oldchunk.FileID, tocompare.FileID)==0)
	{
		if (!checktime) return true; 
		else if (tocompare.timestamp - oldchunk.timestamp < THROTTLETIME/2) return true; //if we receive a OHCB for a chunk after half of the throttletime add it to the list
	}
	return false;
}

CThrottledChunkList::CThrottledChunkList(void)
{
}

CThrottledChunkList::~CThrottledChunkList(void)
{
RemoveAll();
}
