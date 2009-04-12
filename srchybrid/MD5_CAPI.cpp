//
//	MD5_CAPI.cpp :	Take an MD5 hash over a buffer.
//

#include "stdafx.h"
#include "libald.h"
#include <wincrypt.h>

///////////////////////////////////////////////////////////////////////////////////
//	Use Microsoft CryptoAPI to generate an MD5 hash over the input 'buffer'
//	and put the 128-bit result (16 bytes) into the 'output' buffer.
//	Return TRUE unless one of the CryptoAPI (CAPI) functions fails.
//
BOOL MD5_CAPI(PBYTE buffer, int buffer_size, PBYTE output)
{
	BOOL bSuccess;
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	DWORD dwHashLen;

	bSuccess = CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);

	if (bSuccess) {
		bSuccess = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);

		if (bSuccess) {
			bSuccess = CryptHashData(hHash, buffer, buffer_size, 0);

			if (bSuccess) {
				dwHashLen = 16;

				bSuccess = CryptGetHashParam(hHash, HP_HASHVAL, output, &dwHashLen, 0);
			}

			CryptDestroyHash(hHash);
		}

		CryptReleaseContext(hProv, 0);
	}

	return bSuccess;
}
