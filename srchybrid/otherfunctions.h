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
#include "KnownFile.h"
#include "PartFile.h"
#include <math.h>

#define _WINVER_NT4_	0x0004
#define _WINVER_95_		0x0004
#define _WINVER_98_		0x0A04
#define _WINVER_ME_		0x5A04
#define _WINVER_2K_		0x0005
#define _WINVER_XP_		0x0105

#define ROUND(x) (floor((float)x+0.5f))

#ifdef _DEBUG
#define OUTPUT_DEBUG_TRACE() {(void)0;}
#else
#define OUTPUT_DEBUG_TRACE() ((void)0)
#endif

#define ELEMENT_COUNT(X) (sizeof(X) / sizeof(X[0]))

enum EED2KFileType
{
	ED2KFT_ANY,
	ED2KFT_AUDIO,
	ED2KFT_VIDEO,
	ED2KFT_IMAGE,
	ED2KFT_PROGRAM,
	ED2KFT_DOCUMENT,
	ED2KFT_ARCHIVE,
	ED2KFT_CDIMAGE
};


CString CastItoXBytes(uint64 count);
// khaos::categorymod+ Takes a string and returns bytes...
ULONG	CastXBytesToI(const CString& strExpr);
// Takes bytes and returns a string with only integers...
CString CastItoUIXBytes(uint32 count);
// khaos::categorymod-
CString CastItoIShort(uint64 number);
CString CastSecondsToHM(sint32 seconds); //<<--9/21/02

// -khaos--+++>
CString	CastSecondsToLngHM(__int64 count);
// <-----khaos-

CString LeadingZero(uint32 units);
void ShellOpenFile(CString name); //<<--9/21/02
void ShellOpenFile(CString name, LPCTSTR pszVerb);

// khaos::kmod+ Random number generator using ranges.
int			GetRandRange( int from, int to);
void		InitRandGen();
int			RandNum();
static int	rgiState[2+55];
// Wildcard support for Category Selection Mask
int wildcmp(char *wild, char *string);
// khaos::kmod-

#ifdef USE_STRING_IDS
#define	RESSTRIDTYPE		LPCTSTR
#define	IDS2RESIDTYPE(id)	#id
#define GetResString(id)	_GetResString(#id)
CString _GetResString(RESSTRIDTYPE StringID);
#else//USE_STRING_IDS
#define	RESSTRIDTYPE		UINT
#define	IDS2RESIDTYPE(id)	id
CString GetResString(RESSTRIDTYPE StringID);
#define _GetResString(id)	GetResString(id)
#endif//!USE_STRING_IDS

__inline char* nstrdup(const char* todup){
   size_t len = strlen(todup) + 1;
   return (char*)memcpy(new char[len], todup, len);
}

CHAR *stristr(const CHAR *str1, const CHAR *str2);
int GetSystemErrorString(DWORD dwError, CString &rstrError);
int GetModuleErrorString(DWORD dwError, CString &rstrError, LPCTSTR pszModule);
int GetErrorMessage(DWORD dwError, CString &rstrErrorMsg, DWORD dwFlags = 0);
CString GetErrorMessage(DWORD dwError, DWORD dwFlags = 0);
CString GetHexDump(const uint8* data, UINT size);
void DbgSetThreadName(LPCSTR szThreadName, ...);
void Debug(LPCTSTR pszFmtMsg, ...);
void DebugHexDump(const uint8* data, UINT lenData);
void DebugHexDump(CFile& file);
CString DbgGetFileInfo(const uchar* hash);
LPCTSTR DbgGetHashTypeString(const uchar* hash);
CString DbgGetClientID(uint32 nClientID);
CString GetFormatedUInt(ULONG ulVal);
void SecToTimeLength(unsigned long ulSec, CStringA& rstrTimeLength);
ULONGLONG GetDiskFileSize(LPCTSTR pszFilePath);

void URLDecode(CString& result, const char* buff); // Make a malloc'd decoded strnig from an URL encoded string (with escaped spaces '%20' and  the like
CString URLDecode(CString sIn);
CString URLEncode(CString sIn);

__inline BYTE toHex(const BYTE &x){
	return x > 9 ? x + 55: x + 48;
}

bool Ask4RegFix(bool checkOnly, bool dontAsk = false); // Barry - Allow forced update without prompt
void BackupReg(void); // Barry - Store previous values
void RevertReg(void); // Barry - Restore previous values
int GetMaxConnections();
CString MakeStringEscaped(CString in);
void RunURL(CAbstractFile* file,CString urlpattern);

WORD	DetectWinVersion();
uint64	GetFreeDiskSpaceX(LPCTSTR pDirectory);
//For Rate File 
CString GetRateString(uint16 rate);
void	UpdateURLMenu(CMenu &menu, int &counter);
int		GetAppImageListColorFlag();

// From Gnucleus project [found by Tarod]
CString EncodeBase32(const unsigned char* buffer, unsigned int bufLen);
CString EncodeBase16(const unsigned char* buffer, unsigned int bufLen);
int	DecodeLengthBase16(int base16Length);
void DecodeBase16(const char *base16Buffer, unsigned int base16BufLen, byte *buffer);
bool SelectDir(HWND hWnd, LPTSTR pszPath, LPCTSTR pszTitle = NULL, LPCTSTR pszDlgTitle = NULL);
void MakeFoldername(char* path);
CString RemoveFileExtension(const CString& rstrFilePath);
int CompareDirectories(const CString& rstrDir1, const CString& rstrDir2);
CString StringLimit(CString in,uint16 length);
BOOL DialogBrowseFile(CString& rstrPath, LPCTSTR pszFilters, LPCTSTR pszDefaultFileName = NULL, DWORD dwFlags = 0);
bool CheckShowItemInGivenCat(CPartFile* file,int inCategory);
uint8 GetRealPrio(uint8 in);

CString CleanupFilename(CString filename);
// khaos::categorymod+ Obsolete CString GetCatTitle(int catid);

// md4cmp -- replacement for memcmp(hash1,hash2,16)
// Like 'memcmp' this function returns 0, if hash1==hash2, and !0, if hash1!=hash2.
// NOTE: Do *NOT* use that function for determining if hash1<hash2 or hash1>hash2.
__inline int md4cmp(const void* hash1, const void* hash2) {
	return !(((uint32*)hash1)[0] == ((uint32*)hash2)[0] &&
		     ((uint32*)hash1)[1] == ((uint32*)hash2)[1] &&
		     ((uint32*)hash1)[2] == ((uint32*)hash2)[2] &&
		     ((uint32*)hash1)[3] == ((uint32*)hash2)[3]);
}

// md4clr -- replacement for MEMSET(hash,0,16)
__inline void md4clr(const void* hash) {
	((uint32*)hash)[0] = ((uint32*)hash)[1] = ((uint32*)hash)[2] = ((uint32*)hash)[3] = 0;
}

//Morph Start - commented by AndCycle, eMulePlus CPU optimize
/*	//original commented
// md4cpy -- replacement for memcpy(dst,src,16)
__inline void md4cpy(void* dst, const void* src) {
	((uint32*)dst)[0] = ((uint32*)src)[0];
	((uint32*)dst)[1] = ((uint32*)src)[1];
	((uint32*)dst)[2] = ((uint32*)src)[2];
	((uint32*)dst)[3] = ((uint32*)src)[3];
}
*/
//Morph End - commented by AndCycle, eMulePlus CPU optimize

#define	MAX_HASHSTR_SIZE (16*2+1)
CString md4str(const uchar* hash);
void md4str(const uchar* hash, char* pszHash);
bool strmd4(const char* pszHash, uchar* hash);
bool strmd4(const CString& rstr, uchar* hash);

__inline int CompareUnsigned(uint32 uSize1, uint32 uSize2)
{
	if (uSize1 < uSize2)
		return -1;
	if (uSize1 > uSize2)
		return 1;
	return 0;
}

__inline int CompareOptStringNoCase(LPCTSTR psz1, LPCTSTR psz2)
{
	if (psz1 && psz2)
		return _tcsicmp(psz1, psz2);
	if (psz1)
		return -1;
	if (psz2)
		return 1;
	return 0;
}

CString GetFiletypeByName(LPCTSTR pszFileName);
LPCSTR GetED2KFileTypeSearchTerm(EED2KFileType iFileID);
EED2KFileType GetED2KFileTypeID(LPCTSTR pszFileName);

bool IsGoodIP(uint32 nIP, bool forceCheck = false); //MORPH - Modified by SiRoB, ZZ Upload system (USS)
bool IsGoodIPPort(uint32 nIP, uint16 nPort);

bool	IsLowIDHybrid(uint32 id);
bool	IsLowIDED2K(uint32 id);
