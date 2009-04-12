// SetVistaIcon.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

typedef std::vector<std::pair<LPCTSTR, WORD> > CExecutableRscs;

BOOL CALLBACK EnumRscLangs(HMODULE UNREF(a_hModule), LPCTSTR UNREF(a_pszType), LPCTSTR a_pszName, WORD a_wIDLanguage, LONG_PTR a_lParam)
{
	CExecutableRscs* pIcons = reinterpret_cast<CExecutableRscs*>(a_lParam);
	if (IS_INTRESOURCE(a_pszName))
	{
		pIcons->push_back(std::make_pair(a_pszName, a_wIDLanguage));
	}
	else
	{
		LPCTSTR pszCopy = _tcsdup(a_pszName);
		if (pszCopy != NULL)
		{
			pIcons->push_back(std::make_pair(pszCopy, a_wIDLanguage));
		}
	}
	return TRUE;
}
BOOL CALLBACK EnumRscs(HMODULE a_hModule, LPCTSTR a_pszType, LPTSTR a_pszName, LONG_PTR a_lParam)
{
	return EnumResourceLanguages(a_hModule, a_pszType, a_pszName, EnumRscLangs, a_lParam);
}

void DeleteExecutableIcons(CExecutableRscs& a_cRscs)
{
	for (CExecutableRscs::iterator i = a_cRscs.begin(); i != a_cRscs.end(); i++)
	{
		if (!IS_INTRESOURCE(i->first))
			free(const_cast<LPTSTR>(i->first));
	}
	a_cRscs.clear();
}


#pragma pack( push )
#pragma pack( 2 )
typedef struct
{
   BYTE   bWidth;               // Width, in pixels, of the image
   BYTE   bHeight;              // Height, in pixels, of the image
   BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
   BYTE   bReserved;            // Reserved
   WORD   wPlanes;              // Color Planes
   WORD   wBitCount;            // Bits per pixel
   DWORD  dwBytesInRes;         // how many bytes in this resource?
   WORD   nID;                  // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;

typedef struct 
{
   WORD            idReserved;   // Reserved (must be 0)
   WORD            idType;       // Resource type (1 for icons)
   WORD            idCount;      // How many images?
   GRPICONDIRENTRY idEntries[1]; // The entries for each image
} GRPICONDIR, *LPGRPICONDIR;

typedef struct
{
   BYTE   bWidth;
   BYTE   bHeight;
   BYTE   bColorCount;
   BYTE   bReserved;
   WORD   wPlanes;
   WORD   wBitCount;
   DWORD  dwBytesInRes;
   DWORD  dwFileOffset;
} FILEGRPICONDIRENTRY;

typedef struct 
{
   WORD                idReserved;
   WORD                idType;
   WORD                idCount;
   FILEGRPICONDIRENTRY idEntries[1];
} FILEGRPICONDIR;

#pragma pack( pop )

void ReplaceIcon(BYTE const* a_pData, HANDLE a_hRes, LPCTSTR a_pszResID, WORD a_wLangID, CExecutableRscs const& a_cUsedImageIDs, std::set<UINT>& a_cFreeImageIDs)
{
	UINT nNextImageID = 1;
	FILEGRPICONDIR const* pFile = reinterpret_cast<FILEGRPICONDIR const*>(a_pData);
	int nDirSize = sizeof(GRPICONDIR)+(int(pFile->idCount)-1)*sizeof(GRPICONDIRENTRY);
	GRPICONDIR* pHead = reinterpret_cast<GRPICONDIR*>(new BYTE[nDirSize]);
	pHead->idReserved = pFile->idReserved;
	pHead->idType = pFile->idType;
	pHead->idCount = pFile->idCount;
	for (WORD i = 0; i < pFile->idCount; ++i)
	{
		pHead->idEntries[i].bWidth = pFile->idEntries[i].bWidth;
		pHead->idEntries[i].bHeight = pFile->idEntries[i].bHeight;
		pHead->idEntries[i].bColorCount = pFile->idEntries[i].bColorCount;
		pHead->idEntries[i].bReserved = pFile->idEntries[i].bReserved;
		pHead->idEntries[i].wPlanes = pFile->idEntries[i].wPlanes;
		pHead->idEntries[i].wBitCount = pFile->idEntries[i].wBitCount;
		pHead->idEntries[i].dwBytesInRes = pFile->idEntries[i].dwBytesInRes;
		if (a_cFreeImageIDs.empty())
		{
			// find free id
			for (CExecutableRscs::const_iterator j = a_cUsedImageIDs.begin(); j != a_cUsedImageIDs.end(); ++j)
			{
				if (UINT(j->first) == nNextImageID)
				{
					++nNextImageID;
					j = a_cUsedImageIDs.begin();
				}
			}
			pHead->idEntries[i].nID = nNextImageID;
			++nNextImageID;
		}
		else
		{
			pHead->idEntries[i].nID = *a_cFreeImageIDs.begin();
			a_cFreeImageIDs.erase(a_cFreeImageIDs.begin());
		}
	}

	UpdateResource(a_hRes, RT_GROUP_ICON, a_pszResID, a_wLangID, pHead, nDirSize);
	for (WORD i = 0; i < pFile->idCount; ++i)
	{
		UpdateResource(a_hRes, RT_ICON, MAKEINTRESOURCE(pHead->idEntries[i].nID), a_wLangID, const_cast<BYTE*>(a_pData)+pFile->idEntries[i].dwFileOffset, pFile->idEntries[i].dwBytesInRes);
	}
	for (std::set<UINT>::const_iterator i = a_cFreeImageIDs.begin(); i != a_cFreeImageIDs.end(); ++i)
	{
		UpdateResource(a_hRes, RT_ICON, MAKEINTRESOURCE(*i), a_wLangID, NULL, 0);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3 || argc > 5)
	{
		_tprintf(_T("\nUsage: SetVistaIcon.exe path_to_executable path_to_icon [icon_ID [language_ID]]\n\nMore info: http://www.rw-designer.com/compile-vista-icon\n\n"));
		return 1;
	}

	HMODULE hMod = LoadLibraryEx(argv[1], NULL, LOAD_LIBRARY_AS_DATAFILE);
	if (NULL == hMod)
	{
		_tprintf(_T("\nFailed to open \"%s\"\n\n"), argv[1]);
		return 1;
	}

	CExecutableRscs cIconGroups;
	EnumResourceNames(hMod, RT_GROUP_ICON, EnumRscs, reinterpret_cast<LONG_PTR>(&cIconGroups));
	//for (CExecutableRscs::const_iterator i = cIconGroups.begin(); i != cIconGroups.end(); ++i)
	//{
	//	_tprintf(_T("Icon group %i, %i\n"), int(i->first), int(i->second));
	//}

	CExecutableRscs cIconImages;
	EnumResourceNames(hMod, RT_ICON, EnumRscs, reinterpret_cast<LONG_PTR>(&cIconImages));
	//for (CExecutableRscs::const_iterator i = cIconImages.begin(); i != cIconImages.end(); ++i)
	//{
	//	_tprintf(_T("Icon image %i, %i\n"), int(i->first), int(i->second));
	//}

	LPCTSTR resid;
	WORD langid;
	if (argc > 3)
	{
		int x = _ttoi(argv[3]);
		resid = x ? MAKEINTRESOURCE(x) : argv[3];
		if (argc > 4)
		{
			langid = _ttoi(argv[4]);
		}
		else
		{
			langid = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
			for (CExecutableRscs::const_iterator i = cIconGroups.begin(); i != cIconGroups.end(); ++i)
			{
				if (!IS_INTRESOURCE(i->first) && !IS_INTRESOURCE(resid) ? 0 == _tcscmp(i->first, resid) : i->first == resid)
				{
					langid = i->second;
					break;
				}
			}
		}
	}
	else
	{
		if (cIconGroups.size())
		{
			resid = cIconGroups[0].first;
			langid = cIconGroups[0].second;
		}
		else
		{
			resid = MAKEINTRESOURCE(101);
			langid = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
		}
	}

	std::set<UINT> cImagesToRemove;
	HRSRC hIconDir = FindResourceEx(hMod, RT_GROUP_ICON, resid, langid);
	if (hIconDir)
	{
		HGLOBAL hDirMem = LoadResource(hMod, hIconDir);
		LPGRPICONDIR pDirMem = reinterpret_cast<LPGRPICONDIR>(LockResource(hDirMem));
		for (WORD i = 0; i != pDirMem->idCount; i++)
		{
			cImagesToRemove.insert(pDirMem->idEntries[i].nID);
		}
	}

	FreeLibrary(hMod);

	// access resources in executable file
	HANDLE hRes = BeginUpdateResource(argv[1], FALSE);
	if (NULL == hRes)
	{
		_tprintf(_T("\nFailed to access resources in \"%s\"\n\n"), argv[1]);
		return 1;
	}

	// read the icon file
	HANDLE hIconFile = CreateFile(argv[2], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (INVALID_HANDLE_VALUE == hIconFile)
	{
		_tprintf(_T("\nFailed to open icon file \"%s\"\n\n"), argv[2]);
		EndUpdateResource(hRes, TRUE);
		return 1;
	}
	DWORD dwSize = GetFileSize(hIconFile, NULL);
	CAutoVectorPtr<BYTE> pData(new BYTE[dwSize]);
	DWORD dwRead = 0;
	if (pData)
		ReadFile(hIconFile, pData, dwSize, &dwRead, NULL);
	CloseHandle(hIconFile);
	if (dwSize == 0 || dwSize != dwRead)
	{
		_tprintf(_T("\nFailed to read icon file \"%s\"\n\n"), argv[2]);
		if (pData) free(pData);
		EndUpdateResource(hRes, TRUE);
		return 1;
	}
	ReplaceIcon(pData, hRes, resid, langid, cIconImages, cImagesToRemove);
	if (FALSE == EndUpdateResource(hRes, FALSE))
	{
		_tprintf(_T("\nFailed to update resources\n\n"));
		return 1;
	}

	return 0;
}

