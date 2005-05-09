#pragma once

class CEnBitmap : public CBitmap  
{
public:
	CEnBitmap();
	virtual ~CEnBitmap();

	BOOL LoadImage(LPCTSTR pszImagePath, COLORREF crBack = 0);
	BOOL LoadImage(UINT uIDRes, LPCTSTR pszResourceType, HMODULE hInst = NULL, COLORREF crBack = 0); 
	BOOL LoadImage(LPCTSTR lpszResourceName, LPCTSTR pszResourceType, HMODULE hInst = NULL, COLORREF crBack = 0); 

	// helpers
	static BOOL GetResource(LPCTSTR lpName, LPCTSTR lpType, HMODULE hInst, void* pResource, int& nBufSize);
	static IPicture* LoadFromBuffer(BYTE* pBuff, int nSize);

protected:
	BOOL Attach(IPicture* pPicture, COLORREF crBack);

};
