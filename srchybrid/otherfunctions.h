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

class CAbstractFile;
struct Requested_Block_Struct;
class CUpDownClient;

#define ROUND(x) (floor((float)x+0.5f))


///////////////////////////////////////////////////////////////////////////////
// Low level str
//
__inline char* nstrdup(const char* todup){
   size_t len = strlen(todup) + 1;
   return (char*)memcpy(new char[len], todup, len);
}

CHAR *stristr(const CHAR *str1, const CHAR *str2);


///////////////////////////////////////////////////////////////////////////////
// String conversion
//
CString CastItoXBytes(uint64 count);
CString CastItoIShort(uint64 number);
CString CastSecondsToHM(sint32 seconds);
CString	CastSecondsToLngHM(__int64 count);
CString GetFormatedUInt(ULONG ulVal);
CString GetFormatedUInt64(ULONGLONG ullVal);
void SecToTimeLength(unsigned long ulSec, CStringA& rstrTimeLength);
CString LeadingZero(uint32 units);
// khaos::categorymod+ Takes a string and returns bytes...
ULONG	CastXBytesToI(const CString& strExpr);
// Takes bytes and returns a string with only integers...
CString CastItoUIXBytes(uint32 count);
// khaos::categorymod-

// khaos::kmod+ Random number generator using ranges.
int			GetRandRange( int from, int to);
void		InitRandGen();
int			RandNum();
static int	rgiState[2+55];
// Wildcard support for Category Selection Mask
int wildcmp(char *wild, char *string);
// khaos::kmod-
//MORPH START - Added by SiRoB, XML News [O²]
void HTMLParse(CString &buffer); // Added by N_OxYdE
//MORPH END  - Added by SiRoB, XML News [O²]

///////////////////////////////////////////////////////////////////////////////
// URL conversion
//
void URLDecode(CString& result, const char* buff); // Make a malloc'd decoded strnig from an URL encoded string (with escaped spaces '%20' and  the like
CString URLDecode(CString sIn);
CString URLEncode(CString sIn);
CString MakeStringEscaped(CString in);
CString	StripInvalidFilenameChars(CString strText, bool bKeepSpaces = true);
CString	CreateED2kLink(const CAbstractFile* f);
CString	CreateHTMLED2kLink(const CAbstractFile* f);


///////////////////////////////////////////////////////////////////////////////
// Hex conversion
//
CString EncodeBase32(const unsigned char* buffer, unsigned int bufLen);
CString EncodeBase16(const unsigned char* buffer, unsigned int bufLen);
unsigned int DecodeLengthBase16(unsigned int base16Length);
bool DecodeBase16(const char *base16Buffer, unsigned int base16BufLen, byte *buffer, unsigned int bufflen);


///////////////////////////////////////////////////////////////////////////////
// File/Path string helpers
//
void MakeFoldername(char* path);
CString RemoveFileExtension(const CString& rstrFilePath);
int CompareDirectories(const CString& rstrDir1, const CString& rstrDir2);
CString StringLimit(CString in,uint16 length);
CString CleanupFilename(CString filename);
bool ExpandEnvironmentStrings(CString& rstrStrings);


///////////////////////////////////////////////////////////////////////////////
// GUI helpers
//
void ShellOpenFile(CString name);
void ShellOpenFile(CString name, LPCTSTR pszVerb);
bool SelectDir(HWND hWnd, LPTSTR pszPath, LPCTSTR pszTitle = NULL, LPCTSTR pszDlgTitle = NULL);
BOOL DialogBrowseFile(CString& rstrPath, LPCTSTR pszFilters, LPCTSTR pszDefaultFileName = NULL, DWORD dwFlags = 0,bool openfilestyle=true);
void GetPopupMenuPos(CListCtrl& lv, CPoint& point);
void GetPopupMenuPos(CTreeCtrl& tv, CPoint& point);
void InitWindowStyles(CWnd* pWnd);
CString GetRateString(uint16 rate);
HWND GetComboBoxEditCtrl(CComboBox& cb);


///////////////////////////////////////////////////////////////////////////////
// Resource strings
//
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


///////////////////////////////////////////////////////////////////////////////
// Error strings, Debugging, Logging
//
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
int GetHashType(const uchar* hash);
CString DbgGetDonkeyClientTCPOpcode(UINT opcode);
CString DbgGetMuleClientTCPOpcode(UINT opcode);
CString DbgGetClientTCPOpcode(UINT protocol, UINT opcode);
CString DbgGetClientTCPPacket(UINT protocol, UINT opcode, UINT size);
CString DbgGetBlockInfo(const Requested_Block_Struct* block);
void DebugRecv(LPCTSTR pszMsg, const CUpDownClient* client, const char* packet = NULL, uint32 nIP = 0);
void DebugSend(LPCTSTR pszMsg, const CUpDownClient* client, const char* packet = NULL);



///////////////////////////////////////////////////////////////////////////////
// Win32 specifics
//
bool HaveEd2kRegAccess();
bool Ask4RegFix(bool checkOnly, bool dontAsk = false); // Barry - Allow forced update without prompt
void BackupReg(void); // Barry - Store previous values
void RevertReg(void); // Barry - Restore previous values

int GetMaxWindowsTCPConnections();

#define _WINVER_NT4_	0x0004
#define _WINVER_95_		0x0004
#define _WINVER_98_		0x0A04
#define _WINVER_ME_		0x5A04
#define _WINVER_2K_		0x0005
#define _WINVER_XP_		0x0105
WORD DetectWinVersion();
uint64 GetFreeDiskSpaceX(LPCTSTR pDirectory);
ULONGLONG GetDiskFileSize(LPCTSTR pszFilePath);
int GetAppImageListColorFlag();


///////////////////////////////////////////////////////////////////////////////
// MD4 helpers
//

__inline BYTE toHex(const BYTE &x){
	return x > 9 ? x + 55: x + 48;
}

// md4cmp -- replacement for memcmp(hash1,hash2,16)
// Like 'memcmp' this function returns 0, if hash1==hash2, and !0, if hash1!=hash2.
// NOTE: Do *NOT* use that function for determining if hash1<hash2 or hash1>hash2.
__inline int md4cmp(const void* hash1, const void* hash2) {
	return !(((uint32*)hash1)[0] == ((uint32*)hash2)[0] &&
		     ((uint32*)hash1)[1] == ((uint32*)hash2)[1] &&
		     ((uint32*)hash1)[2] == ((uint32*)hash2)[2] &&
		     ((uint32*)hash1)[3] == ((uint32*)hash2)[3]);
}

// md4clr -- replacement for memset(hash,0,16)
__inline void md4clr(const void* hash) {
	((uint32*)hash)[0] = ((uint32*)hash)[1] = ((uint32*)hash)[2] = ((uint32*)hash)[3] = 0;
}

// md4cpy -- replacement for memcpy(dst,src,16)
__inline void md4cpy(void* dst, const void* src) {
	((uint32*)dst)[0] = ((uint32*)src)[0];
	((uint32*)dst)[1] = ((uint32*)src)[1];
	((uint32*)dst)[2] = ((uint32*)src)[2];
	((uint32*)dst)[3] = ((uint32*)src)[3];
}

#define	MAX_HASHSTR_SIZE (16*2+1)
CString md4str(const uchar* hash);
void md4str(const uchar* hash, char* pszHash);
bool strmd4(const char* pszHash, uchar* hash);
bool strmd4(const CString& rstr, uchar* hash);


///////////////////////////////////////////////////////////////////////////////
// Compar helpers
//
__inline int CompareUnsigned(uint32 uSize1, uint32 uSize2)
{
	if (uSize1 < uSize2)
		return -1;
	if (uSize1 > uSize2)
		return 1;
	return 0;
}

__inline int CompareUnsigned64(uint64 uSize1, uint64 uSize2)
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


///////////////////////////////////////////////////////////////////////////////
// ED2K File Type
//
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

CString GetFiletypeByName(LPCTSTR pszFileName);
LPCSTR GetED2KFileTypeSearchTerm(EED2KFileType iFileID);
EED2KFileType GetED2KFileTypeID(LPCTSTR pszFileName);


///////////////////////////////////////////////////////////////////////////////
// IP/UserID
//
bool IsGoodIP(uint32 nIP, bool forceCheck = false);
bool IsGoodIPPort(uint32 nIP, uint16 nPort);
//No longer need seperate lowID checks as we now know the servers just give *.*.*.0 users a lowID
__inline bool IsLowID(uint32 id){
	return (id < 16777216);
}
CString ipstr(uint32 nIP);


///////////////////////////////////////////////////////////////////////////////
// Date/Time
//
time_t safe_mktime(struct tm* ptm);
bool AdjustNTFSDaylightFileTime(uint32& ruFileDate, LPCTSTR pszFilePath);
