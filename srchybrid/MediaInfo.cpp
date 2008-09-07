//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include "resource.h"
#include "OtherFunctions.h"
#include "MediaInfo.h"
#include "SafeFile.h"
#include <io.h>
#include <fcntl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CStringStream& CStringStream::operator<<(LPCTSTR psz)
{
	str += psz;
	return *this;
}

CStringStream& CStringStream::operator<<(char* psz)
{
	str += psz;
	return *this;
}

CStringStream& CStringStream::operator<<(UINT uVal)
{
	CString strVal;
	strVal.Format(_T("%u"), uVal);
	str += strVal;
	return *this;
}

CStringStream& CStringStream::operator<<(int iVal)
{
	CString strVal;
	strVal.Format(_T("%d"), iVal);
	str += strVal;
	return *this;
}

CStringStream& CStringStream::operator<<(double fVal)
{
	CString strVal;
	strVal.Format(_T("%.3f"), fVal);
	str += strVal;
	return *this;
}

CString GetWaveFormatTagName(UINT uWavFmtTag, CString& rstrComment)
{
	const static struct _tagWavFmtTag
	{
		unsigned int	uFmtTag;
		const char	   *pszDefine;
		const char	   *pszComment;
	} WavFmtTag[] =
	{
		{ 0x0000, "Unknown", "" },
		{ 0x0001, "PCM", "" },
		{ 0x0002, "ADPCM", "" },
		{ 0x0003, "IEEE_FLOAT", "" },
		{ 0x0004, "VSELP", "Compaq Computer Corp." },
		{ 0x0005, "IBM_CVSD", "IBM Corporation" },
		{ 0x0006, "ALAW", "" },
		{ 0x0007, "MULAW", "" },
		{ 0x0008, "DTS", "" },
		{ 0x0009, "DRM", "" },
		{ 0x0010, "OKI_ADPCM", "OKI" },
		{ 0x0011, "DVI_ADPCM", "Intel Corporation" },
		{ 0x0012, "MEDIASPACE_ADPCM", "Videologic" },
		{ 0x0013, "SIERRA_ADPCM", "Sierra Semiconductor Corp" },
		{ 0x0014, "G723_ADPCM", "Antex Electronics Corporation" },
		{ 0x0015, "DIGISTD", "DSP Solutions, Inc." },
		{ 0x0016, "DIGIFIX", "DSP Solutions, Inc." },
		{ 0x0017, "DIALOGIC_OKI_ADPCM", "Dialogic Corporation" },
		{ 0x0018, "MEDIAVISION_ADPCM", "Media Vision, Inc." },
		{ 0x0019, "CU_CODEC", "Hewlett-Packard Company" },
		{ 0x0020, "YAMAHA_ADPCM", "Yamaha Corporation of America" },
		{ 0x0021, "SONARC", "Speech Compression" },
		{ 0x0022, "DSPGROUP_TRUESPEECH", "DSP Group, Inc" },
		{ 0x0023, "ECHOSC1", "Echo Speech Corporation" },
		{ 0x0024, "AUDIOFILE_AF36", "Virtual Music, Inc." },
		{ 0x0025, "APTX", "Audio Processing Technology" },
		{ 0x0026, "AUDIOFILE_AF10", "Virtual Music, Inc." },
		{ 0x0027, "PROSODY_1612", "Aculab plc" },
		{ 0x0028, "LRC", "Merging Technologies S.A." },
		{ 0x0030, "DOLBY_AC2", "Dolby Laboratories" },
		{ 0x0031, "GSM610", "" },
		{ 0x0032, "MSNAUDIO", "" },
		{ 0x0033, "ANTEX_ADPCME", "Antex Electronics Corporation" },
		{ 0x0034, "CONTROL_RES_VQLPC", "Control Resources Limited" },
		{ 0x0035, "DIGIREAL", "DSP Solutions, Inc." },
		{ 0x0036, "DIGIADPCM", "DSP Solutions, Inc." },
		{ 0x0037, "CONTROL_RES_CR10", "Control Resources Limited" },
		{ 0x0038, "NMS_VBXADPCM", "Natural MicroSystems" },
		{ 0x0039, "CS_IMAADPCM", "Crystal Semiconductor IMA ADPCM" },
		{ 0x003A, "ECHOSC3", "Echo Speech Corporation" },
		{ 0x003B, "ROCKWELL_ADPCM", "Rockwell International" },
		{ 0x003C, "ROCKWELL_DIGITALK", "Rockwell International" },
		{ 0x003D, "XEBEC", "Xebec Multimedia Solutions Limited" },
		{ 0x0040, "G721_ADPCM", "Antex Electronics Corporation" },
		{ 0x0041, "G728_CELP", "Antex Electronics Corporation" },
		{ 0x0042, "MSG723", "" },
		{ 0x0050, "MPEG-1, Layer 1", "" },
		{ 0x0051, "MPEG-1, Layer 2", "" },
		{ 0x0052, "RT24", "InSoft, Inc." },
		{ 0x0053, "PAC", "InSoft, Inc." },
		{ 0x0055, "MPEG-1, Layer 3", "" },
		{ 0x0059, "LUCENT_G723", "Lucent Technologies" },
		{ 0x0060, "CIRRUS", "Cirrus Logic" },
		{ 0x0061, "ESPCM", "ESS Technology" },
		{ 0x0062, "VOXWARE", "Voxware Inc" },
		{ 0x0063, "CANOPUS_ATRAC", "Canopus, co., Ltd." },
		{ 0x0064, "G726_ADPCM", "APICOM" },
		{ 0x0065, "G722_ADPCM", "APICOM" },
		{ 0x0067, "DSAT_DISPLAY", "" },
		{ 0x0069, "VOXWARE_BYTE_ALIGNED", "Voxware Inc" },
		{ 0x0070, "VOXWARE_AC8", "Voxware Inc" },
		{ 0x0071, "VOXWARE_AC10", "Voxware Inc" },
		{ 0x0072, "VOXWARE_AC16", "Voxware Inc" },
		{ 0x0073, "VOXWARE_AC20", "Voxware Inc" },
		{ 0x0074, "VOXWARE_RT24", "Voxware Inc" },
		{ 0x0075, "VOXWARE_RT29", "Voxware Inc" },
		{ 0x0076, "VOXWARE_RT29HW", "Voxware Inc" },
		{ 0x0077, "VOXWARE_VR12", "Voxware Inc" },
		{ 0x0078, "VOXWARE_VR18", "Voxware Inc" },
		{ 0x0079, "VOXWARE_TQ40", "Voxware Inc" },
		{ 0x0080, "SOFTSOUND", "Softsound, Ltd." },
		{ 0x0081, "VOXWARE_TQ60", "Voxware Inc" },
		{ 0x0082, "MSRT24", "" },
		{ 0x0083, "G729A", "AT&T Labs, Inc." },
		{ 0x0084, "MVI_MVI2", "Motion Pixels" },
		{ 0x0085, "DF_G726", "DataFusion Systems (Pty) (Ltd)" },
		{ 0x0086, "DF_GSM610", "DataFusion Systems (Pty) (Ltd)" },
		{ 0x0088, "ISIAUDIO", "Iterated Systems, Inc." },
		{ 0x0089, "ONLIVE", "OnLive! Technologies, Inc." },
		{ 0x0091, "SBC24", "Siemens Business Communications Sys" },
		{ 0x0092, "DOLBY_AC3_SPDIF", "Sonic Foundry" },
		{ 0x0093, "MEDIASONIC_G723", "MediaSonic" },
		{ 0x0094, "PROSODY_8KBPS", "Aculab plc" },
		{ 0x0097, "ZYXEL_ADPCM", "ZyXEL Communications, Inc." },
		{ 0x0098, "PHILIPS_LPCBB", "Philips Speech Processing" },
		{ 0x0099, "PACKED", "Studer Professional Audio AG" },
		{ 0x00A0, "MALDEN_PHONYTALK", "Malden Electronics Ltd." },
		{ 0x0100, "RHETOREX_ADPCM", "Rhetorex Inc." },
		{ 0x0101, "IRAT", "BeCubed Software Inc." },
		{ 0x0111, "VIVO_G723", "Vivo Software" },
		{ 0x0112, "VIVO_SIREN", "Vivo Software" },
		{ 0x0123, "DIGITAL_G723", "Digital Equipment Corporation" },
		{ 0x0125, "SANYO_LD_ADPCM", "Sanyo Electric Co., Ltd." },
		{ 0x0130, "Sipro ACEPLNET", "" },
		{ 0x0131, "Sipro ACELP4800", "" },
		{ 0x0132, "Sipro ACELP8V3", "" },
		{ 0x0133, "Sipro G729", "" },
		{ 0x0134, "Sipro G729A", "" },
		{ 0x0135, "Sipro KELVIN", "" },
		{ 0x0140, "G726ADPCM", "Dictaphone Corporation" },
		{ 0x0150, "QUALCOMM_PUREVOICE", "Qualcomm, Inc." },
		{ 0x0151, "QUALCOMM_HALFRATE", "Qualcomm, Inc." },
		{ 0x0155, "TUBGSM", "Ring Zero Systems, Inc." },
		{ 0x0160, "MSAUDIO1", "" },
		{ 0x0161, "DIVXAUDIO", "DivX ;-) Audio" },
		{ 0x0170, "UNISYS_NAP_ADPCM", "Unisys Corp." },
		{ 0x0171, "UNISYS_NAP_ULAW", "Unisys Corp." },
		{ 0x0172, "UNISYS_NAP_ALAW", "Unisys Corp." },
		{ 0x0173, "UNISYS_NAP_16K", "Unisys Corp." },
		{ 0x0200, "CREATIVE_ADPCM", "Creative Labs, Inc" },
		{ 0x0202, "CREATIVE_FASTSPEECH8", "Creative Labs, Inc" },
		{ 0x0203, "CREATIVE_FASTSPEECH10", "Creative Labs, Inc" },
		{ 0x0210, "UHER_ADPCM", "UHER informatic GmbH" },
		{ 0x0220, "QUARTERDECK", "Quarterdeck Corporation" },
		{ 0x0230, "ILINK_VC", "I-link Worldwide" },
		{ 0x0240, "RAW_SPORT", "Aureal Semiconductor" },
		{ 0x0241, "ESST_AC3", "ESS Technology, Inc." },
		{ 0x0250, "IPI_HSX", "Interactive Products, Inc." },
		{ 0x0251, "IPI_RPELP", "Interactive Products, Inc." },
		{ 0x0260, "CS2", "Consistent Software" },
		{ 0x0270, "SONY_SCX", "Sony Corp." },
		{ 0x0300, "FM_TOWNS_SND", "Fujitsu Corp." },
		{ 0x0400, "BTV_DIGITAL", "Brooktree Corporation" },
		{ 0x0401, "IMC", "Intel Music Coder for MSACM" },
		{ 0x0450, "QDESIGN_MUSIC", "QDesign Corporation" },
		{ 0x0680, "VME_VMPCM", "AT&T Labs, Inc." },
		{ 0x0681, "TPC", "AT&T Labs, Inc." },
		{ 0x1000, "OLIGSM", "Ing C. Olivetti & C., S.p.A." },
		{ 0x1001, "OLIADPCM", "Ing C. Olivetti & C., S.p.A." },
		{ 0x1002, "OLICELP", "Ing C. Olivetti & C., S.p.A." },
		{ 0x1003, "OLISBC", "Ing C. Olivetti & C., S.p.A." },
		{ 0x1004, "OLIOPR", "Ing C. Olivetti & C., S.p.A." },
		{ 0x1100, "LH_CODEC", "Lernout & Hauspie" },
		{ 0x1400, "NORRIS", "Norris Communications, Inc." },
		{ 0x1500, "SOUNDSPACE_MUSICOMPRESS", "AT&T Labs, Inc." },
		{ 0x2000, "AC3", "Dolby AC3" },
		{ 0x2001, "DTS", "Digital Theater Systems" },
		{ 0x2002, "Real Audio 1", "RealAudio 1/2 14.4" },
		{ 0x2003, "Real Audio 1", "RealAudio 1/2 28.8" },
		{ 0x2004, "Real Audio 2", "RealAudio G2/8 Cook (low bitrate)" },
		{ 0x2005, "Real Audio 3", "RealAudio 3/4/5 Music (DNET)" },
		{ 0x2006, "AAC", "RealAudio 10 AAC (RAAC)" },
		{ 0x2007, "AAC+", "RealAudio 10 AAC+ (RACP)" }
	};

	USES_CONVERSION;
	for (int i = 0; i < ARRSIZE(WavFmtTag); i++)
	{
		if (WavFmtTag[i].uFmtTag == uWavFmtTag){
			rstrComment = WavFmtTag[i].pszComment;
			return A2CT(WavFmtTag[i].pszDefine);
		}
	}

	CString strCompression;
	strCompression.Format(_T("0x%04x (unknown)"), uWavFmtTag);
	return strCompression;
}

CString GetWaveFormatTagName(UINT wFormatTag)
{
	CString strComment;
	CString strFormat = GetWaveFormatTagName(wFormatTag, strComment);
	if (!strComment.IsEmpty())
		strFormat += _T(" (") + strComment + _T(")");
	return strFormat;
}

BOOL IsEqualFOURCC(FOURCC fccA, FOURCC fccB)
{
	for (int i = 0; i < 4; i++)
	{
		if (tolower((unsigned char)fccA) != tolower((unsigned char)fccB))
			return FALSE;
		fccA >>= 8;
		fccB >>= 8;
	}
	return TRUE;
}

CString GetVideoFormatName(DWORD biCompression)
{
	CString strFormat;
	if (biCompression == BI_RGB)
		strFormat = _T("RGB");
	else if (biCompression == BI_RLE8)
		strFormat = _T("RLE8");
	else if (biCompression == BI_RLE4)
		strFormat = _T("RLE4");
	else if (biCompression == BI_BITFIELDS)
		strFormat = _T("Bitfields");
	else if (IsEqualFOURCC(biCompression, MAKEFOURCC('D', 'I', 'V', '3')))
		strFormat = _T("DIV3 (DivX ;-) MPEG-4 v3)");
	else if (IsEqualFOURCC(biCompression, MAKEFOURCC('D', 'I', 'V', '4')))
		strFormat = _T("DIV4 (DivX ;-) MPEG-4 v4)");
	else if (IsEqualFOURCC(biCompression, MAKEFOURCC('D', 'I', 'V', 'X')))
		strFormat = _T("DIVX (DivX)");
	else if (IsEqualFOURCC(biCompression, MAKEFOURCC('D', 'X', '5', '0')))
		strFormat = _T("DX50 (DivX 5)");
	else if (IsEqualFOURCC(biCompression, MAKEFOURCC('M', 'P', '4', '3')))
		strFormat = _T("MP43 (MS MPEG-4 v3)");
	else if (IsEqualFOURCC(biCompression, MAKEFOURCC('M', 'P', '4', '2')))
		strFormat = _T("MP42 (MS MPEG-4 v2)");
	else if (IsEqualFOURCC(biCompression, MAKEFOURCC('D', 'X', 'S', 'B')))
		strFormat = _T("DXSB (Subtitle)");
	else
	{
		char szFourcc[5];
		*(LPDWORD)szFourcc = biCompression;
		szFourcc[4] = '\0';
		strFormat = szFourcc;
		strFormat.MakeUpper();
	}
	return strFormat;
}

CString GetKnownAspectRatioDisplayString(float fAspectRatio)
{
	CString strAspectRatio;
         if (fAspectRatio >= 1.00F && fAspectRatio < 1.50F) strAspectRatio = _T("4/3");
    else if (fAspectRatio >= 1.50F && fAspectRatio < 2.00F) strAspectRatio = _T("16/9");
    else if (fAspectRatio >= 2.00F && fAspectRatio < 2.22F) strAspectRatio = _T("2.2");
    else if (fAspectRatio >= 2.22F && fAspectRatio < 2.30F) strAspectRatio = _T("2.25");
    else if (fAspectRatio >= 2.30F && fAspectRatio < 2.50F) strAspectRatio = _T("2.35");
	return strAspectRatio;
}

typedef struct
{
	SHORT	left;
	SHORT	top;
	SHORT	right;
	SHORT	bottom;
} RECT16;

typedef struct
{
    FOURCC		fccType;
    FOURCC		fccHandler;
    DWORD		dwFlags;
    WORD		wPriority;
    WORD		wLanguage;
    DWORD		dwInitialFrames;
    DWORD		dwScale;	
    DWORD		dwRate;
    DWORD		dwStart;
    DWORD		dwLength;
    DWORD		dwSuggestedBufferSize;
    DWORD		dwQuality;
    DWORD		dwSampleSize;
    RECT16		rcFrame;
} AVIStreamHeader_fixed;

#ifndef AVIFILEINFO_NOPADDING
#define AVIFILEINFO_NOPADDING	0x0400 // from the SDK tool "RIFFWALK.EXE"
#endif

#ifndef AVIFILEINFO_TRUSTCKTYPE
#define AVIFILEINFO_TRUSTCKTYPE	0x0800 // from DirectX SDK "Types of DV AVI Files"
#endif

typedef struct 
{
	AVIStreamHeader_fixed	hdr;
	DWORD					dwFormatLen;
	union
	{
		BITMAPINFOHEADER*   bmi;
		PCMWAVEFORMAT*		wav;
		LPBYTE				dat;
	} fmt;
	char*                   nam;
} STREAMHEADER;

static BOOL ReadChunkHeader(int fd, FOURCC *pfccType, DWORD *pdwLength)
{
	if (_read(fd, pfccType, sizeof(*pfccType)) != sizeof(*pfccType))
		return FALSE;
	if (_read(fd, pdwLength, sizeof(*pdwLength)) != sizeof(*pdwLength))
		return FALSE;
	return TRUE;
}

static BOOL ParseStreamHeader(int hAviFile, DWORD dwLengthLeft, STREAMHEADER* pStrmHdr)
{
	FOURCC fccType;
	DWORD dwLength;
	while (dwLengthLeft >= sizeof(DWORD)*2)
	{
		if (!ReadChunkHeader(hAviFile, &fccType, &dwLength))
			return FALSE;

		dwLengthLeft -= sizeof(DWORD)*2;
		if (dwLength > dwLengthLeft) {
			errno = 0;
			return FALSE;
		}
		dwLengthLeft -= dwLength + (dwLength & 1);

		switch (fccType)
		{
		case ckidSTREAMHEADER:
			if (dwLength < sizeof(pStrmHdr->hdr))
			{
				memset(&pStrmHdr->hdr, 0x00, sizeof(pStrmHdr->hdr));
				if (_read(hAviFile, &pStrmHdr->hdr, dwLength) != (int)dwLength)
					return FALSE;
				if (dwLength & 1) {
					if (_lseek(hAviFile, 1, SEEK_CUR) == -1)
						return FALSE;
				}
			}
			else
			{
				if (_read(hAviFile, &pStrmHdr->hdr, sizeof(pStrmHdr->hdr)) != sizeof(pStrmHdr->hdr))
					return FALSE;
				if (_lseek(hAviFile, dwLength + (dwLength & 1) - sizeof(pStrmHdr->hdr), SEEK_CUR) == -1)
					return FALSE;
			}
			dwLength = 0;
			break;

		case ckidSTREAMFORMAT:
			if (dwLength > 4096) // expect corrupt data
				return FALSE;
			if ((pStrmHdr->fmt.dat = new BYTE[pStrmHdr->dwFormatLen = dwLength]) == NULL) {
				errno = ENOMEM;
				return FALSE;
			}
			if (_read(hAviFile, pStrmHdr->fmt.dat, dwLength) != (int)dwLength)
				return FALSE;
			if (dwLength & 1) {
				if (_lseek(hAviFile, 1, SEEK_CUR) == -1)
					return FALSE;
			}
			dwLength = 0;
			break;

		case ckidSTREAMNAME:
			if (dwLength > 512) // expect corrupt data
				return FALSE;
			if ((pStrmHdr->nam = new char[dwLength + 1]) == NULL) {
				errno = ENOMEM;
				return FALSE;
			}
			if (_read(hAviFile, pStrmHdr->nam, dwLength) != (int)dwLength)
				return FALSE;
			pStrmHdr->nam[dwLength] = '\0';
			if (dwLength & 1) {
				if (_lseek(hAviFile, 1, SEEK_CUR) == -1)
					return FALSE;
			}
			dwLength = 0;
			break;
		}

		if (dwLength) {
			if (_lseek(hAviFile, dwLength + (dwLength & 1), SEEK_CUR) == -1)
				return FALSE;
		}
	}

	if (dwLengthLeft) {
		if (_lseek(hAviFile, dwLengthLeft, SEEK_CUR) == -1)
			return FALSE;
	}

	return TRUE;
}

BOOL GetRIFFHeaders(LPCTSTR pszFileName, SMediaInfo* mi, bool& rbIsAVI, bool bFullInfo)
{
	ASSERT( !bFullInfo || mi->strInfo.m_hWnd != NULL );

	BOOL bResult = FALSE;

	// Open AVI file
	int hAviFile = _topen(pszFileName, O_RDONLY | O_BINARY);
	if (hAviFile == -1)
		return FALSE;

	DWORD dwLengthLeft;
	FOURCC fccType;
	DWORD dwLength;
	BOOL bSizeInvalid = FALSE;
	int iStream = 0;
	DWORD dwMovieChunkSize = 0;
	DWORD uVideoFrames = 0;
	int	iNonAVStreams = 0;
	DWORD dwAllNonVideoAvgBytesPerSec = 0;

	//
	// Read 'RIFF' header
	//
	if (!ReadChunkHeader(hAviFile, &fccType, &dwLength))
		goto cleanup;
	if (fccType != FOURCC_RIFF)
		goto cleanup;
	if (dwLength < sizeof(DWORD))
	{
		dwLength = 0xFFFFFFF0;
		bSizeInvalid = TRUE;
	}
	dwLengthLeft = dwLength -= sizeof(DWORD);

	//
	// Read 'AVI ' or 'WAVE' header
	//
	FOURCC fccMain;
	if (_read(hAviFile, &fccMain, sizeof(fccMain)) != sizeof(fccMain))
		goto cleanup;
	if (fccMain == formtypeAVI)
		rbIsAVI = true;
	if (fccMain != formtypeAVI && fccMain != mmioFOURCC('W', 'A', 'V', 'E'))
		goto cleanup;

	// We need to read almost all streams (regardless of 'bFullInfo' mode) because we need to get the 'dwMovieChunkSize'
	BOOL bHaveReadAllStreams;
	bHaveReadAllStreams = FALSE;
	while (!bHaveReadAllStreams && dwLengthLeft >= sizeof(DWORD)*2)
	{
		if (!ReadChunkHeader(hAviFile, &fccType, &dwLength))
			goto inv_format_errno;
		if (fccType == 0 && dwLength == 0) {
			// We jumped right into a gap which is (still) filled with 0-bytes. If we 
			// continue reading this until EOF we throw an error although we may have
			// already read valid data.
			if (mi->iVideoStreams > 0 || mi->iAudioStreams > 0)
				break; // already have valid data
			goto cleanup;
		}

		BOOL bInvalidLength = FALSE;
		if (!bSizeInvalid)
		{
			dwLengthLeft -= sizeof(DWORD)*2;
			if (dwLength > dwLengthLeft)
			{
				if (fccType == FOURCC_LIST)
					bInvalidLength = TRUE;
				else
					goto cleanup;
			}
			dwLengthLeft -= (dwLength + (dwLength & 1));
		}

		switch (fccType)
		{
			case FOURCC_LIST:
				if (_read(hAviFile, &fccType, sizeof(fccType)) != sizeof(fccType))
					goto inv_format_errno;
				if (fccType != listtypeAVIHEADER && bInvalidLength)
					goto inv_format;

				// Some Premiere plugin is writing AVI files with an invalid size field in the LIST/hdrl chunk.
				if (dwLength < sizeof(DWORD) && fccType != listtypeAVIHEADER && (fccType != listtypeAVIMOVIE || !bSizeInvalid))
					goto inv_format;
				dwLength -= sizeof(DWORD);

				switch (fccType)
				{
					case listtypeAVIHEADER:
						dwLengthLeft += (dwLength + (dwLength&1)) + 4;
						dwLength = 0;	// silently enter the header block
						break;
					case listtypeSTREAMHEADER:
					{
						BOOL bStreamRes;
						STREAMHEADER strmhdr = {0};
						if ((bStreamRes = ParseStreamHeader(hAviFile, dwLength, &strmhdr)) != FALSE)
						{
							double fSamplesSec = (strmhdr.hdr.dwScale != 0) ? (double)strmhdr.hdr.dwRate / (double)strmhdr.hdr.dwScale : 0.0F;
							double fLength = (fSamplesSec != 0.0) ? (double)strmhdr.hdr.dwLength / fSamplesSec : 0.0;
							if (strmhdr.hdr.fccType == streamtypeAUDIO)
							{
								mi->iAudioStreams++;
								if (mi->iAudioStreams == 1)
								{
									mi->fAudioLengthSec = fLength;
									if (strmhdr.dwFormatLen >= sizeof(*strmhdr.fmt.wav) && strmhdr.fmt.wav)
									{
										*(PCMWAVEFORMAT*)&mi->audio = *strmhdr.fmt.wav;
										mi->strAudioFormat = GetWaveFormatTagName(strmhdr.fmt.wav->wf.wFormatTag);
									}
								}
								else
								{
									// this works only for CBR
									//
									// TODO: Determine VBR audio...
									if (strmhdr.dwFormatLen >= sizeof(*strmhdr.fmt.wav) && strmhdr.fmt.wav)
										dwAllNonVideoAvgBytesPerSec += strmhdr.fmt.wav->wf.nAvgBytesPerSec;

									if (bFullInfo && mi->strInfo.m_hWnd)
									{
										if (!mi->strInfo.IsEmpty())
											mi->strInfo << _T("\n");
										mi->OutputFileName();
										mi->strInfo.SetSelectionCharFormat(mi->strInfo.m_cfBold);
										mi->strInfo << GetResString(IDS_AUDIO) << _T(" #") << mi->iAudioStreams;
										if (strmhdr.nam && strmhdr.nam[0] != '\0')
											mi->strInfo << _T(": \"") << strmhdr.nam << _T("\"");
										mi->strInfo << _T("\n");
										if (strmhdr.dwFormatLen >= sizeof(*strmhdr.fmt.wav) && strmhdr.fmt.wav)
										{
											mi->strInfo << _T("   ") << GetResString(IDS_CODEC) << _T(":\t") << GetWaveFormatTagName(strmhdr.fmt.wav->wf.wFormatTag) << _T("\n");

											if (strmhdr.fmt.wav->wf.nAvgBytesPerSec)
											{
												CString strBitrate;
												if (strmhdr.fmt.wav->wf.nAvgBytesPerSec == (DWORD)-1)
													strBitrate = _T("Variable");
												else
													strBitrate.Format(_T("%u %s"), (UINT)(((strmhdr.fmt.wav->wf.nAvgBytesPerSec * 8.0) + 500.0) / 1000.0), GetResString(IDS_KBITSSEC));
												mi->strInfo << _T("   ") << GetResString(IDS_BITRATE) << _T(":\t") << strBitrate << _T("\n");
											}

											if (strmhdr.fmt.wav->wf.nChannels)
											{
												mi->strInfo << _T("   ") << GetResString(IDS_CHANNELS) << _T(":\t");
												if (strmhdr.fmt.wav->wf.nChannels == 1)
													mi->strInfo << _T("1 (Mono)");
												else if (strmhdr.fmt.wav->wf.nChannels == 2)
													mi->strInfo << _T("2 (Stereo)");
												else if (strmhdr.fmt.wav->wf.nChannels == 5)
													mi->strInfo << _T("5.1 (Surround)");
												else
													mi->strInfo << strmhdr.fmt.wav->wf.nChannels;
												mi->strInfo << _T("\n");
											}
											
											if (strmhdr.fmt.wav->wf.nSamplesPerSec)
												mi->strInfo << _T("   ") << GetResString(IDS_SAMPLERATE) << _T(":\t") << strmhdr.fmt.wav->wf.nSamplesPerSec / 1000.0 << _T(" kHz\n");
											
											if (strmhdr.fmt.wav->wBitsPerSample)
												mi->strInfo << _T("   Bit/sample:\t") << strmhdr.fmt.wav->wBitsPerSample << _T(" Bit\n");
										}
										if (fLength)
										{
											CString strLength;
											SecToTimeLength((ULONG)fLength, strLength);
											mi->strInfo << _T("   ") << GetResString(IDS_LENGTH) << _T(":\t") << strLength << _T("\n");
										}
									}
								}
							}
							else if (strmhdr.hdr.fccType == streamtypeVIDEO)
							{
								mi->iVideoStreams++;
								if (mi->iVideoStreams == 1)
								{
									uVideoFrames = strmhdr.hdr.dwLength;
									mi->fVideoLengthSec = fLength;
									mi->fVideoFrameRate = fSamplesSec;
									if (strmhdr.dwFormatLen >= sizeof(*strmhdr.fmt.bmi)  && strmhdr.fmt.bmi)
									{
										mi->video.bmiHeader = *strmhdr.fmt.bmi;
										mi->strVideoFormat = GetVideoFormatName(strmhdr.fmt.bmi->biCompression);
										if (strmhdr.fmt.bmi->biWidth && strmhdr.fmt.bmi->biHeight)
											mi->fVideoAspectRatio = (float)abs(strmhdr.fmt.bmi->biWidth) / (float)abs(strmhdr.fmt.bmi->biHeight);
									}
								}
								else
								{
									if (bFullInfo && mi->strInfo.m_hWnd)
									{
										if (!mi->strInfo.IsEmpty())
											mi->strInfo << _T("\n");
										mi->OutputFileName();
										mi->strInfo.SetSelectionCharFormat(mi->strInfo.m_cfBold);
										mi->strInfo << GetResString(IDS_VIDEO) << _T(" #") << mi->iVideoStreams;
										if (strmhdr.nam && strmhdr.nam[0] != '\0')
											mi->strInfo << _T(": \"") << strmhdr.nam << _T("\"");
										mi->strInfo << _T("\n");
										if (strmhdr.dwFormatLen >= sizeof(*strmhdr.fmt.bmi)  && strmhdr.fmt.bmi)
										{
											mi->strInfo << _T("   ") << GetResString(IDS_CODEC) << _T(":\t") << GetVideoFormatName(strmhdr.fmt.bmi->biCompression) << _T("\n");
											if (strmhdr.fmt.bmi->biWidth && strmhdr.fmt.bmi->biHeight)
											{
												mi->strInfo << _T("   ") << GetResString(IDS_WIDTH) << _T(" x ") << GetResString(IDS_HEIGHT) << _T(":\t") << abs(strmhdr.fmt.bmi->biWidth) << _T(" x ") << abs(strmhdr.fmt.bmi->biHeight) << _T("\n");
												float fAspectRatio = (float)abs(strmhdr.fmt.bmi->biWidth) / (float)abs(strmhdr.fmt.bmi->biHeight);
												mi->strInfo << _T("   ") << GetResString(IDS_ASPECTRATIO) << _T(":\t") << fAspectRatio << _T("  (") << GetKnownAspectRatioDisplayString(fAspectRatio) << _T(")\n");
											}
										}
										if (fLength)
										{
											CString strLength;
											SecToTimeLength((ULONG)fLength, strLength);
											mi->strInfo << _T("   ") << GetResString(IDS_LENGTH) << _T(":\t") << strLength << _T("\n");
										}
									}
								}
							}
							else
							{
								iNonAVStreams++;
								if (bFullInfo && mi->strInfo.m_hWnd)
								{
									if (!mi->strInfo.IsEmpty())
										mi->strInfo << _T("\n");
									mi->OutputFileName();
									mi->strInfo.SetSelectionCharFormat(mi->strInfo.m_cfBold);
									mi->strInfo << _T("Unknown Stream #") << iStream;
									if (strmhdr.nam && strmhdr.nam[0] != '\0')
										mi->strInfo << _T(": \"") << strmhdr.nam << _T("\"");
									mi->strInfo << _T("\n");
									if (fLength)
									{
										CString strLength;
										SecToTimeLength((ULONG)fLength, strLength);
										mi->strInfo << _T("   ") << GetResString(IDS_LENGTH) << _T(":\t") << strLength << _T("\n");
									}
								}
							}
						}
						delete[] strmhdr.fmt.dat;
						delete[] strmhdr.nam;
						if (!bStreamRes)
							goto inv_format_errno;
						iStream++;

						dwLength = 0;
						break;
					}
					case listtypeAVIMOVIE:
						dwMovieChunkSize = dwLength;
						if (!bFullInfo)
							bHaveReadAllStreams = TRUE;
						break;
					case mmioFOURCC('I', 'N', 'F', 'O'):
					{
						if (bFullInfo && mi->strInfo.m_hWnd && dwLength < 0x10000)
						{
							bool bError = false;
							BYTE* pChunk = new BYTE[dwLength];
							if ((UINT)_read(hAviFile, pChunk, dwLength) == dwLength)
							{
								CSafeMemFile ck(pChunk, dwLength);
								try {
									if (!mi->strInfo.IsEmpty())
										mi->strInfo << _T("\n");
									mi->OutputFileName();
									mi->strInfo.SetSelectionCharFormat(mi->strInfo.m_cfBold);
									mi->strInfo << GetResString(IDS_FD_GENERAL) << _T("\n");

									while (ck.GetPosition() < ck.GetLength())
									{
										FOURCC ckId = ck.ReadUInt32();
										DWORD ckLen = ck.ReadUInt32();
										CString strValue;
										if (ckLen < 512) {
											CStringA strValueA;
											ck.Read(strValueA.GetBuffer(ckLen), ckLen);
											strValueA.ReleaseBuffer(ckLen);
											strValue = strValueA;
											strValue.Trim();
										}
										else {
											ck.Seek(ckLen, CFile::current);
											strValue.Empty();
										}
										strValue.Replace(_T("\r"), _T(" "));
										strValue.Replace(_T("\n"), _T(" "));
										switch (ckId)
										{
											case mmioFOURCC('I', 'N', 'A', 'M'):
												mi->strInfo << _T("   ") << GetResString(IDS_TITLE) << _T(":\t") << strValue << _T("\n");
												break;
											case mmioFOURCC('I', 'S', 'B', 'J'):
												mi->strInfo << _T("   ") << _T("Subject") << _T(":\t") << strValue << _T("\n");
												break;
											case mmioFOURCC('I', 'A', 'R', 'T'):
												mi->strInfo << _T("   ") << GetResString(IDS_AUTHOR) << _T(":\t") << strValue << _T("\n");
												break;
											case mmioFOURCC('I', 'C', 'O', 'P'):
												mi->strInfo << _T("   ") << _T("Copyright") << _T(":\t") << strValue << _T("\n");
												break;
											case mmioFOURCC('I', 'C', 'M', 'T'):
												mi->strInfo << _T("   ") << GetResString(IDS_COMMENT) << _T(":\t") << strValue << _T("\n");
												break;
											case mmioFOURCC('I', 'C', 'R', 'D'):
												mi->strInfo << _T("   ") << GetResString(IDS_DATE) << _T(":\t") << strValue << _T("\n");
												break;
											case mmioFOURCC('I', 'S', 'F', 'T'):
												mi->strInfo << _T("   ") << _T("Software") << _T(":\t") << strValue << _T("\n");
												break;
										}

										if (ckLen & 1)
											ck.Seek(1, CFile::current);
									}
								}
								catch(CException* ex) {
									ex->Delete();
									bError = true;
								}
							}
							else {
								bError = true;
							}
							delete[] pChunk;

							if (bError) {
								bHaveReadAllStreams = TRUE;
							}
							else {
								if (dwLength & 1) {
									if (_lseek(hAviFile, 1, SEEK_CUR) == -1)
										bHaveReadAllStreams = TRUE;
								}
								dwLength = 0;
							}
						}
						break;
					}
				}
				break;

			case ckidAVIMAINHDR:
				if (dwLength == sizeof(MainAVIHeader))
				{
					MainAVIHeader avihdr;
					if (_read(hAviFile, &avihdr, sizeof(avihdr)) != sizeof(avihdr))
						goto inv_format_errno;
					if (dwLength & 1) {
						if (_lseek(hAviFile, 1, SEEK_CUR) == -1)
							goto inv_format_errno;
					}
					dwLength = 0;
				}
				break;

			case ckidAVINEWINDEX:	// idx1
				if (!bFullInfo)
					bHaveReadAllStreams = TRUE;
				break;

			case mmioFOURCC('f', 'm', 't', ' '):
				if (fccMain == mmioFOURCC('W', 'A', 'V', 'E'))
				{
					STREAMHEADER strmhdr = {0};
					if (dwLength > 4096) // expect corrupt data
						goto inv_format;
					if ((strmhdr.fmt.dat = new BYTE[strmhdr.dwFormatLen = dwLength]) == NULL) {
						errno = ENOMEM;
						goto inv_format_errno;
					}
					if (_read(hAviFile, strmhdr.fmt.dat, dwLength) != (int)dwLength)
						goto inv_format_errno;
					if (dwLength & 1) {
						if (_lseek(hAviFile, 1, SEEK_CUR) == -1)
							goto inv_format_errno;
					}
					dwLength = 0;

					strmhdr.hdr.fccType = streamtypeAUDIO;
					if (strmhdr.dwFormatLen)
					{
						mi->iAudioStreams++;
						if (mi->iAudioStreams == 1)
						{
							if (strmhdr.dwFormatLen >= sizeof(*strmhdr.fmt.wav) && strmhdr.fmt.wav)
							{
								*(PCMWAVEFORMAT*)&mi->audio = *strmhdr.fmt.wav;
								mi->strAudioFormat = GetWaveFormatTagName(strmhdr.fmt.wav->wf.wFormatTag);
							}
						}
					}
					delete[] strmhdr.fmt.dat;
					delete[] strmhdr.nam;
					iStream++;
					if (!bFullInfo)
						bHaveReadAllStreams = TRUE;
				}
				break;
		}

		if (bHaveReadAllStreams)
			break;
		if (dwLength)
		{
			if (_lseek(hAviFile, dwLength + (dwLength & 1), SEEK_CUR) == -1)
				goto inv_format_errno;
		}
	}

	if (fccMain == formtypeAVI)
	{
		mi->strFileFormat = _T("AVI");

		// NOTE: This video bitrate is published to ed2k servers and Kad, so, do everything to determine it right!
		if (mi->iVideoStreams == 1 /*&& mi->iAudioStreams <= 1*/ && iNonAVStreams == 0 && mi->fVideoLengthSec)
		{
			DWORD dwVideoFramesOverhead = uVideoFrames * (sizeof(WORD) + sizeof(WORD) + sizeof(DWORD));
			mi->video.dwBitRate = (DWORD)(((dwMovieChunkSize - dwVideoFramesOverhead) / mi->fVideoLengthSec - dwAllNonVideoAvgBytesPerSec/*mi->audio.nAvgBytesPerSec*/) * 8);
		}
	}
	else if (fccMain == mmioFOURCC('W', 'A', 'V', 'E'))
		mi->strFileFormat = _T("WAV (RIFF)");
	else
		mi->strFileFormat = _T("RIFF");

	mi->InitFileLength();
	bResult = TRUE;

cleanup:
	_close(hAviFile);
	return bResult;

inv_format:
	goto cleanup;

inv_format_errno:
	goto cleanup;
}

#pragma pack(push,1)
typedef struct {
	DWORD	id;
	DWORD	size;
} SRmChunkHdr;

typedef struct {
	DWORD   file_version;
	DWORD   num_headers;
} SRmRMF;

typedef struct {
	DWORD   max_bit_rate;
	DWORD   avg_bit_rate;
	DWORD   max_packet_size;
	DWORD   avg_packet_size;
	DWORD   num_packets;
	DWORD   duration;
	DWORD   preroll;
	DWORD   index_offset;
	DWORD   data_offset;
	WORD    num_streams;
	WORD    flags;
} SRmPROP;

typedef struct {
	WORD    stream_number;
	DWORD   max_bit_rate;
	DWORD   avg_bit_rate;
	DWORD   max_packet_size;
	DWORD   avg_packet_size;
	DWORD   start_time;
	DWORD   preroll;
	DWORD	duration;
	//BYTE                       stream_name_size;    
	//BYTE[stream_name_size]     stream_name;    
	//BYTE                       mime_type_size;    
	//BYTE[mime_type_size]       mime_type;    
	//DWORD                      type_specific_len;    
	//BYTE[type_specific_len]    type_specific_data;
} SRmMDPR;
#pragma pack(pop)

struct SRmFileProp {
	SRmFileProp()
	{}
	SRmFileProp(const CStringA& rstrName, const CStringA& rstrValue)
		: strName(rstrName), strValue(rstrValue)
	{}

	SRmFileProp& operator=(const SRmFileProp& r)
	{
		strName = r.strName;
		strValue = r.strValue;
		return *this;
	}
	CStringA strName;
	CStringA strValue;
};

CStringA GetFOURCCString(DWORD fcc)
{
	char szFourcc[5];
	szFourcc[0] = ((LPCSTR)&fcc)[0];
	szFourcc[1] = ((LPCSTR)&fcc)[1];
	szFourcc[2] = ((LPCSTR)&fcc)[2];
	szFourcc[3] = ((LPCSTR)&fcc)[3];
	szFourcc[4] = '\0';

	// Strip trailing spaces
	int i = 3;
	while (i >= 0)
	{
		if (szFourcc[i] == ' ')
			szFourcc[i] = '\0';
		else if (szFourcc[i] != '\0')
			break;
		i--;
	}

	// Convert to lower case
	/*if (bToLower)
	{
		while (i >= 0)
		{
			szFourcc[i] = (char)tolower((unsigned char)szFourcc[i]);
			i--;
		}
	}*/

	return szFourcc;
}

struct SRmCodec {
	LPCSTR	pszID;
	LPCTSTR pszName;
	LPCTSTR pszDesc;
} g_aRealMediaCodecs[] = {
{ "14.4", _T("RealAudio 1"),           _T("Real Audio 1 (14.4)") },
{ "14_4", _T("RealAudio 1"),           _T("Real Audio 1 (14.4)") },
{ "28.8", _T("RealAudio 2"),           _T("Real Audio 2 (28.8)") },
{ "28_8", _T("RealAudio 2"),           _T("Real Audio 2 (28.8)") },
{ "RV10", _T("RealVideo 1"),           _T("RealVideo 5") },
{ "RV13", _T("RealVideo 1"),           _T("RealVideo 5") },
{ "RV20", _T("RealVideo 2"),           _T("RealVideo G2") },
{ "RV30", _T("RealVideo 3"),           _T("RealVideo 8") },
{ "RV40", _T("RealVideo 4"),           _T("RealVideo 9") },
{ "atrc", _T("RealAudio Atrac3"),      _T("Real & Sony Atrac3 Codec") },
{ "cook", _T("RealAudio 5"),           _T("Real Audio G2/7 Cook (low bitrate)") },
{ "dnet", _T("RealAudio 3"),           _T("Real Audio 3/4/5 Music (DNET)") },
{ "lpcJ", _T("RealAudio 2"),           _T("Real Audio 1 (14.4)") },
{ "raac", _T("AAC"),                   _T("Real Audio 10 AAC (RAAC)") },
{ "racp", _T("AAC+"),                  _T("Real Audio 10 AAC+ (RACP)") },
{ "ralf", _T("RealAudio Lossless"),    _T("Real Audio Lossless Format") },
{ "rtrc", _T("RealAudio 8"),           _T("Real Audio 8 (RTRC)") },
{ "rv10", _T("RealVideo 1"),           _T("RealVideo 5") },
{ "rv20", _T("RealVideo 2"),           _T("RealVideo G2") },
{ "rv30", _T("RealVideo 3"),           _T("RealVideo 8") },
{ "rv40", _T("RealVideo 4"),           _T("RealVideo 9") },
{ "sipr", _T("RealAudio 4"),           _T("Real Audio 4 (Sipro)") },
};

static int __cdecl CmpRealMediaCodec(const void *p1, const void *p2)
{
	const SRmCodec *pCodec1 = (const SRmCodec *)p1;
	const SRmCodec *pCodec2 = (const SRmCodec *)p2;
	return strncmp(pCodec1->pszID, pCodec2->pszID, 4);
}

CString GetRealMediaCodecInfo(LPCSTR pszCodecID, bool bFullInfo = false)
{
	CString strInfo(GetFOURCCString(*(DWORD *)pszCodecID));
	SRmCodec CodecSearch;
	CodecSearch.pszID = pszCodecID;
	SRmCodec *pCodecFound = (SRmCodec *)bsearch(&CodecSearch, g_aRealMediaCodecs, _countof(g_aRealMediaCodecs), sizeof(g_aRealMediaCodecs[0]), CmpRealMediaCodec);
	if (pCodecFound)
	{
		strInfo += _T(" (");
		strInfo += pCodecFound->pszName;
		if (bFullInfo && pCodecFound->pszDesc)
		{
			strInfo += _T(", ");
			strInfo += pCodecFound->pszDesc;
		}
		strInfo += _T(")");
	}
	return strInfo;
}

BOOL GetRMHeaders(LPCTSTR pszFileName, SMediaInfo* mi, bool& rbIsRM, bool bFullInfo)
{
	ASSERT( !bFullInfo || mi->strInfo.m_hWnd != NULL );

	bool bResult = false;
	CSafeBufferedFile file;
	if (!file.Open(pszFileName, CFile::modeRead | CFile::osSequentialScan | CFile::typeBinary))
		return false;
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

	bool bReadPROP = false;
	bool bReadMDPR_Video = false;
	bool bReadMDPR_Audio = false;
	bool bReadMDPR_File = false;
	bool bReadCONT = false;
	UINT nFileBitrate = 0;
	CString strCopyright;
	CString strComment;
	CArray<SRmFileProp> aFileProps;
	try
	{
		ULONGLONG ullCurFilePos = 0;
		ULONGLONG ullChunkStartFilePos = ullCurFilePos;
		ULONGLONG ullChunkEndFilePos;

		SRmChunkHdr rmChunkHdr;
		file.Read(&rmChunkHdr, sizeof(rmChunkHdr));
		DWORD dwID = _byteswap_ulong(rmChunkHdr.id);
		if (dwID != '.RMF')
			return false;
		DWORD dwSize = _byteswap_ulong(rmChunkHdr.size);
		if (dwSize < sizeof(rmChunkHdr))
			return false;
		ullChunkEndFilePos = ullChunkStartFilePos + dwSize;
		dwSize -= sizeof(rmChunkHdr);
		if (dwSize == 0)
			return false;

		WORD wVersion;
		file.Read(&wVersion, sizeof(wVersion));
		wVersion = _byteswap_ushort(wVersion);
		if (wVersion >= 2)
			return false;

		SRmRMF rmRMF;
		file.Read(&rmRMF, sizeof(rmRMF));

		rbIsRM = true;
		mi->strFileFormat = _T("Real Media");

		bool bBrokenFile = false;
		while (!bBrokenFile && (!bReadCONT || !bReadPROP || !bReadMDPR_Video || !bReadMDPR_Audio || (bFullInfo && !bReadMDPR_File)))
		{
			ullCurFilePos = file.GetPosition();
			if (ullCurFilePos > ullChunkEndFilePos)
				break; // expect broken DATA-chunk headers created by 'mplayer'
			if (ullCurFilePos < ullChunkEndFilePos)
				ullCurFilePos = file.Seek(ullChunkEndFilePos - ullCurFilePos, CFile::current);

			ullChunkStartFilePos = ullCurFilePos;

			file.Read(&rmChunkHdr, sizeof(rmChunkHdr));
			dwID = _byteswap_ulong(rmChunkHdr.id);
			dwSize = _byteswap_ulong(rmChunkHdr.size);
			TRACE("%08x: id=%.4s  size=%u\n", (DWORD)ullCurFilePos, &rmChunkHdr.id, dwSize);
			if (dwSize < sizeof(rmChunkHdr))
				break; // expect broken DATA-chunk headers created by 'mplayer'
			ullChunkEndFilePos = ullChunkStartFilePos + dwSize;
			dwSize -= sizeof(rmChunkHdr);
			if (dwSize > 0)
			{
				switch (dwID)
				{
					case 'PROP': { // Properties Header
						WORD wVersion;
						file.Read(&wVersion, sizeof(wVersion));
						wVersion = _byteswap_ushort(wVersion);
						if (wVersion == 0)
						{
							SRmPROP rmPROP;
							file.Read(&rmPROP, sizeof(rmPROP));
							nFileBitrate = _byteswap_ulong(rmPROP.avg_bit_rate);
							mi->fFileLengthSec = (_byteswap_ulong(rmPROP.duration) + 500.0) / 1000.0;
							bReadPROP = true;
						}
						break;
					}
					case 'MDPR': { // Media Properties Header
						WORD wVersion;
						file.Read(&wVersion, sizeof(wVersion));
						wVersion = _byteswap_ushort(wVersion);
						if (wVersion == 0)
						{
							SRmMDPR rmMDPR;
							file.Read(&rmMDPR, sizeof(rmMDPR));

							// Read 'Stream Name'
							BYTE ucLen;
							CStringA strStreamName;
							file.Read(&ucLen, sizeof(ucLen));
							file.Read(strStreamName.GetBuffer(ucLen), ucLen);
							strStreamName.ReleaseBuffer(ucLen);

							// Read 'MIME Type'
							CStringA strMimeType;
							file.Read(&ucLen, sizeof(ucLen));
							file.Read(strMimeType.GetBuffer(ucLen), ucLen);
							strMimeType.ReleaseBuffer(ucLen);

							DWORD dwTypeDataLen;
							file.Read(&dwTypeDataLen, sizeof(dwTypeDataLen));
							dwTypeDataLen = _byteswap_ulong(dwTypeDataLen);

							if (strMimeType == "video/x-pn-realvideo")
							{
								mi->iVideoStreams++;
								if (mi->iVideoStreams == 1)
								{
									mi->video.dwBitRate = _byteswap_ulong(rmMDPR.avg_bit_rate);
									mi->fVideoLengthSec = _byteswap_ulong(rmMDPR.duration) / 1000.0;

									if (dwTypeDataLen >= 22+2 && dwTypeDataLen < 8192)
									{
										BYTE *pucTypeData = new BYTE[dwTypeDataLen];
										try {
											file.Read(pucTypeData, dwTypeDataLen);
											if (_byteswap_ulong(*(DWORD *)(pucTypeData + 4)) == 'VIDO')
											{
												mi->strVideoFormat = GetRealMediaCodecInfo((LPCSTR)(pucTypeData + 8));
												mi->video.bmiHeader.biCompression = *(DWORD *)(pucTypeData + 8);
												mi->video.bmiHeader.biWidth = _byteswap_ushort(*(WORD *)(pucTypeData + 12));
												mi->video.bmiHeader.biHeight = _byteswap_ushort(*(WORD *)(pucTypeData + 14));
												mi->fVideoAspectRatio = (float)abs(mi->video.bmiHeader.biWidth) / (float)abs(mi->video.bmiHeader.biHeight);
												mi->fVideoFrameRate = _byteswap_ushort(*(WORD *)(pucTypeData + 22));
												bReadMDPR_Video = true;
											}
										}
										catch (CException* ex) {
											ex->Delete();
											delete[] pucTypeData;
											break;
										}
										delete[] pucTypeData;
									}
								}
							}
							else if (strMimeType == "audio/x-pn-realaudio")
							{
								mi->iAudioStreams++;
								if (mi->iAudioStreams == 1)
								{
									mi->audio.nAvgBytesPerSec = _byteswap_ulong(rmMDPR.avg_bit_rate) / 8;
									mi->fAudioLengthSec = _byteswap_ulong(rmMDPR.duration) / 1000.0;
									
									if (dwTypeDataLen >= 4+2 && dwTypeDataLen < 8192)
									{
										BYTE *pucTypeData = new BYTE[dwTypeDataLen];
										try {
											file.Read(pucTypeData, dwTypeDataLen);
											DWORD dwFourCC = _byteswap_ulong(*(DWORD *)(pucTypeData + 0));
											WORD wVer = _byteswap_ushort(*(WORD *)(pucTypeData + 4));
											if (dwFourCC == '.ra\xFD')
											{
												if (wVer == 3)
												{
													if (dwTypeDataLen >= 8+2)
													{
														mi->audio.nSamplesPerSec = 8000;
														mi->audio.nChannels = _byteswap_ushort(*(WORD *)(pucTypeData + 8));
														mi->strAudioFormat = _T(".ra3");
														bReadMDPR_Audio = true;
													}
												}
												else if (wVer == 4) // RealAudio G2, RealAudio 8
												{
													if (dwTypeDataLen >= 62+4)
													{
														mi->audio.nSamplesPerSec = _byteswap_ushort(*(WORD *)(pucTypeData + 48));
														mi->audio.nChannels = _byteswap_ushort(*(WORD *)(pucTypeData + 54));
														if (strncmp((LPCSTR)(pucTypeData + 62), "sipr", 4) == 0)
															mi->audio.wFormatTag = 0x0130;
														else if (strncmp((LPCSTR)(pucTypeData + 62), "cook", 4) == 0)
															mi->audio.wFormatTag = 0x2004;
														mi->strAudioFormat = GetRealMediaCodecInfo((LPCSTR)(pucTypeData + 62));
														bReadMDPR_Audio = true;
													}
												}
												else if (wVer == 5)
												{
													if (dwTypeDataLen >= 66+4)
													{
														mi->audio.nSamplesPerSec = _byteswap_ulong(*(DWORD *)(pucTypeData + 48));
														mi->audio.nChannels = _byteswap_ushort(*(WORD *)(pucTypeData + 60));
														if (strncmp((LPCSTR)(pucTypeData + 62), "sipr", 4) == 0)
															mi->audio.wFormatTag = 0x0130;
														else if (strncmp((LPCSTR)(pucTypeData + 62), "cook", 4) == 0)
															mi->audio.wFormatTag = 0x2004;
														mi->strAudioFormat = GetRealMediaCodecInfo((LPCSTR)(pucTypeData + 66));
														bReadMDPR_Audio = true;
													}
												}
											}
										}
										catch (CException* ex) {
											ex->Delete();
											delete[] pucTypeData;
											break;
										}
										delete[] pucTypeData;
									}
								}
							}
							else if (bFullInfo && strcmp(strMimeType, "logical-fileinfo") == 0)
							{
								DWORD dwLogStreamLen;
								file.Read(&dwLogStreamLen, sizeof(dwLogStreamLen));
								dwLogStreamLen = _byteswap_ulong(dwLogStreamLen);

								WORD wVersion;
								file.Read(&wVersion, sizeof(wVersion));
								wVersion = _byteswap_ushort(wVersion);
								if (wVersion == 0)
								{
									WORD wStreams;
									file.Read(&wStreams, sizeof(wStreams));
									wStreams = _byteswap_ushort(wStreams);

									// Skip 'Physical Stream Numbers'
									file.Seek(wStreams * sizeof(WORD), CFile::current);

									// Skip 'Data Offsets'
									file.Seek(wStreams * sizeof(DWORD), CFile::current);

									WORD wRules;
									file.Read(&wRules, sizeof(wRules));
									wRules = _byteswap_ushort(wRules);

									// Skip 'Rule to Physical Stream Number Map'
									file.Seek(wRules * sizeof(WORD), CFile::current);

									WORD wProperties;
									file.Read(&wProperties, sizeof(wProperties));
									wProperties = _byteswap_ushort(wProperties);

									while (wProperties)
									{
										DWORD dwPropSize;
										file.Read(&dwPropSize, sizeof(dwPropSize));
										dwPropSize = _byteswap_ulong(dwPropSize);

										WORD wPropVersion;
										file.Read(&wPropVersion, sizeof(wPropVersion));
										wPropVersion = _byteswap_ushort(wPropVersion);

										if (wPropVersion == 0)
										{
											BYTE ucLen;
											CStringA strPropNameA;
											file.Read(&ucLen, sizeof(ucLen));
											file.Read(strPropNameA.GetBuffer(ucLen), ucLen);
											strPropNameA.ReleaseBuffer(ucLen);

											DWORD dwPropType;
											file.Read(&dwPropType, sizeof(dwPropType));
											dwPropType = _byteswap_ulong(dwPropType);

											WORD wPropValueLen;
											file.Read(&wPropValueLen, sizeof(wPropValueLen));
											wPropValueLen = _byteswap_ushort(wPropValueLen);

											CStringA strPropValueA;
											if (dwPropType == 0 && wPropValueLen == sizeof(DWORD))
											{
												DWORD dwPropValue;
												file.Read(&dwPropValue, sizeof(dwPropValue));
												dwPropValue = _byteswap_ulong(dwPropValue);
												strPropValueA.Format("%u", dwPropValue);
											}
											else if (dwPropType == 2)
											{
												LPSTR pszA = strPropValueA.GetBuffer(wPropValueLen);
												file.Read(pszA, wPropValueLen);
												if (wPropValueLen > 0 && pszA[wPropValueLen - 1] == '\0')
													wPropValueLen--;
												strPropValueA.ReleaseBuffer(wPropValueLen);
											}
											else
											{
												file.Seek(wPropValueLen, CFile::current);
												strPropValueA.Format("<%u bytes>", wPropValueLen);
											}

											aFileProps.Add(SRmFileProp(strPropNameA, strPropValueA));
											TRACE("Prop: %s, typ=%u, size=%u, value=%s\n", strPropNameA, dwPropType, wPropValueLen, (LPCSTR)strPropValueA);
										}
										else
											file.Seek(dwPropSize - sizeof(DWORD) - sizeof(WORD), CFile::current);

										wProperties--;
									}
									bReadMDPR_File = true;
								}
							}
						}
						break;
					}
					case 'CONT': { // Content Description Header
						WORD wVersion;
						file.Read(&wVersion, sizeof(wVersion));
						wVersion = _byteswap_ushort(wVersion);
						if (wVersion == 0)
						{
							WORD wLen;
							CStringA strA;
							file.Read(&wLen, sizeof(wLen));
							wLen = _byteswap_ushort(wLen);
							file.Read(strA.GetBuffer(wLen), wLen);
							strA.ReleaseBuffer(wLen);
							mi->strTitle = strA;

							file.Read(&wLen, sizeof(wLen));
							wLen = _byteswap_ushort(wLen);
							file.Read(strA.GetBuffer(wLen), wLen);
							strA.ReleaseBuffer(wLen);
							mi->strAuthor = strA;

							file.Read(&wLen, sizeof(wLen));
							wLen = _byteswap_ushort(wLen);
							file.Read(strA.GetBuffer(wLen), wLen);
							strA.ReleaseBuffer(wLen);
							strCopyright = strA;

							file.Read(&wLen, sizeof(wLen));
							wLen = _byteswap_ushort(wLen);
							file.Read(strA.GetBuffer(wLen), wLen);
							strA.ReleaseBuffer(wLen);
							strComment = strA;
							bReadCONT = true;
						}
						break;
					}
					
					case 'DATA':
					case 'INDX':
					case 'RMMD':
					case 'RMJE':
						// Expect broken DATA-chunk headers created by 'mplayer'. Thus catch
						// all valid tags just to have chance to detect the broken ones.
						break;
					
					default:
						// Expect broken DATA-chunk headers created by 'mplayer'. Stop reading
						// the file on first broken chunk header.
						bBrokenFile = true;
						break;
				}
			}
		}
	}
	catch(CException *ex)
	{
		ex->Delete();
	}

	// Expect broken DATA-chunk headers created by 'mplayer'. A broken DATA-chunk header
	// may not be a fatal error. We may have already successfully read the media properties
	// headers. Therefore we indicate 'success' if we read at least some valid headers.
	bResult = bReadCONT || bReadPROP || bReadMDPR_Video || bReadMDPR_Audio;
	if (bResult)
	{
		mi->InitFileLength();
		if (   bFullInfo
			&& mi->strInfo.m_hWnd
			&& (   nFileBitrate
			    || !mi->strTitle.IsEmpty() 
				|| !mi->strAuthor.IsEmpty()
				|| !strCopyright.IsEmpty()
				|| !strComment.IsEmpty()
				|| aFileProps.GetSize()))
		{
			if (!mi->strInfo.IsEmpty())
				mi->strInfo << _T("\n");
			mi->OutputFileName();
			mi->strInfo.SetSelectionCharFormat(mi->strInfo.m_cfBold);
			mi->strInfo << GetResString(IDS_FD_GENERAL) << _T("\n");

			if (nFileBitrate) {
				CString strBitrate;
				strBitrate.Format(_T("%u %s"), (UINT)((nFileBitrate + 500.0) / 1000.0), GetResString(IDS_KBITSSEC));
				mi->strInfo << _T("   ") << GetResString(IDS_BITRATE) << _T(":\t") << strBitrate << _T("\n");
			}
			if (!mi->strTitle.IsEmpty())
				mi->strInfo << _T("   ") << GetResString(IDS_TITLE) << _T(":\t") << mi->strTitle << _T("\n");
			if (!mi->strAuthor.IsEmpty())
				mi->strInfo << _T("   ") << GetResString(IDS_AUTHOR) << _T(":\t") << mi->strAuthor << _T("\n");
			if (!strCopyright.IsEmpty())
				mi->strInfo << _T("   ") << _T("Copyright") << _T(":\t") << strCopyright << _T("\n");
			if (!strComment.IsEmpty())
				mi->strInfo << _T("   ") << GetResString(IDS_COMMENT) << _T(":\t") << strComment << _T("\n");
			for (int i = 0; i < aFileProps.GetSize(); i++) {
				if (!aFileProps[i].strValue.IsEmpty())
					mi->strInfo << _T("   ") << CString(aFileProps[i].strName) << _T(":\t") << CString(aFileProps[i].strValue) << _T("\n");
			}
		}
	}

	return bResult;
}

bool GetMimeType(LPCTSTR pszFilePath, CString& rstrMimeType)
{
	int fd = _topen(pszFilePath, O_RDONLY | O_BINARY);
	if (fd != -1)
	{
		BYTE aucBuff[8192];
		int iRead = _read(fd, aucBuff, sizeof aucBuff);
		
		_close(fd);
		fd = -1;

		if (iRead > 0)
		{
			USES_CONVERSION;
			LPWSTR pwszMime = NULL;
			// Don't pass the file name to 'FindMimeFromData'. In case 'FindMimeFromData' can not determine the MIME type
			// from sniffing the header data it will parse the passed file name's extension to guess the MIME type.
			// That's basically OK for browser mode, but we can't use that here.
			HRESULT hr = FindMimeFromData(NULL, NULL/*T2CW(pszFilePath)*/, aucBuff, iRead, NULL, 0, &pwszMime, 0);
			if (SUCCEEDED(hr) && pwszMime != NULL && wcscmp(pwszMime, L"application/octet-stream") != 0 /*&& wcscmp(pwszMime, L"text/plain") != 0*/) {
				rstrMimeType = W2T(pwszMime);
				return true;
			}

			// RAR file type
			if (iRead >= 7 && aucBuff[0] == 0x52) {
				if (   (aucBuff[1] == 0x45 && aucBuff[2] == 0x7e && aucBuff[3] == 0x5e)
					|| (aucBuff[1] == 0x61 && aucBuff[2] == 0x72 && aucBuff[3] == 0x21 && aucBuff[4] == 0x1a && aucBuff[5] == 0x07 && aucBuff[6] == 0x00)) {
					rstrMimeType = _T("application/x-rar-compressed");
					return true;
				}
			}

			// bzip (BZ2) file type
			if (aucBuff[0] == 'B' && aucBuff[1] == 'Z' && aucBuff[2] == 'h' && (aucBuff[3] >= '1' && aucBuff[3] <= '9')) {
				rstrMimeType = _T("application/x-bzip-compressed");
				return true;
			}

			// ACE file type
			static const char _cACEheader[] = "**ACE**";
			if (iRead >= 7 + _countof(_cACEheader)-1 && memcmp(&aucBuff[7], _cACEheader, _countof(_cACEheader)-1) == 0) {
				rstrMimeType = _T("application/x-ace-compressed");
				return true;
			}

			// LHA/LZH file type
			static const char _cLZHheader[] = "-lh5-";
			if (iRead >= 2 + _countof(_cLZHheader)-1 && memcmp(&aucBuff[2], _cLZHheader, _countof(_cLZHheader)-1) == 0) {
				rstrMimeType = _T("application/x-lha-compressed");
				return true;
			}
		}
	}
	return false;
}

const BYTE *FindPattern(const BYTE *pucBuff, int iBuffSize, const BYTE *pucPattern, int iPatternSize)
{
	int iSearchRange = iBuffSize - iPatternSize;
	while (iSearchRange-- >= 0) {
		if (memcmp(pucBuff, pucPattern, iPatternSize) == 0)
			return pucBuff;
		pucBuff++;
	}
	return NULL;
}

bool GetDRM(LPCTSTR pszFilePath)
{
	int fd = _topen(pszFilePath, O_RDONLY | O_BINARY);
	if (fd != -1)
	{
		BYTE aucBuff[8192];
		int iRead = _read(fd, aucBuff, sizeof aucBuff);
		_close(fd);
		fd = -1;

		if (iRead > 0)
		{
			static const WCHAR wszWrmHdr[] = L"<WRMHEADER";
			if (FindPattern(aucBuff, sizeof aucBuff, (const BYTE *)wszWrmHdr, sizeof(wszWrmHdr) - sizeof(wszWrmHdr[0])))
				return true;
		}
	}
	return false;
}
