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

#include "stdafx.h"
#include "otherfunctions.h"
#include "emule.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// Base chars for encode an decode functions
static byte base16Chars[17] = "0123456789ABCDEF";
static byte base32Chars[33] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
#define BASE16_LOOKUP_MAX 23
static byte base16Lookup[BASE16_LOOKUP_MAX][2] = {
    { '0', 0x0 },
    { '1', 0x1 },
    { '2', 0x2 },
    { '3', 0x3 },
    { '4', 0x4 },
    { '5', 0x5 },
    { '6', 0x6 },
    { '7', 0x7 },
    { '8', 0x8 },
    { '9', 0x9 },
	{ ':', 0x9 },
    { ';', 0x9 },
    { '<', 0x9 },
    { '=', 0x9 },
    { '>', 0x9 },
    { '?', 0x9 },
    { '@', 0x9 },
    { 'A', 0xA },
    { 'B', 0xB },
    { 'C', 0xC },
    { 'D', 0xD },
    { 'E', 0xE },
    { 'F', 0xF }
};

CString CastItoXBytes(uint64 count){
	CString buffer;
	if (count < 1024)
		buffer.Format("%.0f %s",(float)count,GetResString(IDS_BYTES));
	else if (count < 1048576)
		buffer.Format("%.0f %s",(float)count/1024,GetResString(IDS_KBYTES));
	else if (count < 1073741824)
		buffer.Format("%.2f %s",(float)count/1048576,GetResString(IDS_MBYTES));
	else if (count < 1099511627776)
		buffer.Format("%.2f %s",(float)count/1073741824,GetResString(IDS_GBYTES));
	else 
		buffer.Format("%.3f %s",(float)count/1099511627776,GetResString(IDS_TBYTES));
	return buffer;
}

CString CastItoIShort(uint64 count){
	CString output;
	if (count < 1000)
		output.Format("%i",count);
	else if (count < 1000000)
		output.Format("%.0f%s",(float)count/1000, GetResString(IDS_KILO));
	else if (count < 1000000000)
		output.Format("%.2f%s",(float)count/1000000, GetResString(IDS_MEGA));
	else if (count < 1000000000000)
		output.Format("%.2f%s",(float)count/1000000000, GetResString(IDS_GIGA));
	else if (count < 1000000000000000)
		output.Format("%.2f%s",(float)count/1000000000000, GetResString(IDS_TERRA));
	return output;
}

CString CastSecondsToHM(sint32 count){
	CString buffer;
	if (count < 0)
		buffer = "?"; 
	else if (count < 60)
		buffer.Format("%i %s",count,GetResString(IDS_SECS)); 
	else if (count < 3600) 
		buffer.Format("%i:%s %s",count/60,LeadingZero(count-(count/60)*60),GetResString(IDS_MINS));
	else if (count < 86400) 
		buffer.Format("%i:%s %s",count/3600,LeadingZero((count-(count/3600)*3600)/60),GetResString(IDS_HOURS));
	else 
		buffer.Format("%i %s %i %s",count/86400,GetResString(IDS_DAYS),(count-(count/86400)*86400)/3600,GetResString(IDS_HOURS)); 
	return buffer;
}

// -khaos--+++> Prettier.
CString CastSecondsToLngHM(__int64 count){
	CString buffer;
	if (count < 0)
		buffer = "?"; 
	else if (count < 60)
		buffer.Format("%I64d %s",count,GetResString(IDS_LONGSECS)); 
	else if (count < 3600) 
		buffer.Format("%I64d:%s %s",count/60,LeadingZero(count-(count/60)*60),GetResString(IDS_LONGMINS));
	else if (count < 86400) 
		buffer.Format("%I64d:%s %s",count/3600,LeadingZero((count-(count/3600)*3600)/60),GetResString(IDS_LONGHRS));
	else {
		__int64 cntDays = count/86400;
		__int64 cntHrs = (count-(count/86400)*86400)/3600;
		if (cntHrs)
			buffer.Format("%I64d %s %I64d:%s %s",cntDays,GetResString(IDS_DAYS2),cntHrs,LeadingZero((uint32)(count-(cntDays*86400)-(cntHrs*3600))/60),GetResString(IDS_LONGHRS)); 
		else
			buffer.Format("%I64d %s %u %s",cntDays,GetResString(IDS_DAYS2),(uint32)(count-(cntDays*86400)-(cntHrs*3600))/60,GetResString(IDS_LONGMINS));
	}
	return buffer;
} 
// <-----khaos-

CString LeadingZero(uint32 units) {
	CString temp;
	if (units<10) temp.Format("0%i",units); else temp.Format("%i",units);
	return temp;
}

//<<--9/21/02
void ShellOpenFile(CString name){ 
    ShellExecute(NULL, "open", name, NULL, NULL, SW_SHOW); 
} 
void ShellOpenFile(CString name, LPCTSTR pszVerb){ 
    ShellExecute(NULL, pszVerb, name, NULL, NULL, SW_SHOW); 
} 

namespace {
	bool IsHexDigit(int c) {
		switch (c) {
		case '0': return true;
		case '1': return true;
		case '2': return true;
		case '3': return true;
		case '4': return true;
		case '5': return true;
		case '6': return true;
		case '7': return true;
		case '8': return true;
		case '9': return true;
		case 'A': return true;
		case 'B': return true;
		case 'C': return true;
		case 'D': return true;
		case 'E': return true;
		case 'F': return true;
		case 'a': return true;
		case 'b': return true;
		case 'c': return true;
		case 'd': return true;
		case 'e': return true;
		case 'f': return true;
		default: return false;
		}
	}
}

CString URLDecode(CString inStr) {
	
	CString res="";

	for (int x = 0; x < inStr.GetLength() ; ++x )
	{
		if ( inStr.GetAt(x)== '%' && x+2 < inStr.GetLength() && IsHexDigit(inStr.GetAt(x+1)) && IsHexDigit(inStr.GetAt(x+2)) ) {

			char hexstr[3]; hexstr[2]=0;
			// Copy the two bytes following the %
			strncpy(hexstr, inStr.Mid(x+1,2).GetBuffer(), 2);

			// Skip over the hex
			x = x + 2;

			// Convert the hex to ASCII
			res.AppendChar((unsigned char)strtoul(hexstr, NULL, 16));
		}
		else {
			res.AppendChar( inStr.GetAt(x));
			//break;
		}
	}
	return res;
}

void URLDecode(CString& result, const char* buff)
{
	int buflen = (int)strlen(buff);
	int x;
	int y;
	char* buff2 = nstrdup(buff); // length of buff2 will be less or equal to length of buff
	for (x = 0, y = 0; x < buflen ; ++x )
	{
		if ( buff[x] == '%' && x+2 < buflen && IsHexDigit(buff[x+1]) && IsHexDigit(buff[x+2]) ) {
			char hexstr[3];
			// Copy the two bytes following the %
			strncpy(hexstr, &buff[x + 1], 2);

			// Skip over the hex
			x = x + 2;

			// Convert the hex to ASCII
			buff2[y++] = (unsigned char)strtoul(hexstr, NULL, 16);
		}
		else {
			buff2[y++] = buff[x];
			break;
		}
	}
	result = buff2;
	free(buff2);
}


CString URLEncode(CString sIn){
    CString sOut;
	
    const int nLen = sIn.GetLength() + 1;

    register LPBYTE pOutTmp = NULL;
    LPBYTE pOutBuf = NULL;
    register LPBYTE pInTmp = NULL;
    LPBYTE pInBuf =(LPBYTE)sIn.GetBuffer(nLen);
	
    //alloc out buffer
    pOutBuf = (LPBYTE)sOut.GetBuffer(nLen  * 3 - 2);//new BYTE [nLen  * 3];

    if(pOutBuf)
    {
        pInTmp	= pInBuf;
		pOutTmp = pOutBuf;
			
		// do encoding
		while (*pInTmp)
		{
			if(isalnum(*pInTmp))
				*pOutTmp++ = *pInTmp;
			else
				if(isspace(*pInTmp))
				*pOutTmp++ = '+';
			else
			{
				*pOutTmp++ = '%';
				*pOutTmp++ = toHex(*pInTmp>>4);
				*pOutTmp++ = toHex(*pInTmp%16);
			}
			pInTmp++;
		}
		*pOutTmp = '\0';
		//sOut=pOutBuf;
		//delete [] pOutBuf;
		sOut.ReleaseBuffer();
    }
    sIn.ReleaseBuffer();
    return sOut;
}

CString MakeStringEscaped(CString in) {
	in.Replace("&","&&");
	
	return in;
}


bool Ask4RegFix(bool checkOnly, bool dontAsk){

	// Barry - Make backup first
	if (!checkOnly)
		BackupReg();

	// check registry if ed2k links is assigned to emule
	CRegKey regkey;
	regkey.Create(HKEY_CLASSES_ROOT,"ed2k\\shell\\open\\command");
	ULONG maxsize = 500;
	TCHAR rbuffer[500];
	char modbuffer[490];
	char regbuffer[520];
	regkey.QueryStringValue(0,rbuffer,&maxsize);
	::GetModuleFileName(0,modbuffer, 490);
	sprintf(regbuffer,"\"%s\" \"%%1\"",modbuffer);
	if (strcmp(rbuffer,regbuffer)){
		if (checkOnly) return true;
		if (dontAsk || (AfxMessageBox(GetResString(IDS_ASSIGNED2K),MB_ICONQUESTION|MB_YESNO) == IDYES)){
			regkey.SetStringValue(0,regbuffer);	
			regkey.Create(HKEY_CLASSES_ROOT,"ed2k\\DefaultIcon" );// Added Shrink 
			regkey.SetStringValue(0,modbuffer);
			regkey.Create(HKEY_CLASSES_ROOT,"ed2k" );
			regkey.SetStringValue(0,"URL: ed2k Protocol");
			regkey.SetStringValue("URL Protocol","" );
		}
	}
	regkey.Close();
	return false;
}

void BackupReg(void)
{
	// Look for pre-existing old ed2k links
	CRegKey regkey;
	regkey.Create(HKEY_CLASSES_ROOT, "ed2k\\shell\\open\\command");
	ULONG maxsize = 500;
	TCHAR rbuffer[500];
	// Is it ok to write new values
	if ((regkey.QueryStringValue("OldDefault", rbuffer, &maxsize) != ERROR_SUCCESS) || (maxsize == 0))
	{
		maxsize = 500;
		regkey.QueryStringValue(0, rbuffer, &maxsize);
		regkey.SetStringValue("OldDefault", rbuffer);	
		regkey.Create(HKEY_CLASSES_ROOT, "ed2k\\DefaultIcon" );
		maxsize = 500;	
		if (regkey.QueryStringValue(0, rbuffer, &maxsize) == ERROR_SUCCESS)
			regkey.SetStringValue("OldIcon", rbuffer);
	}
	regkey.Close();
}

// Barry - Restore previous values
void RevertReg(void)
{
	// restore previous ed2k links before being assigned to emule
	CRegKey regkey;
	regkey.Create(HKEY_CLASSES_ROOT, "ed2k\\shell\\open\\command");
	ULONG maxsize = 500;
	TCHAR rbuffer[500];
	if (regkey.QueryStringValue("OldDefault", rbuffer, &maxsize) == ERROR_SUCCESS)
	{
		regkey.SetStringValue(0, rbuffer);	
		regkey.Create(HKEY_CLASSES_ROOT, "ed2k\\DefaultIcon" );
		regkey.DeleteValue("OldDefault");
		maxsize = 500;	
		if (regkey.QueryStringValue("OldIcon", rbuffer, &maxsize) == ERROR_SUCCESS)
		{
			regkey.SetStringValue(0, rbuffer);
			regkey.DeleteValue("OldIcon");
		}
	}
	regkey.Close();
}

int GetMaxConnections() {
	OSVERSIONINFOEX osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(!GetVersionEx((OSVERSIONINFO*)&osvi)) {
		//if OSVERSIONINFOEX doesn't work, try OSVERSIONINFO
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if(!GetVersionEx((OSVERSIONINFO*)&osvi))
			return -1;  //shouldn't ever happen
	}

	if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) // Windows NT product family
		return -1;  //no limits

	if(osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) { // Windows 95 product family

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0) { //old school 95
			HKEY hKey;
			DWORD dwValue;
			DWORD dwLength = sizeof(dwValue);
			LONG lResult;

			RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\VxD\\MSTCP",
				0, KEY_QUERY_VALUE, &hKey);
			lResult = RegQueryValueEx(hKey, TEXT("MaxConnections"), NULL, NULL,
				(LPBYTE)&dwValue, &dwLength);
			RegCloseKey(hKey);

			if(lResult != ERROR_SUCCESS || lResult < 1)
				return 100;  //the default for 95 is 100

			return dwValue;

		} else { //98 or ME
			HKEY hKey;
			TCHAR szValue[32];
			DWORD dwLength = sizeof(szValue);
			LONG lResult;

			RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\VxD\\MSTCP",
				0, KEY_QUERY_VALUE, &hKey);
			lResult = RegQueryValueEx(hKey, TEXT("MaxConnections"), NULL, NULL,
				(LPBYTE)szValue, &dwLength);
			RegCloseKey(hKey);

			LONG lMaxConnections;
			if(lResult != ERROR_SUCCESS || (lMaxConnections = atoi(szValue)) < 1)
				return 100;  //the default for 98/ME is 100

			return lMaxConnections;
		}         
	}

	return -1;  //give the user the benefit of the doubt, most use NT+ anyway
}

WORD DetectWinVersion()
{
	OSVERSIONINFOEX osvi;
	BOOL bOsVersionInfoEx;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(!(bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*)&osvi)))
	{
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if(!GetVersionEx((OSVERSIONINFO*)&osvi)) 
		return FALSE;
	}

	switch(osvi.dwPlatformId)
	{
		case VER_PLATFORM_WIN32_NT:
			if(osvi.dwMajorVersion <= 4)
				return _WINVER_NT4_;
			if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
				return _WINVER_2K_;
			if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
				return _WINVER_XP_;
			return _WINVER_XP_; // never return Win95 if we get the info about a NT system
      
		case VER_PLATFORM_WIN32_WINDOWS:
			if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
				return _WINVER_95_; 
			if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
				return _WINVER_98_; 
			if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
				return _WINVER_ME_; 
			break;
		
		default:
			break;
	}
	
	return _WINVER_95_;		// there should'nt be anything lower than this
}

uint64 GetFreeDiskSpaceX(LPCTSTR pDirectory)
{	
	static BOOL _bInitialized = FALSE;
	static BOOL (WINAPI *_pGetDiskFreeSpaceEx)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER) = NULL;

	if (!_bInitialized){
		_bInitialized = TRUE;
		(FARPROC&)_pGetDiskFreeSpaceEx = GetProcAddress(GetModuleHandle("kernel32.dll"), "GetDiskFreeSpaceExA");
	}

	if(_pGetDiskFreeSpaceEx)
	{
		ULARGE_INTEGER nFreeDiskSpace;
		ULARGE_INTEGER dummy;
		_pGetDiskFreeSpaceEx(pDirectory, &nFreeDiskSpace, &dummy, &dummy);
		return nFreeDiskSpace.QuadPart;
	}
	else 
	{
		char cDrive[16];
		char *p = strchr(pDirectory, '\\');
		if(p)
		{
			memcpy(cDrive, pDirectory, p-pDirectory);
			cDrive[p-pDirectory] = '\0';
		}
		else
			strcpy(cDrive, pDirectory);
		DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwDummy;
		GetDiskFreeSpace(cDrive, &dwSectPerClust, &dwBytesPerSect, &dwFreeClusters, &dwDummy);
		return (dwFreeClusters * dwSectPerClust * dwBytesPerSect);
	}
}

//For  Rate File // 
CString   GetRateString(uint16 rate)   { 
      switch (rate){ 
      case 0: 
         return GetResString(IDS_CMT_NOTRATED); 
         break; 
      case 1: 
         return GetResString(IDS_CMT_FAKE); 
         break; 
      case 2: 
         return GetResString(IDS_CMT_POOR); 
         break; 
      case 3: 
         return GetResString(IDS_CMT_GOOD); 
         break; 
      case 4: 
         return GetResString(IDS_CMT_FAIR); 
         break; 
      case 5: 
         return GetResString(IDS_CMT_EXCELLENT); 
         break; 
      } 
      return GetResString(IDS_CMT_NOTRATED); 
} 
//end rate File//

// Returns a BASE32 encoded byte array
//
// [In]
//   buffer: Pointer to byte array
//   bufLen: Lenght of buffer array
//
// [Return]
//   CString object with BASE32 encoded byte array
CString EncodeBase32(const unsigned char* buffer, unsigned int bufLen)
{
	CString Base32Buff;
    
	unsigned int i, index;
    unsigned char word;

    for(i = 0, index = 0; i < bufLen;) {

		// Is the current word going to span a byte boundary?
        if (index > 3) {
            word = (buffer[i] & (0xFF >> index));
            index = (index + 5) % 8;
            word <<= index;
            if (i < bufLen - 1)
                word |= buffer[i + 1] >> (8 - index);

            i++;
        } else {
            word = (buffer[i] >> (8 - (index + 5))) & 0x1F;
            index = (index + 5) % 8;
            if (index == 0)
               i++;
        }

		Base32Buff += (char) base32Chars[word];
    }

    return Base32Buff;
}

// Returns a BASE16 encoded byte array
//
// [In]
//   buffer: Pointer to byte array
//   bufLen: Lenght of buffer array
//
// [Return]
//   CString object with BASE16 encoded byte array
CString EncodeBase16(const unsigned char* buffer, unsigned int bufLen)
{
	CString Base16Buff;

	for(unsigned int i = 0; i < bufLen; i++) {
		Base16Buff += base16Chars[buffer[i] >> 4];
		Base16Buff += base16Chars[buffer[i] & 0xf];
	}

    return Base16Buff;
}

// Decodes a BASE16 string into a byte array
//
// [In]
//   base16Buffer: String containing BASE16
//   base16BufLen: Lenght BASE16 coded string's length
//
// [Out]
//   buffer: byte array containing decoded string
void DecodeBase16(const char *base16Buffer, unsigned int base16BufLen, byte *buffer)
{
    memset(buffer, 0, DecodeLengthBase16(base16BufLen));
  
    for(unsigned int i = 0; i < base16BufLen; i++) {
		int lookup = toupper(base16Buffer[i]) - '0';

        // Check to make sure that the given word falls inside a valid range
		byte word = 0;
        
		if ( lookup < 0 || lookup >= BASE16_LOOKUP_MAX)
           word = 0xFF;
        else
           word = base16Lookup[lookup][1];

		if(i % 2 == 0) {
			buffer[i/2] = word << 4;
		} else {
			buffer[(i-1)/2] |= word;
		}
	}
}

// Calculates length to decode from BASE16
//
// [In]
//   base16Length: Actual length of BASE16 string
//
// [Return]
//   New length of byte array decoded
int	DecodeLengthBase16(int base16Length)
{
	return base16Length / 2;
}


void UpdateURLMenu(CMenu &menu,int &counter){
	counter=0;
	theApp.webservices.RemoveAll();
	CString name,url,sbuffer;
	char buffer[1024];
	int lenBuf = 1024;

	FILE* readFile= fopen(CString(theApp.glob_prefs->GetConfigDir())+"webservices.dat", "r");
	if (readFile!=NULL) {
		while (!feof(readFile)) {
			if (fgets(buffer,lenBuf,readFile)==0) break;
			sbuffer=buffer;
			
			
			// ignore comments & too short lines
			//MORPH START - Added by SiRoB, Webservices PopupMenuSeparator Intelligent Detection
			if (sbuffer.GetAt(0) == '#' || sbuffer.GetAt(0) == '/' || sbuffer.GetLength()<3)
				continue;
				
			int i;
			for(i=1;sbuffer.GetAt(0)==sbuffer.GetAt(i)&&i<4;i++) continue;
			
			if ( i>2 || sbuffer.GetAt(0)=='-')
			{
				counter++;
				menu.AppendMenu(MF_SEPARATOR);
				theApp.webservices.Add("-");
				continue;
			}
			//MORPH START - Added by SiRoB, Webservices PopupMenuSeparator Intelligent Detection

			int pos=sbuffer.Find(',');
			if (pos>0) {
				counter++;
				menu.AppendMenu(MF_STRING,MP_WEBURL+(counter-1), sbuffer.Left(pos).Trim() );
				theApp.webservices.Add(sbuffer.Right(sbuffer.GetLength()-pos-1).Trim() );
			}
		}
		fclose(readFile);
	}
}

void RunURL(CAbstractFile* file, CString urlpattern)
{
	if (file!=NULL) {
		// Convert hash to hexadecimal text and add it to the URL
		urlpattern.Replace("#hashid", EncodeBase16(file->GetFileHash(), 16));

		// Add file size to the URL
		CString temp;
		temp.Format("%u",file->GetFileSize());
		urlpattern.Replace("#filesize", temp);

		// add filename to the url
		urlpattern.Replace("#filename",URLEncode(file->GetFileName()));
	
	}
	// Open URL
	ShellExecute(NULL, NULL, urlpattern, NULL, theApp.glob_prefs->GetAppDir(), SW_SHOWDEFAULT);
}

typedef struct
{
	LPCTSTR	pszInitialDir;
	LPCTSTR	pszDlgTitle;
} BROWSEINIT, *LPBROWSEINIT;

extern "C" int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM /*lParam*/, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED)
	{
		// Set initial directory
		if ( ((LPBROWSEINIT)lpData)->pszInitialDir != NULL )
			SendMessage(hWnd, BFFM_SETSELECTION, TRUE, (LPARAM)((LPBROWSEINIT)lpData)->pszInitialDir);

		// Set dialog's window title
		if ( ((LPBROWSEINIT)lpData)->pszDlgTitle != NULL )
			SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)((LPBROWSEINIT)lpData)->pszDlgTitle);
	}

	return 0;
}

bool SelectDir(HWND hWnd, LPTSTR pszPath, LPCTSTR pszTitle, LPCTSTR pszDlgTitle)
{
	BOOL bResult = FALSE;
	CoInitialize(0);
	LPMALLOC pShlMalloc;
	if (SHGetMalloc(&pShlMalloc) == NOERROR)
	{
		BROWSEINFO BrsInfo = {0};
		BrsInfo.hwndOwner = hWnd;
		BrsInfo.lpszTitle = (pszTitle != NULL) ? pszTitle : pszDlgTitle;
		BrsInfo.ulFlags = BIF_VALIDATE | BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS | BIF_SHAREABLE | BIF_DONTGOBELOWDOMAIN;

		BROWSEINIT BrsInit = {0};
		if (pszPath != NULL || pszTitle != NULL || pszDlgTitle != NULL){
			// Need the 'BrowseCallbackProc' to set those strings
			BrsInfo.lpfn = BrowseCallbackProc;
			BrsInfo.lParam = (LPARAM)&BrsInit;
			BrsInit.pszDlgTitle = (pszDlgTitle != NULL) ? pszDlgTitle : NULL/*pszTitle*/;
			BrsInit.pszInitialDir = pszPath;
		}

		LPITEMIDLIST pidlBrowse;
		if ((pidlBrowse = SHBrowseForFolder(&BrsInfo)) != NULL){
			if (SHGetPathFromIDList(pidlBrowse, pszPath))
				bResult = TRUE;
			pShlMalloc->Free(pidlBrowse);
		}
		pShlMalloc->Release();
	}
	CoUninitialize();
	return bResult;
}

void MakeFoldername(char* path){
//	CString string(path);
//	if (string.GetLength()>0) if (string.Right(1)=='\\') string=string.Left(string.GetLength()-1);
//	sprintf(path,"%s",string);
	CString strPath(path);
	PathCanonicalize(path, strPath);
	PathRemoveBackslash(path);
}

CString StringLimit(CString in,uint16 length){
	if (in.GetLength()<=length || length<10) return in;

	return (in.Left(length-8)+"..."+in.Right(8));
}

BOOL DialogBrowseFile(CString& rstrPath, LPCTSTR pszFilters, LPCTSTR pszDefaultFileName, DWORD dwFlags) {
    CFileDialog myFileDialog(true,NULL,pszDefaultFileName,
							 dwFlags | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, pszFilters, NULL, 
							 0/*automatically use explorer style open dialog on systems which support it*/);
    if (myFileDialog.DoModal() != IDOK)
		return FALSE;
	rstrPath = myFileDialog.GetPathName();
	return TRUE;
}

// khaos::categorymod+ Obsolete
/*CString GetCatTitle(int catid) {
	switch (catid) {
		case 0 : return GetResString(IDS_ALL);
		case 1 : return GetResString(IDS_ALLOTHERS);
		case 2 : return GetResString(IDS_STATUS_NOTCOMPLETED);
		case 3 : return GetResString(IDS_DL_TRANSFCOMPL);
		case 4 : return GetResString(IDS_WAITING);
		case 5 : return GetResString(IDS_DOWNLOADING);
		case 6 : return GetResString(IDS_ERRORLIKE);
		case 7 : return GetResString(IDS_PAUSED);
		case 8 : return GetResString(IDS_STOPPED);
		case 10 : return GetResString(IDS_VIDEO);
		case 11 : return GetResString(IDS_AUDIO);
		case 12 : return GetResString(IDS_SEARCH_ARC);
		case 13 : return GetResString(IDS_SEARCH_CDIMG);
	}
	return "?";
}*/
 
/*Checks, if a given item should be shown in a given category
AllcatTypes:
	0	all
	1	all not assigned
	2	not completed
	3	completed
	4	waiting
	5	transfering
	6	errorous
	7	paused
	8	stopped
	10	Video
	11	Audio
	12	Archive
	13	CDImage
*/
// Rewritten.
bool CheckShowItemInGivenCat(CPartFile* file, int inCategory) {

	Category_Struct* curCat = theApp.glob_prefs->GetCategory(inCategory);

	if (curCat->viewfilters.bSuspendFilters && (file->GetCategory() == inCategory || curCat->viewfilters.nFromCats == 0))
		return true;

	if (curCat->viewfilters.nFromCats == 2 && file->GetCategory() != inCategory)
		return false;

	if (!curCat->viewfilters.bVideo && file->IsMovie())
		return false;
	if (!curCat->viewfilters.bAudio && ED2KFT_AUDIO == GetED2KFileTypeID(file->GetFileName()))
		return false;
	if (!curCat->viewfilters.bArchives && file->IsArchive())
		return false;
	if (!curCat->viewfilters.bImages && ED2KFT_CDIMAGE == GetED2KFileTypeID(file->GetFileName()))
		return false;
	if (!curCat->viewfilters.bWaiting && !file->IsPaused() && !file->IsStopped() && ((file->GetStatus()==PS_READY|| file->GetStatus()==PS_EMPTY) && file->GetTransferingSrcCount()==0))
		return false;
	if (!curCat->viewfilters.bTransferring && ((file->GetStatus()==PS_READY|| file->GetStatus()==PS_EMPTY) && file->GetTransferingSrcCount()>0))
		return false;
	if (!curCat->viewfilters.bPaused && file->IsPaused())
		return false;
	if (!curCat->viewfilters.bStopped && file->IsStopped())
		return false;
	if (!curCat->viewfilters.bComplete && !file->IsPartFile())
		return false;
	if (!curCat->viewfilters.bCompleting && file->GetStatus() == PS_COMPLETING)
		return false;
	if (!curCat->viewfilters.bHashing && file->GetStatus() == PS_HASHING)
		return false;
	if (!curCat->viewfilters.bErrorUnknown && (file->GetStatus() == PS_ERROR || file->GetStatus() == PS_UNKNOWN))
		return false;
	if (file->GetFileSize() < curCat->viewfilters.nFSizeMin || (curCat->viewfilters.nFSizeMax != 0 && file->GetFileSize() > curCat->viewfilters.nFSizeMax))
		return false;
	uint32 nTemp = file->GetFileSize() - file->GetCompletedSize();
	if (nTemp < curCat->viewfilters.nRSizeMin || (curCat->viewfilters.nRSizeMax != 0 && nTemp > curCat->viewfilters.nRSizeMax))
		return false;
	if (curCat->viewfilters.nTimeRemainingMin > 0 || curCat->viewfilters.nTimeRemainingMax > 0)
	{
		sint32 nTemp2 = file->getTimeRemaining();
		if (nTemp2 < (sint32)curCat->viewfilters.nTimeRemainingMin || (curCat->viewfilters.nTimeRemainingMax != 0 && nTemp2 > (sint32)curCat->viewfilters.nTimeRemainingMax))
	return false;
}
	nTemp = file->GetSourceCount();
	if (nTemp < curCat->viewfilters.nSourceCountMin || (curCat->viewfilters.nSourceCountMax != 0 && nTemp > curCat->viewfilters.nSourceCountMax))
		return false;
	nTemp = file->GetAvailableSrcCount();
	if (nTemp < curCat->viewfilters.nAvailSourceCountMin || (curCat->viewfilters.nAvailSourceCountMax != 0 && nTemp > curCat->viewfilters.nAvailSourceCountMax))
		return false;
	if (!curCat->viewfilters.sAdvancedFilterMask.IsEmpty() && !theApp.downloadqueue->ApplyFilterMask(file->GetFileName(), inCategory))
		return false;

	return true;
}
// khaos::categorymod-

void md4str(const uchar* hash, char* pszHash)
{
    static const char _acHexDigits[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; i++){
		*pszHash++ = _acHexDigits[hash[i] >> 4];
		*pszHash++ = _acHexDigits[hash[i] & 0xf];
	}
	*pszHash = '\0';
}

CString md4str(const uchar* hash)
{
	char szHash[MAX_HASHSTR_SIZE];
	md4str(hash, szHash);
	return szHash;
}

uint8 GetRealPrio(uint8 in) {
	switch(in) {
		case 4 : return 0;
		case 0 : return 1;
		case 1 : return 2;
		case 2 : return 3;
		case 3 : return 4;
	}
	return 0;
}

CString CleanupFilename(CString filename) {
	
	CString tempStr;
	filename=URLDecode(filename);
	filename.MakeLower();

	//remove substrings, defined in the preferences (.ini)
	CString resToken;
	CString strlink=theApp.glob_prefs->GetFilenameCleanups().MakeLower();
	int curPos=0;
	resToken= strlink.Tokenize("|",curPos);
	while (resToken != "") {
		filename.Replace(resToken,"");
		resToken= strlink.Tokenize("|",curPos);
	}

	// Replace . with Spaces - except the last one (extention-dot)
	int extpos=filename.ReverseFind('.');
	if (extpos>=0) {
		for (int i=0;i<extpos;++i) {
			if (filename.GetAt(i)!='.') continue;
			if (i>0 && i<filename.GetLength()-1 && isdigit(filename.GetAt(i-1)) && isdigit(filename.GetAt(i+1)) ) continue;
			filename.SetAt(i,' ');
		}
	}

	// Replace Space-holders with Spaces
	filename.Replace('_',' ');
	filename.Replace("+"," "); //SyruS for Jigle

	//SyruS additional cleanup
	// invalid for filenames
	filename.Replace("\\", ""); //[edit: oops, one \ was missing]
	filename.Replace("\"", "");
	filename.Replace("/", "");
	filename.Replace(":", "");
	filename.Replace("*", "");
	filename.Replace("?", "");
	filename.Replace("<", "");
	filename.Replace(">", "");
	filename.Replace("|", "");
	// other common nonsense (u can use dots here!)
	filename.Replace("=", "");

	int pos1,pos2;
	pos1=-1;
	while (true) {
		pos1=filename.Find('[',pos1+1);
		if (pos1==-1) break;
		pos2=filename.Find(']',pos1);
		if (pos1>-1 && pos2>pos1) {
			if (pos2-pos1 > 1) {
				tempStr=filename.Mid(pos1+1,pos2-pos1-1);
				int numcount=0;
				for (int i=0;i<tempStr.GetLength();++i)
					if (isdigit(tempStr.GetAt(i))) ++numcount;
				if (numcount>tempStr.GetLength()/2) continue;
			}
			filename=filename.Left(pos1)+filename.Right(filename.GetLength()-pos2-1);
			pos1-=(pos2-pos1);
		} else break;
	}

	// Barry - Some additional formatting
	filename.Replace("()", "");
	filename.Replace("  ", " ");
	filename.Replace(" .",".");

	filename.Replace("( ", "(");
	filename.Replace(" )", ")");
	filename.Replace("()", "");
	filename.Replace("{ ", "{");
	filename.Replace(" }", "}");
	filename.Replace("{}", "");

	// Make leading Caps 
	if (filename.GetLength()>1)
	{
		tempStr=filename.GetAt(0);
		tempStr.MakeUpper();
		filename.SetAt(0, tempStr.GetAt(0));
		int topos=filename.ReverseFind('.')-1;
		if (topos<0) topos=filename.GetLength()-1;

		for (int ix=0; ix<topos; ix++)
		{
			if (!IsCharAlpha(filename.GetAt(ix))) //if (filename.GetAt(ix) == ' ' || isdigit(filename.GetAt(ix)) ) 
			{
				if (ix<filename.GetLength()-2 && isdigit(filename.GetAt(ix+2) ))
					continue;

				tempStr=filename.GetAt(ix+1);
				tempStr.MakeUpper();
				filename.SetAt(ix+1,tempStr.GetAt(0));
			}
		}
	}
	filename.Trim();
	return filename;
}

struct SED2KFileType
{
	LPCTSTR pszExt;
	EED2KFileType iFileType;
} _aED2KFileTypes[] = 
{
    { _T(".669"),   ED2KFT_AUDIO },
	{ _T(".aac"),	ED2KFT_AUDIO },
	{ _T(".aif"),	ED2KFT_AUDIO },
	{ _T(".aiff"),	ED2KFT_AUDIO },
    { _T(".amf"),   ED2KFT_AUDIO },
    { _T(".ams"),   ED2KFT_AUDIO },
	{ _T(".ape"),	ED2KFT_AUDIO },
	{ _T(".au"),	ED2KFT_AUDIO },
    { _T(".dbm"),   ED2KFT_AUDIO },
    { _T(".dmf"),   ED2KFT_AUDIO },
    { _T(".dsm"),   ED2KFT_AUDIO },
    { _T(".far"),   ED2KFT_AUDIO },
	{ _T(".flac"),	ED2KFT_AUDIO },
    { _T(".it"),    ED2KFT_AUDIO },
    { _T(".mdl"),   ED2KFT_AUDIO },
    { _T(".med"),   ED2KFT_AUDIO },
	{ _T(".mid"),	ED2KFT_AUDIO },
	{ _T(".midi"),	ED2KFT_AUDIO },
    { _T(".mod"),   ED2KFT_AUDIO },
    { _T(".mol"),   ED2KFT_AUDIO },
	{ _T(".mp1"),	ED2KFT_AUDIO },
	{ _T(".mp2"),	ED2KFT_AUDIO },
	{ _T(".mp3"),	ED2KFT_AUDIO },
	{ _T(".mp4"),	ED2KFT_AUDIO },
	{ _T(".mpa"),	ED2KFT_AUDIO },
	{ _T(".mpc"),	ED2KFT_AUDIO },
	{ _T(".mpp"),	ED2KFT_AUDIO },
    { _T(".mtm"),   ED2KFT_AUDIO },
    { _T(".nst"),   ED2KFT_AUDIO },
	{ _T(".ogg"),	ED2KFT_AUDIO },
    { _T(".okt"),   ED2KFT_AUDIO },
    { _T(".psm"),   ED2KFT_AUDIO },
    { _T(".ptm"),   ED2KFT_AUDIO },
	{ _T(".ra"),	ED2KFT_AUDIO },
	{ _T(".rmi"),	ED2KFT_AUDIO },
    { _T(".s3m"),   ED2KFT_AUDIO },
    { _T(".stm"),   ED2KFT_AUDIO },
    { _T(".ult"),   ED2KFT_AUDIO },
    { _T(".umx"),   ED2KFT_AUDIO },
	{ _T(".wav"),	ED2KFT_AUDIO },
	{ _T(".wma"),	ED2KFT_AUDIO },
    { _T(".wow"),   ED2KFT_AUDIO },
    { _T(".xm"),    ED2KFT_AUDIO },

	{ _T(".asf"),	ED2KFT_VIDEO },
	{ _T(".avi"),	ED2KFT_VIDEO },
	{ _T(".divx"),	ED2KFT_VIDEO },
	{ _T(".m1v"),	ED2KFT_VIDEO },
	{ _T(".m2v"),	ED2KFT_VIDEO },
	{ _T(".mkv"),	ED2KFT_VIDEO },
	{ _T(".mov"),	ED2KFT_VIDEO },
	{ _T(".mp1v"),	ED2KFT_VIDEO },
	{ _T(".mp2v"),	ED2KFT_VIDEO },
	{ _T(".mpe"),	ED2KFT_VIDEO },
	{ _T(".mpeg"),	ED2KFT_VIDEO },
	{ _T(".mpg"),	ED2KFT_VIDEO },
	{ _T(".mps"),	ED2KFT_VIDEO },
	{ _T(".mpv"),	ED2KFT_VIDEO },
	{ _T(".mpv1"),	ED2KFT_VIDEO },
	{ _T(".mpv2"),	ED2KFT_VIDEO },
	{ _T(".ogm"),	ED2KFT_VIDEO },
	{ _T(".qt"),	ED2KFT_VIDEO },
	{ _T(".ram"),	ED2KFT_VIDEO },
	{ _T(".rm"),	ED2KFT_VIDEO },
	{ _T(".rv"),	ED2KFT_VIDEO },
	{ _T(".vivo"),	ED2KFT_VIDEO },
	{ _T(".vob"),	ED2KFT_VIDEO },
	{ _T(".wmv"),	ED2KFT_VIDEO },
	{ _T(".rv9"),	ED2KFT_VIDEO },

	{ _T(".bmp"),	ED2KFT_IMAGE },
	{ _T(".dcx"),	ED2KFT_IMAGE },
	{ _T(".emf"),	ED2KFT_IMAGE },
	{ _T(".gif"),	ED2KFT_IMAGE },
	{ _T(".ico"),	ED2KFT_IMAGE },
	{ _T(".jpeg"),	ED2KFT_IMAGE },
	{ _T(".jpg"),	ED2KFT_IMAGE },
	{ _T(".pct"),	ED2KFT_IMAGE },
	{ _T(".pcx"),	ED2KFT_IMAGE },
	{ _T(".pic"),	ED2KFT_IMAGE },
	{ _T(".pict"),	ED2KFT_IMAGE },
	{ _T(".png"),	ED2KFT_IMAGE },
	{ _T(".psd"),	ED2KFT_IMAGE },
	{ _T(".psp"),	ED2KFT_IMAGE },
	{ _T(".tga"),	ED2KFT_IMAGE },
	{ _T(".tif"),	ED2KFT_IMAGE },
	{ _T(".tiff"),	ED2KFT_IMAGE },
	{ _T(".wmf"),	ED2KFT_IMAGE },
	{ _T(".xif"),	ED2KFT_IMAGE },

	{ _T(".ace"),	ED2KFT_ARCHIVE },
	{ _T(".arj"),	ED2KFT_ARCHIVE },
	{ _T(".gz"),	ED2KFT_ARCHIVE },
	{ _T(".hqx"),	ED2KFT_ARCHIVE },
	{ _T(".lha"),	ED2KFT_ARCHIVE },
	{ _T(".rar"),	ED2KFT_ARCHIVE },
	{ _T(".sea"),	ED2KFT_ARCHIVE },
	{ _T(".sit"),	ED2KFT_ARCHIVE },
	{ _T(".tar"),	ED2KFT_ARCHIVE },
	{ _T(".tgz"),	ED2KFT_ARCHIVE },
	{ _T(".uc2"),	ED2KFT_ARCHIVE },
	{ _T(".zip"),	ED2KFT_ARCHIVE },

	{ _T(".bat"),	ED2KFT_PROGRAM },
	{ _T(".cmd"),	ED2KFT_PROGRAM },
	{ _T(".com"),	ED2KFT_PROGRAM },
	{ _T(".exe"),	ED2KFT_PROGRAM },

	{ _T(".bin"),	ED2KFT_CDIMAGE },
	{ _T(".bwa"),	ED2KFT_CDIMAGE },
	{ _T(".bwi"),	ED2KFT_CDIMAGE },
	{ _T(".bws"),	ED2KFT_CDIMAGE },
	{ _T(".bwt"),	ED2KFT_CDIMAGE },
	{ _T(".ccd"),	ED2KFT_CDIMAGE },
	{ _T(".cue"),	ED2KFT_CDIMAGE },
	{ _T(".dmg"),	ED2KFT_CDIMAGE },
	{ _T(".dmz"),	ED2KFT_CDIMAGE },
	{ _T(".img"),	ED2KFT_CDIMAGE },
	{ _T(".iso"),	ED2KFT_CDIMAGE },
	{ _T(".mdf"),	ED2KFT_CDIMAGE },
	{ _T(".mds"),	ED2KFT_CDIMAGE },
	{ _T(".nrg"),	ED2KFT_CDIMAGE },
	{ _T(".sub"),	ED2KFT_CDIMAGE },
	{ _T(".toast"), ED2KFT_CDIMAGE }

	// To be uncommented after we use the 'Doc' ed2k filetype for search expressions
//	{ _T(".txt"),   ED2KFT_DOCUMENT },
//	{ _T(".nfo"),   ED2KFT_DOCUMENT },
//	{ _T(".diz"),   ED2KFT_DOCUMENT },
//	{ _T(".doc"),   ED2KFT_DOCUMENT },
//	{ _T(".rtf"),   ED2KFT_DOCUMENT },
//	{ _T(".pdf"),   ED2KFT_DOCUMENT },	// double check this!
//	{ _T(".xls"),   ED2KFT_DOCUMENT },	// double check this!
//	{ _T(".html"),  ED2KFT_DOCUMENT },
//	{ _T(".htm"),   ED2KFT_DOCUMENT }
};

int __cdecl CompareE2DKFileType(const void* p1, const void* p2)
{
	return _tcscmp( ((const SED2KFileType*)p1)->pszExt, ((const SED2KFileType*)p2)->pszExt );
}

EED2KFileType GetED2KFileTypeID(LPCTSTR pszFileName)
{
	LPCTSTR pszExt = _tcsrchr(pszFileName, _T('.'));
	if (pszExt == NULL)
		return ED2KFT_ANY;
	CString strExt(pszExt);
	strExt.MakeLower();

	SED2KFileType ft;
	ft.pszExt = strExt;
	ft.iFileType = ED2KFT_ANY;
	const SED2KFileType* pFound = (SED2KFileType*)bsearch(&ft, _aED2KFileTypes, ELEMENT_COUNT(_aED2KFileTypes), sizeof _aED2KFileTypes[0], CompareE2DKFileType);
	if (pFound != NULL)
		return pFound->iFileType;
	return ED2KFT_ANY;
}

// Retuns the ed2k file type term which is to be used in server searches
LPCSTR GetED2KFileTypeSearchTerm(EED2KFileType iFileID)
{
	if (iFileID == ED2KFT_AUDIO)		return "Audio";
	if (iFileID == ED2KFT_VIDEO)		return "Video";
	if (iFileID == ED2KFT_IMAGE)		return "Image";
	if (iFileID == ED2KFT_ARCHIVE || iFileID == ED2KFT_PROGRAM || iFileID == ED2KFT_CDIMAGE) return "Pro";
	if (iFileID == ED2KFT_DOCUMENT)		return "Doc";
	return NULL;
}

// Returns a localized typename, examining the extention of the given filename
CString GetFiletypeByName(LPCTSTR pszFileName)
{
	EED2KFileType iFileType = GetED2KFileTypeID(pszFileName);
	switch (iFileType) {
		case ED2KFT_AUDIO :		return GetResString(IDS_SEARCH_AUDIO);
		case ED2KFT_VIDEO :		return GetResString(IDS_SEARCH_VIDEO);
		case ED2KFT_IMAGE :		return GetResString(IDS_SEARCH_PICS);
		case ED2KFT_ARCHIVE :	return GetResString(IDS_SEARCH_ARC);
		case ED2KFT_PROGRAM :	return GetResString(IDS_SEARCH_PRG);
		case ED2KFT_CDIMAGE :	return GetResString(IDS_SEARCH_CDIMG);
		default:				return GetResString(IDS_SEARCH_ANY);
	}
}


class CED2KFileTypes{
public:
	CED2KFileTypes(){
		qsort(_aED2KFileTypes, ELEMENT_COUNT(_aED2KFileTypes), sizeof _aED2KFileTypes[0], CompareE2DKFileType);
#ifdef _DEBUG
		// check for duplicate entries
		LPCTSTR pszLast = _aED2KFileTypes[0].pszExt;
		for (int i = 1; i < ELEMENT_COUNT(_aED2KFileTypes); i++){
			ASSERT( _tcscmp(pszLast, _aED2KFileTypes[i].pszExt) != 0 );
			pszLast = _aED2KFileTypes[i].pszExt;
		}
#endif
	}
};
CED2KFileTypes theED2KFileTypes; // get the list sorted *before* any code is accessing it

CHAR *stristr(const CHAR *str1, const CHAR *str2)
{
	const CHAR *cp = str1;
	const CHAR *s1;
	const CHAR *s2;

	if (!*str2)
		return (CHAR *)str1;

	while (*cp)
	{
		s1 = cp;
		s2 = str2;

		while (*s1 && *s2 && tolower(*s1) == tolower(*s2))
			s1++, s2++;

		if (!*s2)
			return (CHAR *)cp;

		cp++;
	}

	return NULL;
}

int GetSystemErrorString(DWORD dwError, CString &rstrError)
{
	// FormatMessage language flags:
	//
	// - MFC uses: MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT)
	//				SUBLANG_SYS_DEFAULT = 0x02 (system default)
	//
	// - SDK uses: MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
	//				SUBLANG_DEFAULT		= 0x01 (user default)
	//
	//
	// Found in "winnt.h"
	// ------------------
	//  Language IDs.
	//
	//  The following two combinations of primary language ID and
	//  sublanguage ID have special semantics:
	//
	//    Primary Language ID   Sublanguage ID      Result
	//    -------------------   ---------------     ------------------------
	//    LANG_NEUTRAL          SUBLANG_NEUTRAL     Language neutral
	//    LANG_NEUTRAL          SUBLANG_DEFAULT     User default language
	//    LANG_NEUTRAL          SUBLANG_SYS_DEFAULT System default language
	//
	// *** SDK notes also:
	// If you pass in zero, 'FormatMessage' looks for a message for LANGIDs in 
	// the following order:
	//
	//	1) Language neutral 
	//	2) Thread LANGID, based on the thread's locale value 
	//  3) User default LANGID, based on the user's default locale value 
	//	4) System default LANGID, based on the system default locale value 
	//	5) US English 
	LPTSTR pszSysMsg = NULL;
	DWORD dwLength = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
								   NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
								   (LPTSTR)&pszSysMsg, 0, NULL);
	if (pszSysMsg != NULL && dwLength != 0)
	{
		if (dwLength >= 2 && pszSysMsg[dwLength - 2] == _T('\r'))
			pszSysMsg[dwLength - 2] = _T('\0');
		rstrError = pszSysMsg;
		rstrError.Replace(_T("\r\n"), _T(" ")); // some messages contain CRLF within the message!?
	}
	else {
		rstrError.Empty();
	}

	if (pszSysMsg)
		LocalFree(pszSysMsg);

	return rstrError.GetLength();
}

int GetModuleErrorString(DWORD dwError, CString &rstrError, LPCTSTR pszModule)
{
	LPTSTR pszSysMsg = NULL;
	DWORD dwLength = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
								   GetModuleHandle(pszModule), dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
								   (LPTSTR)&pszSysMsg, 0, NULL);
	if (pszSysMsg != NULL && dwLength != 0)
	{
		if (dwLength >= 2 && pszSysMsg[dwLength - 2] == _T('\r'))
			pszSysMsg[dwLength - 2] = _T('\0');
		rstrError = pszSysMsg;
		rstrError.Replace(_T("\r\n"), _T(" ")); // some messages contain CRLF within the message!?
	}
	else {
		rstrError.Empty();
	}

	if (pszSysMsg)
		LocalFree(pszSysMsg);

	return rstrError.GetLength();
}

int GetErrorMessage(DWORD dwError, CString &rstrErrorMsg, DWORD dwFlags)
{
	int iMsgLen = GetSystemErrorString(dwError, rstrErrorMsg);
	if (iMsgLen == 0)
	{
		if ((long)dwError >= 0)
			rstrErrorMsg.Format(_T("Error %u"), dwError);
		else
			rstrErrorMsg.Format(_T("Error 0x%08x"), dwError);
	}
	else if (dwFlags & 1)
	{
		CString strFullErrorMsg;
		if ((long)dwError >= 0)
			strFullErrorMsg.Format(_T("Error %u: %s"), dwError, rstrErrorMsg);
		else
			strFullErrorMsg.Format(_T("Error 0x%08x: %s"), dwError, rstrErrorMsg);
		rstrErrorMsg = strFullErrorMsg;
	}

	return rstrErrorMsg.GetLength();
}

CString GetErrorMessage(DWORD dwError, DWORD dwFlags)
{
	CString strError;
	GetErrorMessage(dwError, strError, dwFlags);
	return strError;
}

int GetAppImageListColorFlag()
{
	HDC hdcScreen = ::GetDC(NULL);
	int iColorBits = GetDeviceCaps(hdcScreen, BITSPIXEL) * GetDeviceCaps(hdcScreen, PLANES);
	::ReleaseDC(NULL, hdcScreen);
	int iIlcFlag;
	if (iColorBits >= 32)
		iIlcFlag = ILC_COLOR32;
	else if (iColorBits >= 24)
		iIlcFlag = ILC_COLOR24;
	else if (iColorBits >= 16)
		iIlcFlag = ILC_COLOR16;
	else if (iColorBits >= 8)
		iIlcFlag = ILC_COLOR8;
	else if (iColorBits >= 4)
		iIlcFlag = ILC_COLOR4;
	else
		iIlcFlag = ILC_COLOR;
	return iIlcFlag;
}

CString GetHexDump(const uint8* data, UINT size)
{
	CString buffer; 
	buffer.Format(_T("size=%u"), size);
	buffer += _T(", data=[");
	UINT i = 0;
	for(; i < size && i < 50; i++){
		if (i > 0)
			buffer += _T(" ");
		TCHAR temp[3];
		_stprintf(temp, _T("%02x"), data[i]);
		buffer += temp;
	}
	buffer += (i == size) ? _T("]") : _T("..]");
	return buffer;
}

void DbgSetThreadName(LPCSTR szThreadName, ...) 
{
#ifdef DEBUG

#ifndef MS_VC_EXCEPTION
#define MS_VC_EXCEPTION 0x406d1388 

typedef struct tagTHREADNAME_INFO 
{
	DWORD dwType;		// must be 0x1000 
	LPCSTR szName;		// pointer to name (in same addr space) 
	DWORD dwThreadID;	// thread ID (-1 caller thread) 
	DWORD dwFlags;		// reserved for future use, must be zero 
} THREADNAME_INFO; 
#endif

	__try
	{
		va_list args;
		va_start(args, szThreadName);
		int lenBuf = 0;
		char *buffer = NULL;
		int lenResult;
		do // the VS debugger truncates the string to 31 characters anyway!
		{
			lenBuf += 128;
			if (buffer != NULL)
				delete [] buffer;
			buffer = new char[lenBuf];
			lenResult = _vsnprintf(buffer, lenBuf, szThreadName, args);
		} while (lenResult == -1);
		va_end(args);
		THREADNAME_INFO info; 
		info.dwType = 0x1000; 
		info.szName = buffer; 
		info.dwThreadID = -1; 
		info.dwFlags = 0; 
		__try 
		{ 
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (DWORD *)&info); 
		} __except (EXCEPTION_CONTINUE_EXECUTION) { } 
		delete [] buffer;
	}
	__except (EXCEPTION_CONTINUE_EXECUTION) {}
#endif
}

CString RemoveFileExtension(const CString& rstrFilePath)
{
	int iDot = rstrFilePath.ReverseFind(_T('.'));
	if (iDot == -1)
		return rstrFilePath;
	return rstrFilePath.Mid(0, iDot);
}

int CompareDirectories(const CString& rstrDir1, const CString& rstrDir2)
{
	// use case insensitive compare as a starter
	if (rstrDir1.CompareNoCase(rstrDir2)==0)
		return 0;

	// if one of the paths ends with a '\' the paths may still be equal from the file system's POV
	CString strDir1(rstrDir1);
	CString strDir2(rstrDir2);
	PathRemoveBackslash(strDir1.GetBuffer());	// remove any available backslash
	strDir1.ReleaseBuffer();
	PathRemoveBackslash(strDir2.GetBuffer());	// remove any available backslash
	strDir2.ReleaseBuffer();
	return strDir1.CompareNoCase(strDir2);		// compare again
}

bool IsGoodIP(uint32 nIP, bool forceCheck) //MORPH - Modified by SiRoB, ZZ Upload system (USS)
{
	// always filter following IP's
	// -------------------------------------------
	//	 0.0.0.0
	// 127.*.*.*						localhost

	if (nIP==0 || (uint8)nIP==127)
		return false;

	if (!theApp.glob_prefs->FilterBadIPs() && !forceCheck) //MORPH - Modified by SiRoB, ZZ Upload system (USS)
		return true;

	// filter LAN IP's
	// -------------------------------------------
	//	0.*
	//	10.0.0.0 - 10.255.255.255		class A
	//	172.16.0.0 - 172.31.255.255		class B
	//	192.168.0.0 - 192.168.255.255	class C

	uint8 nFirst = (uint8)nIP;
	uint8 nSecond = (uint8)(nIP >> 8);

	if (nFirst==192 && nSecond==168) // check this 1st, because those LANs IPs are mostly spreaded
		return false;

	if (nFirst==172 && nSecond>=16 && nSecond<=31)
		return false;

	if (nFirst==0 || nFirst==10)
		return false;

	return true; 
}

bool IsGoodIPPort(uint32 nIP, uint16 nPort)
{
	return IsGoodIP(nIP) && nPort!=0;
}

CString GetFormatedUInt(ULONG ulVal)
{
	TCHAR szVal[12];
	_ultot(ulVal, szVal, 10);

	static NUMBERFMT nf;
	if (nf.Grouping == 0) {
		nf.NumDigits = 0;
		nf.LeadingZero = 0;
		nf.Grouping = 3;
		nf.lpDecimalSep = _T(",");
		nf.lpThousandSep = _T(".");
		nf.NegativeOrder = 0;
	}
	CString strVal;
	const int iBuffSize = ARRSIZE(szVal)*2;
	int iResult = GetNumberFormat(LOCALE_SYSTEM_DEFAULT, 0, szVal, &nf, strVal.GetBuffer(iBuffSize), iBuffSize);
	strVal.ReleaseBuffer();
	if (iResult == 0)
		strVal = szVal;
	return strVal;
}
void Debug(LPCTSTR pszFmtMsg, ...)
{
	va_list pArgs;
	va_start(pArgs, pszFmtMsg);
	CString strBuff;
#ifdef _DEBUG
	time_t tNow = time(NULL);
	int iTimeLen = _tcsftime(strBuff.GetBuffer(40), 40, _T("%H:%M:%S "), localtime(&tNow));
	strBuff.ReleaseBuffer(iTimeLen);
#endif
	strBuff.AppendFormatV(pszFmtMsg, pArgs);

	// get around a bug in the debug device which is not capable of dumping long strings
	int i = 0;
	while (i < strBuff.GetLength()){
		OutputDebugString(strBuff.Mid(i, 1024));
		i += 1024;
	}
	va_end(pArgs);
}

void DebugHexDump(const uint8* data, UINT lenData)
{
	uint16 lenLine = 16;
	uint32 pos = 0;
	byte c = 0;
	while (pos < lenData)
	{
		CString line;
		CString single;
		line.Format(_T("%08X "), pos);
		lenLine = min((lenData - pos), 16);
		for (int i=0; i<lenLine; i++)
		{
			single.Format(_T(" %02X"), data[pos+i]);
			line += single;
			if (i == 7)
				line += _T(' ');
		}
		line += CString(_T(' '), 60 - line.GetLength());
		for (int i=0; i<lenLine; i++)
		{
			c = data[pos + i];
			single.Format(_T("%c"), (((c > 31) && (c < 127)) ? (_TUCHAR)c : _T('.')));
			line += single;
		}
		Debug(_T("%s\n"), (LPCTSTR)line);
		pos += lenLine;
	}
}

void DebugHexDump(CFile& file)
{
	int iSize = (int)(file.GetLength() - file.GetPosition());
	if (iSize > 0)
	{
		uint8* data = NULL;
		try{
			data = new uint8[iSize];
			file.Read(data, iSize);
			DebugHexDump(data, iSize);
		}
		catch(CFileException* e){
			TRACE("*** DebugHexDump(CFile&); CFileException\n");
			e->Delete();
		}
		catch(CMemoryException* e){
			TRACE("*** DebugHexDump(CFile&); CMemoryException\n");
			e->Delete();
		}
		delete[] data;
	}
}

bool strmd4(const char* pszHash, uchar* hash)
{
	memset(hash, 0, 16);
	for (int i = 0; i < 16; i++)
	{
		char byte[3];
		byte[0] = pszHash[i*2+0];
		byte[1] = pszHash[i*2+1];
		byte[2] = '\0';

		UINT b;
		if (sscanf(byte, "%x", &b) != 1)
			return false;
		hash[i] = b;
	}
	return true;
}

bool strmd4(const CString& rstr, uchar* hash)
{
	memset(hash, 0, 16);
	if (rstr.GetLength() != 16*2)
		return false;
	for (int i = 0; i < 16; i++)
	{
		char byte[3];
		byte[0] = rstr[i*2+0];
		byte[1] = rstr[i*2+1];
		byte[2] = '\0';

		UINT b;
		if (sscanf(byte, "%x", &b) != 1)
			return false;
		hash[i] = b;
	}
	return true;
}

LPCSTR DbgGetFileNameFromID(const uchar* hash)
{
	CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)hash);
	if (reqfile != NULL)
		return reqfile->GetFileName();

	CPartFile* partfile = theApp.downloadqueue->GetFileByID((uchar*)hash);
	if (partfile != NULL)
		return partfile->GetFileName();

	return NULL;
}

CString DbgGetFileInfo(const uchar* hash)
{
	if (hash == NULL)
		return CString();

	CString strInfo(_T("File="));
	LPCSTR pszName = DbgGetFileNameFromID(hash);
	if (pszName != NULL)
		strInfo += pszName;
	else
		strInfo += md4str(hash);
	return strInfo;
}

int DbgGetHashType(const uchar* hash)
{
	if (hash[5] == 13 && hash[14] == 110)
		return SO_OLDEMULE;
	else if (hash[5] == 14 && hash[14] == 111)
		return SO_EMULE;
 	else if (hash[5] == 'M' && hash[14] == 'L')
		return SO_MLDONKEY;
	else
		return SO_UNKNOWN;
}

LPCTSTR DbgGetHashTypeString(const uchar* hash)
{
	int iHashType = DbgGetHashType(hash);
	if (iHashType == SO_EMULE)
		return _T("eMule");
	if (iHashType == SO_MLDONKEY)
		return _T("MLdonkey");
	if (iHashType == SO_OLDEMULE)
		return _T("Old eMule");
	ASSERT( iHashType == SO_UNKNOWN );
	return _T("Unknown");
}

CString DbgGetClientID(uint32 nClientID)
{
	CString strClientID;
	if (nClientID < 16777216)
		strClientID.Format(_T("LowID=%u"), nClientID);
	else
		strClientID = inet_ntoa(*(in_addr*)&nClientID);
	return strClientID;
}

bool	IsLowIDHybrid(uint32 id){
	return (id < 16777216);
}

bool	IsLowIDED2K(uint32 id){
	return (id < 16777216); //Need to verify what the highest LowID can be returned by the server.
}
// khaos::kmod+ Functions to return a random number within a given range.
int GetRandRange( int from, int to )
{
    int power;
    int number;
    if ( ( to = to - from + 1 ) <= 1 )
    	return from;
    for ( power = 2; power < to; power <<= 1 )
    	;
    while ( ( number = RandNum() & ( power - 1 ) ) >= to )
    	;
    return from + number;
}


int RandNum()
{
    int *piState;
    int iState1;
    int iState2;
    int iRand;
    piState		= &rgiState[2];
    iState1	 	= piState[-2];
    iState2	 	= piState[-1];
    iRand	 	= ( piState[iState1] + piState[iState2] )
    			& ( ( 1 << 30 ) - 1 );
    piState[iState1]	= iRand;
    if ( ++iState1 == 55 )
    	iState1 = 0;
    if ( ++iState2 == 55 )
    	iState2 = 0;
    piState[-2]		= iState1;
    piState[-1]		= iState2;
    return iRand >> 6;
}

void InitRandGen()
{
    int *piState;
    int iState;
    piState	= &rgiState[2];
    piState[-2]	= 55 - 55;
    piState[-1]	= 55 - 24;
    piState[0]	= ( (int) time( NULL ) ) & ( ( 1 << 30 ) - 1 );
    piState[1]	= 1;
    for ( iState = 2; iState < 55; iState++ )


        {
        	piState[iState] = ( piState[iState-1] + piState[iState-2] )
        			& ( ( 1 << 30 ) - 1 );
    }
    return;
}

// khaos::categorymod+
// Compares strings using wildcards * and ?.
// Credited To: Jack Handy
int wildcmp(char *wild, char *string)
{
	char *cp, *mp;

	while ((*string) && (*wild != '*'))
	{
		if ((*wild != *string) && (*wild != '?'))
			return 0;
		wild++;
		string++;
	}

	while (*string)
	{
		if (*wild == '*')
		{
			if (!*++wild)
				return 1;
			mp = wild;
			cp = string+1;
		}
		else if ((*wild == *string) || (*wild == '?'))
		{
			wild++;
			string++;
		}
		else
		{
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*')
		wild++;
	return !*wild;
}

ULONG CastXBytesToI(const CString& strExpr)
{
	ULONG ulNum;
	TCHAR szUnit[40];
	int iArgs = _stscanf(strExpr, _T("%u%s"), &ulNum, szUnit);
	if (iArgs <= 0)
		return 0;
	if (iArgs == 2){
		CString strUnits(szUnit);
		strUnits.Trim();
		if (!strUnits.IsEmpty()){
			//MORPH START - Fixed by SiRoB, Now Right For multilanguage
			CString strBytes = GetResString(IDS_BYTES);
			if (strUnits.CompareNoCase(_T(strBytes.Left(1))) == 0 || strUnits.CompareNoCase(_T(strBytes)) == 0)
				return ulNum * 1U; // Bytes
			else if (strUnits.CompareNoCase(_T("k")) == 0 || strUnits.CompareNoCase(_T(GetResString(IDS_KBYTES))) == 0 || strUnits.CompareNoCase(_T("k"+strBytes)) == 0)
				return ulNum * 1024U; // KBytes
			else if (strUnits.CompareNoCase(_T("m")) == 0 || strUnits.CompareNoCase(_T(GetResString(IDS_MBYTES))) == 0 || strUnits.CompareNoCase(_T("m"+strBytes)) == 0)
				return ulNum * 1024U*1024; // MBytes
			else if (strUnits.CompareNoCase(_T("g")) == 0 || strUnits.CompareNoCase(_T(GetResString(IDS_GBYTES))) == 0 || strUnits.CompareNoCase(_T("g"+strBytes)) == 0)
				return ulNum * 1024U*1024U*1024U; // GBytes
			else{
				AfxMessageBox(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + GetResString(IDS_SEARCH_INVALIDMINMAX));
				return 0;
			}
			//MORPH END - Fixed by SiRoB, Now Right For multilanguage
		}
	}

	return ulNum * 1024U*1024U; // Default = MBytes
}

CString CastItoUIXBytes(uint32 count)
{
	CString buffer;
	if (count < 1024)
		buffer.Format("%u%s", count, GetResString(IDS_BYTES));
	else if (count < 1048576)
		buffer.Format("%u%s", (uint32)(count/1024), GetResString(IDS_KBYTES));
	else if (count < 1073741824)
		buffer.Format("%u%s", (uint32)(count/1048576), GetResString(IDS_MBYTES));
	else
		buffer.Format("%u%s", (uint32)(count/1073741824), GetResString(IDS_GBYTES));
	return buffer;
}
// khaos::categorymod-
// khaos::kmod-