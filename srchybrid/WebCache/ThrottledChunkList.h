// jp list that contains chunks that should not be requested during regular downloads because we are currently receiving proxy-sources for it

#pragma once
#include "Loggable.h"
#include "PartFile.h"

struct ThrottledChunk 
	{
	uint16	ChunkNr;			// Index of the chunk
	uchar	FileID[16];			// FileID
	uint32	timestamp;			// time stamp
	};

typedef CList<ThrottledChunk, ThrottledChunk&> CStdThrottledChunkList;

class CThrottledChunkList :
	public CStdThrottledChunkList, CLoggable
{
public:
	bool CheckList(ThrottledChunk tocompare, bool checktime);
	void AddToList(ThrottledChunk newelement);
	CThrottledChunkList(void);
	~CThrottledChunkList(void);

private:
	bool Compare(ThrottledChunk tocompare1, ThrottledChunk tocompare2, bool checktime);
	POSITION AddTail( ThrottledChunk newelement );

};

extern CThrottledChunkList ThrottledChunkList;
