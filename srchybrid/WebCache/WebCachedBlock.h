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

#include "PartFile.h"
#include "KnownFile.h"
#include "UpDownClient.h"

class CWebCacheProxyClient;

class CWebCachedBlock
{
	friend class CWebCacheProxyClient;
private:
	uint32 m_uEnd;
	byte remoteKey[WC_KEYLENGTH]; // Superlexx - encryption
	uint32 m_uProxyIp;
	uint32 m_uHostIp;
	UINT m_uHostPort;
	uint32 m_uLastRequestAt;
	uint32 m_uRequestCount;
	uchar m_UserHash[16];			// the user sent us this block
	bool m_bDownloaded;				// set if block DL succeeded
	bool m_bRequested;				// set if block was requested
	CPartFile* GetFile() const;
	CKnownFile* GetKnownFile() const;
	void UpdateProxyClient();
public:
	uint32 m_uTime; //jp remove old chunks (currently only for Stopped-List)
	uchar m_FileID[16];  //jp needs to be public to throttle the chunk or make public member functions
	uint32 m_uStart; //jp needs to be public to throttle the chunk or make public member functions
	uint32 GetProxyIp() const;
	bool DownloadIfPossible();
	bool IsValid() const;
	void OnSuccessfulDownload();
	void OnWebCacheBlockRequestSent();
	Pending_Block_Struct* CWebCachedBlock::CreatePendingBlock();
	CWebCachedBlock( const BYTE* packet, uint32 size, CUpDownClient* client, bool XpressOHCB = false );
	~CWebCachedBlock();
};