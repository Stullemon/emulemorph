#include "StdAfx.h"
#include "webcachecryptography.h"

CWebCacheCryptography::CWebCacheCryptography(void)
{
}

CWebCacheCryptography::~CWebCacheCryptography(void)
{
}

void CWebCacheCryptography::RefreshRemoteKey()
{	// the remote key is calculated of the master and slave keys
	if ( isProxy )
	{
		return;	// the remote key is set directly, no calculations possible or needed
	}
	else
	{
		for (int i=0; i<WC_KEYLENGTH;i++)
			remoteKey[i] = remoteMasterKey[i] ^ remoteSlaveKey[i];	// remoteKey = remoteMasterKey XOR remoteSlaveKey
	}
}

void CWebCacheCryptography::RefreshLocalKey()
{	// local key is calculated of the master and slave keys
	for (int i=0; i<WC_KEYLENGTH;i++)
		localKey[i] = localMasterKey[i] ^ localSlaveKey[i];	// localKey = localMasterKey XOR localSlaveKey
}