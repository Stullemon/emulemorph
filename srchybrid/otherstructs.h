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

#pragma once

#include "types.h"
#include <zlib/zlib.h>

//			SERVER TO CLIENT
#pragma pack(1)
struct Header_Struct{
	int8	eDonkeyID;
	int32	packetlength;
	int8	command;
};
#pragma pack()

#pragma pack(1)
struct UDP_Header_Struct{
	int8	eDonkeyID;
	int8	command;
};
#pragma pack()

#pragma pack(1)
struct LoginAnswer_Struct {
	uint32	clientid;
};
#pragma pack()

#pragma pack(1)
struct Requested_Block_Struct{
	uint32	StartOffset;
	uint32	EndOffset;
	uint32	packedsize;
	uchar	FileID[16];
	uint32  transferred; // Barry - This counts bytes completed
};
#pragma pack()

#pragma pack(1)
struct Requested_File_Struct{
	uchar	  fileid[16];
	uint32	  lastasked;
	uint8	  badrequests;
};
#pragma pack()

struct Pending_Block_Struct{
	Requested_Block_Struct*	block;
	z_stream               *zStream;       // Barry - Used to unzip packets
	uint32                  totalUnzipped; // Barry - This holds the total unzipped bytes for all packets so far
	bool					bZStreamError;
};

struct Gap_Struct{
	uint32 start;
	uint32 end;
};

//MORPH START - Added by IceCream, SLUGFILLER: Spreadbars
struct Spread_Struct{
	uint32 start;
	uint32 end;
	uint32 count;
};
//MORPH END   - Added by IceCream, SLUGFILLER: Spreadbars

#pragma pack(1)
struct ServerMet_Struct {
	uint32	ip;
	uint16	port;
	uint32	tagcount;
};
#pragma pack()
// BadWolf(Ottavio84) - Accurate speed measurement-
struct TransferredData {
	uint32	datalen;
	DWORD	timestamp;
};

// Maella -Enhanced Chunk Selection- (based on jicxicmic)
#pragma pack(1)
struct Chunk {
	uint16 part;      // Index of the chunk
		union {
			uint16 frequency; // Availability of the chunk
			uint16 rank;      // Download priority factor (highest = 0, lowest = 0xffff)
	};
};
#pragma pack()

//MORPH START - Added by SiRoB, ZZ Upload System
struct SocketTransferStats {
	//uint32	datalen;
	DWORD	latency;
	DWORD	timestamp;
};
//MORPH END - Added by SiRoB, ZZ Upload System
