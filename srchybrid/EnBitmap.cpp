// EnBitmap.cpp: implementation of the CEnBitmap class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EnBitmap.h"
#include <atlimage.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

const int HIMETRIC_INCH	= 2540;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEnBitmap::CEnBitmap()
{

}

CEnBitmap::~CEnBitmap()
{

}

BOOL CEnBitmap::LoadImage(UINT uIDRes, LPCTSTR szResourceType, HMODULE hInst, COLORREF crBack)
{
	ASSERT(m_hObject == NULL);      // only attach once, detach on destroy

	if (m_hObject != NULL)
		return FALSE;

	BYTE*	pBuff = NULL;
	int		nSize = 0;
	BOOL bResult = FALSE;

	// first call is to get buffer size
	if (GetResource(MAKEINTRESOURCE(uIDRes), szResourceType, hInst, 0, nSize))
	{
		if (nSize > 0)
		{
			pBuff = new BYTE[nSize];
			
			// this loads it
			if (GetResource(MAKEINTRESOURCE(uIDRes), szResourceType, hInst, pBuff, nSize))
			{
				IPicture* pPicture = LoadFromBuffer(pBuff, nSize);

				if (pPicture)
				{
					bResult = Attach(pPicture, crBack);
					pPicture->Release();
				}
			}
			
			delete [] pBuff;
		}
	}

	return bResult;
}

BOOL CEnBitmap::LoadImage(LPCTSTR szImagePath, COLORREF crBack)
{
	ASSERT(m_hObject == NULL);      // only attach once, detach on destroy

	if (m_hObject != NULL)
		return FALSE;

	// If GDI+ is available, use that API because it supports more file formats and images with alpha channels.
	// That DLL is installed with WinXP is available as redistributable from Microsoft for Win98+. As this DLL
	// may not be available on some OS but we have to link statically to it, we have to take some special care.
	//
	// NOTE: Do *NOT* forget to specify /DELAYLOAD:gdiplus.dll as link parameter.
	static int _bGdiPlusInstalled = -1;
	if (_bGdiPlusInstalled == -1)
	{
		_bGdiPlusInstalled = 0;
		HMODULE hLib = LoadLibrary(_T("gdiplus.dll"));
		if (hLib != NULL)
		{
			_bGdiPlusInstalled = GetProcAddress(hLib, _T("GdiplusStartup")) != NULL;
			FreeLibrary(hLib);
		}
	}
	if (_bGdiPlusInstalled > 0)
	{
		CImage img;
		if (SUCCEEDED(img.Load(szImagePath)))
		{
			CBitmap::Attach(img.Detach());
			return TRUE;
		}
	}

	BOOL bResult = FALSE;
	CFile			cFile;
	CFileException	e;
	
	if (cFile.Open(szImagePath, CFile::modeRead | CFile::typeBinary, &e))
	{
		int nSize = cFile.GetLength();

		BYTE* pBuff = new BYTE[nSize];
		
		if (cFile.Read(pBuff, nSize) > 0)
		{
			IPicture* pPicture = LoadFromBuffer(pBuff, nSize);
			
			if (pPicture)
			{
				bResult = Attach(pPicture, crBack);
				pPicture->Release();
			}
		}
		
		delete [] pBuff;
	}

	return bResult;
}

IPicture* CEnBitmap::LoadFromBuffer(BYTE* pBuff, int nSize)
{
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, nSize);
	void* pData = GlobalLock(hGlobal);
	memcpy(pData, pBuff, nSize);
	GlobalUnlock(hGlobal);

	IStream* pStream = NULL;
	IPicture* pPicture = NULL;

	if (CreateStreamOnHGlobal(hGlobal, TRUE, &pStream) == S_OK)
	{
		OleLoadPicture(pStream, nSize, FALSE, IID_IPicture, (LPVOID *)&pPicture);
		pStream->Release();
	}

	return pPicture; // caller releases
}

BOOL CEnBitmap::GetResource(LPCTSTR lpName, LPCTSTR lpType, HMODULE hInst, void* pResource, int& nBufSize)
{ 
	HRSRC		hResInfo;
	HANDLE		hRes;
	LPSTR		lpRes	= NULL; 
	bool		bResult	= FALSE;

	// Find the resource
	hResInfo = FindResource(hInst, lpName, lpType);

	if (hResInfo == NULL) 
		return false;

	// Load the resource
	hRes = LoadResource(hInst, hResInfo);

	if (hRes == NULL) 
		return false;

	// Lock the resource
	lpRes = (char*)LockResource(hRes);

	if (lpRes != NULL)
	{ 
		if (pResource == NULL)
		{
			nBufSize = SizeofResource(hInst, hResInfo);
			bResult = true;
		}
		else
		{
			if (nBufSize >= (int)SizeofResource(hInst, hResInfo))
			{
				memcpy(pResource, lpRes, nBufSize);
				bResult = true;
			}
		} 

		UnlockResource(hRes);  
	}

	// Free the resource
	FreeResource(hRes);

	return bResult;
}

BOOL CEnBitmap::Attach(IPicture* pPicture, COLORREF crBack)
{
	ASSERT(m_hObject == NULL);      // only attach once, detach on destroy

	if (m_hObject != NULL)
		return FALSE;

	ASSERT(pPicture);

	if (!pPicture)
		return FALSE;

	BOOL bResult = FALSE;

	CDC dcMem;
	CDC* pDC = CWnd::GetDesktopWindow()->GetDC();

	if (dcMem.CreateCompatibleDC(pDC))
	{
		long hmWidth;
		long hmHeight;

		pPicture->get_Width(&hmWidth);
		pPicture->get_Height(&hmHeight);
		
		int nWidth	= MulDiv(hmWidth,	pDC->GetDeviceCaps(LOGPIXELSX), HIMETRIC_INCH);
		int nHeight	= MulDiv(hmHeight,	pDC->GetDeviceCaps(LOGPIXELSY), HIMETRIC_INCH);

		CBitmap bmMem;

		if (bmMem.CreateCompatibleBitmap(pDC, nWidth, nHeight))
		{
			CBitmap* pOldBM = dcMem.SelectObject(&bmMem);

			if (crBack != -1)
				dcMem.FillSolidRect(0, 0, nWidth, nHeight, crBack);
			
			HRESULT hr = pPicture->Render(dcMem, 0, 0, nWidth, nHeight, 0, hmHeight, hmWidth, -hmHeight, NULL);
			dcMem.SelectObject(pOldBM);

			if (hr == S_OK)
				bResult = CBitmap::Attach(bmMem.Detach());
		}
	}

	CWnd::GetDesktopWindow()->ReleaseDC(pDC);

	return bResult;
}

