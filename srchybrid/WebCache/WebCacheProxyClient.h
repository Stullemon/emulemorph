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
#include "URLClient.h"
#include "UpDownClient.h"
#include "WebCachedBlock.h"

class CWebCacheProxyClient 
	: public CUpDownClient
{
private:
	friend class CWebCachedBlock;
	CWebCachedBlock* block;
public:
	CWebCacheProxyClient(CWebCachedBlock* iBlock);
	void UpdateClient(CWebCachedBlock* iBlock);
	~CWebCacheProxyClient(void);
	bool SendWebCacheBlockRequests();
	virtual bool TryToConnect(bool bIgnoreMaxCon, CRuntimeClass* pClassSocket = NULL);
	void OnWebCachedBlockDownloaded( const Requested_Block_Struct* reqblock );
	bool ProxyClientIsBusy();
	void DeleteBlock();
};

extern CWebCacheProxyClient* SINGLEProxyClient;
