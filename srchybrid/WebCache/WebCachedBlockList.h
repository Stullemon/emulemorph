//This file is part of the eMule WebCache mod
//http://ispcachingforemule.de.vu
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
#pragma once
#include "WebCachedBlock.h"

typedef CTypedPtrList<CPtrList, CWebCachedBlock*> CStdWebCachedBlockList;


////////CWebCachedBlockList////////////////////////////////////////////////////
class CWebCachedBlockList :
	public CStdWebCachedBlockList
{
public:
	CWebCachedBlockList(void);
	~CWebCachedBlockList(void);
	CWebCachedBlock* GetNextBlockToDownload(); //JP downlad from same proxy first
	bool IsFull();
	POSITION AddTail( CWebCachedBlock* newelement );
	void TryToDL();
//	bool ProcessWCBlocks(char* packet, uint32 size);	// reads MultiOHCBPacket, finds the sender and adds the blocks to the queue
	bool ProcessWCBlocks(char* packet, uint32 size, UINT opcode = NULL, CUpDownClient* client = NULL);	// reads MultiOHCBPacket and adds the blocks to the queue
};

////////CStoppedWebCachedBlockList////////////////////////////////////////////////////
class CStoppedWebCachedBlockList :
	public CStdWebCachedBlockList
{
public:
	CStoppedWebCachedBlockList(void);
	~CStoppedWebCachedBlockList(void);
	bool IsFull();
	POSITION AddTail( CWebCachedBlock* newelement );
	void CleanUp();
};

extern CWebCachedBlockList WebCachedBlockList;
extern CStoppedWebCachedBlockList StoppedWebCachedBlockList;
