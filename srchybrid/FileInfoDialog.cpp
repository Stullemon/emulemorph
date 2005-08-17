//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include <io.h>
#include <fcntl.h>
#include "eMule.h"
#include "FileInfoDialog.h"
#include "OtherFunctions.h"
#include "PartFile.h"
#include "Preferences.h"
#include "UserMsgs.h"

// id3lib
#include <id3/tag.h>
#include <id3/misc_support.h>

// DirectShow MediaDet
#include <strmif.h>
//#include <uuids.h>
#define _DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
_DEFINE_GUID(MEDIATYPE_Video, 0x73646976, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
_DEFINE_GUID(MEDIATYPE_Audio, 0x73647561, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
_DEFINE_GUID(FORMAT_VideoInfo,0x05589f80, 0xc356, 0x11ce, 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a);
_DEFINE_GUID(FORMAT_WaveFormatEx,0x05589f81, 0xc356, 0x11ce, 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a);
//#define MMNODRV		// mmsystem: Installable driver support
#define MMNOSOUND		// mmsystem: Sound support
//#define MMNOWAVE		// mmsystem: Waveform support
#define MMNOMIDI		// mmsystem: MIDI support
#define MMNOAUX			// mmsystem: Auxiliary audio support
#define MMNOMIXER		// mmsystem: Mixer support
#define MMNOTIMER		// mmsystem: Timer support
#define MMNOJOY			// mmsystem: Joystick support
#define MMNOMCI			// mmsystem: MCI support
//#define MMNOMMIO		// mmsystem: Multimedia file I/O support
#define MMNOMMSYSTEM	// mmsystem: General MMSYSTEM functions
#include <qedit.h>
typedef struct tagVIDEOINFOHEADER {
	RECT			rcSource;		   // The bit we really want to use
	RECT			rcTarget;		   // Where the video should go
	DWORD			dwBitRate;		   // Approximate bit data rate
	DWORD			dwBitErrorRate;	   // Bit error rate for this stream
	REFERENCE_TIME	AvgTimePerFrame;   // Average time per frame (100ns units)
	BITMAPINFOHEADER bmiHeader;
} VIDEOINFOHEADER;

// MediaInfoDLL
/** @brief Kinds of Stream */
typedef enum _stream_t
{
    Stream_General,
    Stream_Video,
    Stream_Audio,
    Stream_Text,
    Stream_Chapters,
    Stream_Image,
    Stream_Max
} stream_t_C;

/** @brief Kinds of Info */
typedef enum _info_t
{
    Info_Name,
    Info_Text,
    Info_Measure,
    Info_Options,
    Info_Name_Text,
    Info_Measure_Text,
    Info_Info,
    Info_HowTo,
    Info_Max
} info_t_C;


// Those defines are for 'mmreg.h' which is included by 'vfw.h'
#define NOMMIDS		 // Multimedia IDs are not defined
//#define NONEWWAVE	   // No new waveform types are defined except WAVEFORMATEX
#define NONEWRIFF	 // No new RIFF forms are defined
#define NOJPEGDIB	 // No JPEG DIB definitions
#define NONEWIC		 // No new Image Compressor types are defined
#define NOBITMAP	 // No extended bitmap info header definition
// Those defines are for 'vfw.h'
//#define NOCOMPMAN
//#define NODRAWDIB
#define NOVIDEO
//#define NOAVIFMT
//#define NOMMREG
//#define NOAVIFILE
#define NOMCIWND
#define NOAVICAP
#define NOMSACM
#include <vfw.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CStringStream

class CStringStream
{
public:
	CStringStream(){}

	CStringStream& operator<<(LPCTSTR psz);
	CStringStream& operator<<(char* psz);
	CStringStream& operator<<(UINT uVal);
	CStringStream& operator<<(int iVal);
	CStringStream& operator<<(double fVal);

	CString str;
};

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


/////////////////////////////////////////////////////////////////////////////
// SMediaInfo

struct SMediaInfo
{
	SMediaInfo()
	{
		(void)strFileFormat;
		(void)strMimeType;
		ulFileSize = 0;

		iVideoStreams = 0;
		(void)strVideoFormat;
		memset(&video, 0, sizeof video);
		fVideoLengthSec = 0.0;
		fVideoFrameRate = 0.0;
		fVideoAspectRatio = 0.0;

		iAudioStreams = 0;
		(void)strAudioFormat;
		memset(&audio, 0, sizeof audio);
		fAudioLengthSec = 0.0;
	}

	CString			strFileFormat;
	CString			strMimeType;
	ULONG			ulFileSize;

	int				iVideoStreams;
	CString			strVideoFormat;
	VIDEOINFOHEADER	video;
	double			fVideoLengthSec;
	double			fVideoFrameRate;
	double			fVideoAspectRatio;

	int				iAudioStreams;
	CString			strAudioFormat;
	WAVEFORMAT		audio;
	double			fAudioLengthSec;

	CStringStream	strInfo;
};


/////////////////////////////////////////////////////////////////////////////
// CGetMediaInfoThread

class CGetMediaInfoThread : public CWinThread
{
	DECLARE_DYNCREATE(CGetMediaInfoThread)

protected:
	CGetMediaInfoThread()
	{
		m_hWndOwner = NULL;
		m_paFiles = NULL;
	}

public:
	virtual BOOL InitInstance();
	virtual int	Run();
	void SetValues(HWND hWnd, const CSimpleArray<CObject*>* paFiles)
	{
		m_hWndOwner = hWnd;
		m_paFiles = paFiles;
	}

private:
	HWND m_hWndOwner;
	const CSimpleArray<CObject*>* m_paFiles;
};


/////////////////////////////////////////////////////////////////////////////
// CMediaInfoDLL

class CMediaInfoDLL
{
public:
	CMediaInfoDLL()
	{
		m_bInitialized = FALSE;
		m_hLib = NULL;
		m_ullVersion = 0;
		// MediaInfoLib - v0.4.0.1
		m_pfnMediaInfo4_Open = NULL;
		m_pfnMediaInfo4_Close = NULL;
		m_pfnMediaInfo4_Get = NULL;
		m_pfnMediaInfo4_Count_Get = NULL;
		// MediaInfoLib - v0.5 - v0.6.1
		m_pfnMediaInfo5_Open = NULL;
		// MediaInfoLib - v0.7+
		m_pfnMediaInfo_New = NULL;
		m_pfnMediaInfo_Delete = NULL;
		m_pfnMediaInfo_Open = NULL;
		m_pfnMediaInfo_Close = NULL;
		m_pfnMediaInfo_Get = NULL;
		m_pfnMediaInfo_Count_Get = NULL;
	}
	~CMediaInfoDLL()
	{
		if (m_hLib)
			FreeLibrary(m_hLib);
	}

	BOOL Initialize()
	{
		if (!m_bInitialized)
		{
			m_bInitialized = TRUE;

			m_hLib = LoadLibrary(_T("MEDIAINFO.DLL"));
			if (m_hLib != NULL)
			{
				ULONGLONG ullVersion = GetModuleVersion(m_hLib);
				if (ullVersion == 0) // MediaInfoLib - v0.4.0.1 does not have a Win32 version info resource record
				{
					char* (__stdcall *fpMediaInfo4_Info_Version)();
					(FARPROC &)fpMediaInfo4_Info_Version = GetProcAddress(m_hLib, "MediaInfo_Info_Version");
					if (fpMediaInfo4_Info_Version)
					{
						char* pszVersion = (*fpMediaInfo4_Info_Version)();
						if (pszVersion && strcmp(pszVersion, "MediaInfoLib - v0.4.0.1 - http://mediainfo.sourceforge.net") == 0)
						{
							(FARPROC &)m_pfnMediaInfo4_Open = GetProcAddress(m_hLib, "MediaInfo_Open");
							(FARPROC &)m_pfnMediaInfo4_Close = GetProcAddress(m_hLib, "MediaInfo_Close");
							(FARPROC &)m_pfnMediaInfo4_Get = GetProcAddress(m_hLib, "MediaInfo_Get");
							(FARPROC &)m_pfnMediaInfo4_Count_Get = GetProcAddress(m_hLib, "MediaInfo_Count_Get");
							if (m_pfnMediaInfo4_Open && m_pfnMediaInfo4_Close && m_pfnMediaInfo4_Get) {
								m_ullVersion = MAKEDLLVERULL(0, 4, 0, 1);
								return TRUE;
							}
							m_pfnMediaInfo4_Open = NULL;
							m_pfnMediaInfo4_Close = NULL;
							m_pfnMediaInfo4_Get = NULL;
							m_pfnMediaInfo4_Count_Get = NULL;
						}
					}
				}
				// Note from MediaInfo developer
				// -----------------------------
				// Note : versioning method, for people who develop with LoadLibrary method
				// - if one of 2 first numbers change, there is no guaranties that the DLL is compatible with old one
				// - if one of 2 last numbers change, there is a garanty that the DLL is compatible with old one.
				// So you should test the version of the DLL, and if one of the 2 first numbers change, not load it.
				// ---
				// eMule currently handles v0.5.1.0, v0.6.0.0, v0.6.1.0
				else if (ullVersion >= MAKEDLLVERULL(0, 5, 0, 0) && ullVersion < MAKEDLLVERULL(0, 7, 0, 0))
				{
					// Don't use 'MediaInfo_Info_Version' with version v0.5+. This function is exported,
					// can be called, but does not return a valid version string..

					(FARPROC &)m_pfnMediaInfo5_Open = GetProcAddress(m_hLib, "MediaInfo_Open");
					(FARPROC &)m_pfnMediaInfo_Close = GetProcAddress(m_hLib, "MediaInfo_Close");
					(FARPROC &)m_pfnMediaInfo_Get = GetProcAddress(m_hLib, "MediaInfo_Get");
					(FARPROC &)m_pfnMediaInfo_Count_Get = GetProcAddress(m_hLib, "MediaInfo_Count_Get");
					if (m_pfnMediaInfo5_Open && m_pfnMediaInfo_Close && m_pfnMediaInfo_Get) {
						m_ullVersion = ullVersion;
						return TRUE;
					}
					m_pfnMediaInfo5_Open = NULL;
					m_pfnMediaInfo_Close = NULL;
					m_pfnMediaInfo_Get = NULL;
					m_pfnMediaInfo_Count_Get = NULL;
				}
				else if (ullVersion >= MAKEDLLVERULL(0, 7, 0, 0) && ullVersion < MAKEDLLVERULL(0, 8, 0, 0))
				{
					(FARPROC &)m_pfnMediaInfo_New = GetProcAddress(m_hLib, "MediaInfo_New");
					(FARPROC &)m_pfnMediaInfo_Delete = GetProcAddress(m_hLib, "MediaInfo_Delete");
					(FARPROC &)m_pfnMediaInfo_Open = GetProcAddress(m_hLib, "MediaInfo_Open");
					(FARPROC &)m_pfnMediaInfo_Close = GetProcAddress(m_hLib, "MediaInfo_Close");
					(FARPROC &)m_pfnMediaInfo_Get = GetProcAddress(m_hLib, "MediaInfo_Get");
					(FARPROC &)m_pfnMediaInfo_Count_Get = GetProcAddress(m_hLib, "MediaInfo_Count_Get");
					if (m_pfnMediaInfo_New && m_pfnMediaInfo_Delete && m_pfnMediaInfo_Open && m_pfnMediaInfo_Close && m_pfnMediaInfo_Get) {
						m_ullVersion = ullVersion;
						return TRUE;
					}
					m_pfnMediaInfo_New = NULL;
					m_pfnMediaInfo_Delete = NULL;
					m_pfnMediaInfo_Open = NULL;
					m_pfnMediaInfo_Close = NULL;
					m_pfnMediaInfo_Get = NULL;
					m_pfnMediaInfo_Count_Get = NULL;
				}
				FreeLibrary(m_hLib);
				m_hLib = NULL;
			}
		}
		return m_hLib != NULL;
	}

	ULONGLONG GetVersion() const
	{
		return m_ullVersion;
	}

	void* Open(LPCTSTR File)
	{
		if (m_pfnMediaInfo4_Open) {
			USES_CONVERSION;
			return (*m_pfnMediaInfo4_Open)(T2A(File));
		}
		else if (m_pfnMediaInfo5_Open)
			return (*m_pfnMediaInfo5_Open)(File);
		else if (m_pfnMediaInfo_New) {
			void* Handle = (*m_pfnMediaInfo_New)();
			if (Handle)
				(*m_pfnMediaInfo_Open)(Handle, File);
			return Handle;
		}
		return NULL;
	}

	void Close(void* Handle)
	{
		if (m_pfnMediaInfo_Delete)
			(*m_pfnMediaInfo_Delete)(Handle);	// File is automaticly closed
		else if (m_pfnMediaInfo4_Close)
			(*m_pfnMediaInfo4_Close)(Handle);
		else if (m_pfnMediaInfo_Close)
			(*m_pfnMediaInfo_Close)(Handle);
	}

	CString Get(void* Handle, stream_t_C StreamKind, int StreamNumber, LPCTSTR Parameter, info_t_C KindOfInfo, info_t_C KindOfSearch)
	{
		if (m_pfnMediaInfo4_Get) {
			USES_CONVERSION;
			return CString((*m_pfnMediaInfo4_Get)(Handle, StreamKind, StreamNumber, T2A(Parameter), KindOfInfo, KindOfSearch));
		}
		else if (m_pfnMediaInfo_Get)
			return (*m_pfnMediaInfo_Get)(Handle, StreamKind, StreamNumber, Parameter, KindOfInfo, KindOfSearch);
		return _T("");
	}

	int Count_Get(void* Handle, stream_t_C StreamKind, int StreamNumber)
	{
		if (m_pfnMediaInfo4_Get)
			return (*m_pfnMediaInfo4_Count_Get)(Handle, StreamKind, StreamNumber);
		else if (m_pfnMediaInfo_Count_Get)
			return (*m_pfnMediaInfo_Count_Get)(Handle, StreamKind, StreamNumber);
		return 0;
	}

protected:
	ULONGLONG m_ullVersion;
	BOOL m_bInitialized;
	HINSTANCE m_hLib;

	// MediaInfoLib - v0.4.0.1
	void* (__stdcall *m_pfnMediaInfo4_Open)(char* File);
	void  (__stdcall *m_pfnMediaInfo4_Close)(void* Handle);
	char* (__stdcall *m_pfnMediaInfo4_Get)(void* Handle, stream_t_C StreamKind, int StreamNumber, char* Parameter, info_t_C KindOfInfo, info_t_C KindOfSearch);
	int   (__stdcall *m_pfnMediaInfo4_Count_Get)(void* Handle, stream_t_C StreamKind, int StreamNumber);

	// MediaInfoLib - v0.5+
	void*			(__stdcall *m_pfnMediaInfo5_Open)(const wchar_t* File);
	void			(__stdcall *m_pfnMediaInfo_Close)(void* Handle);
	const wchar_t*	(__stdcall *m_pfnMediaInfo_Get)(void* Handle, stream_t_C StreamKind, int StreamNumber, const wchar_t* Parameter, info_t_C KindOfInfo, info_t_C KindOfSearch);
	int				(__stdcall *m_pfnMediaInfo_Count_Get)(void* Handle, stream_t_C StreamKind, int StreamNumber);

	// MediaInfoLib - v0.7+
	int				(__stdcall *m_pfnMediaInfo_Open)(void* Handle, const wchar_t* File);
	void*			(__stdcall *m_pfnMediaInfo_New)();
	void			(__stdcall *m_pfnMediaInfo_Delete)(void* Handle);
};

CMediaInfoDLL theMediaInfoDLL;

bool GetMediaInfo(HWND hWndOwner, const CKnownFile* pFile, SMediaInfo* mi, bool bSingleFile);


/////////////////////////////////////////////////////////////////////////////
// CFileInfoDialog dialog

IMPLEMENT_DYNAMIC(CFileInfoDialog, CResizablePage)

BEGIN_MESSAGE_MAP(CFileInfoDialog, CResizablePage)
	ON_MESSAGE(UM_MEDIA_INFO_RESULT, OnMediaInfoResult)
	ON_MESSAGE(UM_DATA_CHANGED, OnDataChanged)
END_MESSAGE_MAP()

CFileInfoDialog::CFileInfoDialog()
	: CResizablePage(CFileInfoDialog::IDD, 0)
{
	m_paFiles = NULL;
	m_bDataChanged = false;
	m_strCaption = GetResString(IDS_FILEINFO);
	m_psp.pszTitle = m_strCaption;
	m_psp.dwFlags |= PSP_USETITLE;
//	memset(&m_cfDef, 0, sizeof m_cfDef);
//	memset(&m_cfBold, 0, sizeof m_cfBold);
//	memset(&m_cfRed, 0, sizeof m_cfRed);
}

CFileInfoDialog::~CFileInfoDialog()
{
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
		{ 0x0130, "SIPROLAB_ACEPLNET", "Sipro Lab Telecom Inc." },
		{ 0x0131, "SIPROLAB_ACELP4800", "Sipro Lab Telecom Inc." },
		{ 0x0132, "SIPROLAB_ACELP8V3", "Sipro Lab Telecom Inc." },
		{ 0x0133, "SIPROLAB_G729", "Sipro Lab Telecom Inc." },
		{ 0x0134, "SIPROLAB_G729A", "Sipro Lab Telecom Inc." },
		{ 0x0135, "SIPROLAB_KELVIN", "Sipro Lab Telecom Inc." },
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
		{ 0x2000, "DVM (AC3-Digital)", "FAST Multimedia AG" }
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

BOOL CFileInfoDialog::OnInitDialog()
{
	CWaitCursor curWait; // we may get quite busy here..
	ReplaceRichEditCtrl(GetDlgItem(IDC_FULL_FILE_INFO), this, GetDlgItem(IDC_FD_XI1)->GetFont());
	CResizablePage::OnInitDialog();
	InitWindowStyles(this);
	AddAnchor(IDC_FULL_FILE_INFO, TOP_LEFT, BOTTOM_RIGHT);

	m_fi.SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
	m_fi.SetAutoURLDetect();
	m_fi.SetEventMask(m_fi.GetEventMask() | ENM_LINK);

	PARAFORMAT pf = {0};
	pf.cbSize = sizeof pf;
	if (m_fi.GetParaFormat(pf)){
		pf.dwMask |= PFM_TABSTOPS;
		pf.cTabCount = 1;
		pf.rgxTabs[0] = 3000;
		m_fi.SetParaFormat(pf);
	}

//	m_cfDef.cbSize = sizeof m_cfDef;
//	if (m_fi.GetSelectionCharFormat(m_cfDef)){
//		m_cfBold = m_cfDef;
//		m_cfBold.dwMask |= CFM_BOLD;
//		m_cfBold.dwEffects |= CFE_BOLD;
//
//		m_cfRed = m_cfDef;
//		m_cfRed.dwMask |= CFM_COLOR;
//		m_cfRed.dwEffects &= ~CFE_AUTOCOLOR;
//		m_cfRed.crTextColor = RGB(255, 0, 0);
//	}

	CResizablePage::UpdateData(FALSE);
	Localize();
	return TRUE;
}

BOOL CFileInfoDialog::OnSetActive()
{
	if (!CResizablePage::OnSetActive())
		return FALSE;
	if (m_bDataChanged)
	{
		CString strWait = GetResString(IDS_FSTAT_WAITING);
		SetDlgItemText(IDC_FORMAT, strWait);
		SetDlgItemText(IDC_FILESIZE, strWait);
		SetDlgItemText(IDC_LENGTH, strWait);
		SetDlgItemText(IDC_VCODEC, strWait);
		SetDlgItemText(IDC_VBITRATE, strWait);
		SetDlgItemText(IDC_VWIDTH, strWait);
		SetDlgItemText(IDC_VHEIGHT, strWait);
		SetDlgItemText(IDC_VASPECT, strWait);
		SetDlgItemText(IDC_VFPS, strWait);
		SetDlgItemText(IDC_ACODEC, strWait);
		SetDlgItemText(IDC_ACHANNEL, strWait);
		SetDlgItemText(IDC_ASAMPLERATE, strWait);
		SetDlgItemText(IDC_ABITRATE, strWait);
		SetDlgItemText(IDC_FULL_FILE_INFO, strWait);

		CGetMediaInfoThread* pThread = (CGetMediaInfoThread*)AfxBeginThread(RUNTIME_CLASS(CGetMediaInfoThread), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		if (pThread)
		{
			pThread->SetValues(m_hWnd, m_paFiles);
			pThread->ResumeThread();
		}
		m_bDataChanged = false;
	}
	return TRUE;
}

LRESULT CFileInfoDialog::OnDataChanged(WPARAM, LPARAM)
{
	m_bDataChanged = true;
	return 1;
}

IMPLEMENT_DYNCREATE(CGetMediaInfoThread, CWinThread)

BOOL CGetMediaInfoThread::InitInstance()
{
	DbgSetThreadName("GetMediaInfo");
	InitThreadLocale();
	return TRUE;
}

int CGetMediaInfoThread::Run()
{
	CoInitialize(NULL);

	CArray<SMediaInfo>* paMediaInfo = new CArray<SMediaInfo>;
	try
	{
		for (int i = 0; i < m_paFiles->GetSize(); i++)
		{
			SMediaInfo mi;
			if (IsWindow(m_hWndOwner) && GetMediaInfo(m_hWndOwner, STATIC_DOWNCAST(CKnownFile, (*m_paFiles)[i]), &mi, m_paFiles->GetSize() == 1))
				paMediaInfo->Add(mi);
			else
			{
				delete paMediaInfo;
				paMediaInfo = NULL;
				break;
			}
		}
	}
	catch(...)
	{
		ASSERT(0);
	}

	if (!IsWindow(m_hWndOwner) || !PostMessage(m_hWndOwner, UM_MEDIA_INFO_RESULT, 0, (LPARAM)paMediaInfo))
		delete paMediaInfo;

	CoUninitialize();
	return 0;
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

LRESULT CFileInfoDialog::OnMediaInfoResult(WPARAM, LPARAM lParam)
{
	SetDlgItemText(IDC_FORMAT, _T("-"));
	SetDlgItemText(IDC_FILESIZE, _T("-"));
	SetDlgItemText(IDC_LENGTH, _T("-"));
	SetDlgItemText(IDC_VCODEC, _T("-"));
	SetDlgItemText(IDC_VBITRATE, _T("-"));
	SetDlgItemText(IDC_VWIDTH, _T("-"));
	SetDlgItemText(IDC_VHEIGHT, _T("-"));
	SetDlgItemText(IDC_VASPECT, _T("-"));
	SetDlgItemText(IDC_VFPS, _T("-"));
	SetDlgItemText(IDC_ACODEC, _T("-"));
	SetDlgItemText(IDC_ACHANNEL, _T("-"));
	SetDlgItemText(IDC_ASAMPLERATE, _T("-"));
	SetDlgItemText(IDC_ABITRATE, _T("-"));
	SetDlgItemText(IDC_FULL_FILE_INFO, _T(""));

	CArray<SMediaInfo>* paMediaInfo = (CArray<SMediaInfo>*)lParam;
	if (paMediaInfo == NULL)
		return 0;

	if (paMediaInfo->GetSize() != m_paFiles->GetSize())
	{
		SetDlgItemText(IDC_FORMAT, _T(""));
		SetDlgItemText(IDC_FILESIZE, _T(""));
		SetDlgItemText(IDC_LENGTH, _T(""));
		SetDlgItemText(IDC_VCODEC, _T(""));
		SetDlgItemText(IDC_VBITRATE, _T(""));
		SetDlgItemText(IDC_VWIDTH, _T(""));
		SetDlgItemText(IDC_VHEIGHT, _T(""));
		SetDlgItemText(IDC_VASPECT, _T(""));
		SetDlgItemText(IDC_VFPS, _T(""));
		SetDlgItemText(IDC_ACODEC, _T(""));
		SetDlgItemText(IDC_ACHANNEL, _T(""));
		SetDlgItemText(IDC_ASAMPLERATE, _T(""));
		SetDlgItemText(IDC_ABITRATE, _T(""));
		SetDlgItemText(IDC_FULL_FILE_INFO, _T(""));
		delete paMediaInfo;
		return 0;
	}

	uint64 uTotalFileSize = 0;
	SMediaInfo ami;
	bool bDiffVideoCompression = false;
	bool bDiffVideoWidth = false;
	bool bDiffVideoHeight = false;
	bool bDiffVideoFrameRate = false;
	bool bDiffVideoBitRate = false;
	bool bDiffVideoAspectRatio = false;
	bool bDiffAudioCompression = false;
	bool bDiffAudioChannels = false;
	bool bDiffAudioSamplesPerSec = false;
	bool bDiffAudioAvgBytesPerSec = false;
	for (int i = 0; i < paMediaInfo->GetSize(); i++)
	{
		const SMediaInfo& mi = paMediaInfo->GetAt(i);

		uTotalFileSize += mi.ulFileSize;
		if (i == 0)
		{
			ami = mi;
		}
		else
		{
			if (ami.strFileFormat != mi.strFileFormat)
				ami.strFileFormat.Empty();

			if (ami.strMimeType != mi.strMimeType)
				ami.strMimeType.Empty();

			ami.fVideoLengthSec += mi.fVideoLengthSec;
			if (ami.iVideoStreams == 0 && mi.iVideoStreams > 0 || ami.iVideoStreams > 0 && mi.iVideoStreams == 0)
			{
				if (ami.iVideoStreams == 0)
					ami.iVideoStreams = mi.iVideoStreams;
				bDiffVideoCompression = true;
				bDiffVideoWidth = true;
				bDiffVideoHeight = true;
				bDiffVideoFrameRate = true;
				bDiffVideoBitRate = true;
				bDiffVideoAspectRatio = true;
			}
			else
			{
				if (ami.strVideoFormat != mi.strVideoFormat)
					bDiffVideoCompression = true;
				if (ami.video.bmiHeader.biWidth != mi.video.bmiHeader.biWidth)
					bDiffVideoWidth = true;
				if (ami.video.bmiHeader.biHeight != mi.video.bmiHeader.biHeight)
					bDiffVideoHeight = true;
				if (ami.fVideoFrameRate != mi.fVideoFrameRate)
					bDiffVideoFrameRate = true;
				if (ami.video.dwBitRate != mi.video.dwBitRate)
					bDiffVideoBitRate = true;
				if (ami.fVideoAspectRatio != mi.fVideoAspectRatio)
					bDiffVideoAspectRatio = true;
			}

			ami.fAudioLengthSec += mi.fAudioLengthSec;
			if (ami.iAudioStreams == 0 && mi.iAudioStreams > 0 || ami.iAudioStreams > 0 && mi.iAudioStreams == 0)
			{
				if (ami.iAudioStreams == 0)
					ami.iAudioStreams = mi.iAudioStreams;
				bDiffAudioCompression = true;
				bDiffAudioChannels = true;
				bDiffAudioSamplesPerSec = true;
				bDiffAudioAvgBytesPerSec = true;
			}
			else
			{
				if (ami.strAudioFormat != mi.strAudioFormat)
					bDiffAudioCompression = true;
				if (ami.audio.nChannels != mi.audio.nChannels)
					bDiffAudioChannels = true;
				if (ami.audio.nSamplesPerSec != mi.audio.nSamplesPerSec)
					bDiffAudioSamplesPerSec = true;
				if (ami.audio.nAvgBytesPerSec != mi.audio.nAvgBytesPerSec)
					bDiffAudioAvgBytesPerSec = true;
			}

			if (!ami.strInfo.str.IsEmpty())
				ami.strInfo << "\n";
			ami.strInfo << mi.strInfo.str;
		}
	}

	CString buffer;

	buffer = ami.strFileFormat;
	if (!ami.strMimeType.IsEmpty())
	{
		if (!buffer.IsEmpty())
			buffer += _T("; MIME type=");
		buffer.AppendFormat(_T("%s"), ami.strMimeType);
	}
	SetDlgItemText(IDC_FORMAT, buffer);

	if (uTotalFileSize)
		SetDlgItemText(IDC_FILESIZE, CastItoXBytes(uTotalFileSize, false, false));
	if (ami.fVideoLengthSec)
		SetDlgItemText(IDC_LENGTH, CastSecondsToHM((time_t)ami.fVideoLengthSec));
	else if (ami.fAudioLengthSec)
		SetDlgItemText(IDC_LENGTH, CastSecondsToHM((time_t)ami.fAudioLengthSec));

	if (ami.iVideoStreams)
	{
		if (!bDiffVideoCompression && !ami.strVideoFormat.IsEmpty())
			SetDlgItemText(IDC_VCODEC, ami.strVideoFormat);
		else
			SetDlgItemText(IDC_VCODEC, _T(""));

		if (!bDiffVideoBitRate && ami.video.dwBitRate)
		{
			if (ami.video.dwBitRate == (DWORD)-1)
				buffer = _T("Variable");
			else
				buffer.Format(_T("%u %s"), (ami.video.dwBitRate + 500) / 1000, GetResString(IDS_KBITSSEC));
			SetDlgItemText(IDC_VBITRATE, buffer);
		}
		else
			SetDlgItemText(IDC_VBITRATE, _T(""));

		if (!bDiffVideoWidth && ami.video.bmiHeader.biWidth)
			SetDlgItemInt(IDC_VWIDTH, abs(ami.video.bmiHeader.biWidth), FALSE);
		else
			SetDlgItemText(IDC_VWIDTH, _T(""));
		
		if (!bDiffVideoHeight && ami.video.bmiHeader.biHeight)
			SetDlgItemInt(IDC_VHEIGHT, abs(ami.video.bmiHeader.biHeight), FALSE);
		else
			SetDlgItemText(IDC_VHEIGHT, _T(""));

		if (!bDiffVideoAspectRatio && ami.fVideoAspectRatio)
		{
			buffer.Format(_T("%.3f"), ami.fVideoAspectRatio);
			CString strAR = GetKnownAspectRatioDisplayString((float)ami.fVideoAspectRatio);
			if (!strAR.IsEmpty())
				buffer.AppendFormat(_T("  (%s)"), strAR);
			SetDlgItemText(IDC_VASPECT, buffer);
		}
		else
			SetDlgItemText(IDC_VASPECT, _T(""));

		if (!bDiffVideoFrameRate && ami.fVideoFrameRate)
		{
			buffer.Format(_T("%.2f"), ami.fVideoFrameRate);
			SetDlgItemText(IDC_VFPS, buffer);
		}
		else
			SetDlgItemText(IDC_VFPS, _T(""));
	}

	if (ami.iAudioStreams)
	{
		if (!bDiffAudioCompression && !ami.strAudioFormat.IsEmpty())
			SetDlgItemText(IDC_ACODEC, ami.strAudioFormat);
		else
			SetDlgItemText(IDC_ACODEC, _T(""));

		if (!bDiffAudioChannels && ami.audio.nChannels)
		{
			switch (ami.audio.nChannels)
			{
				case 1:
					SetDlgItemText(IDC_ACHANNEL, _T("1 (Mono)"));
					break;
				case 2:
					SetDlgItemText(IDC_ACHANNEL, _T("2 (Stereo)"));
					break;
				case 5:
					SetDlgItemText(IDC_ACHANNEL, _T("5.1 (Surround)"));
					break;
				default:
					SetDlgItemInt(IDC_ACHANNEL, ami.audio.nChannels, FALSE);
					break;
			}
		}
		else
			SetDlgItemText(IDC_ACHANNEL, _T(""));

		if (!bDiffAudioSamplesPerSec && ami.audio.nSamplesPerSec)
		{
			buffer.Format(_T("%.3f kHz"), ami.audio.nSamplesPerSec / 1000.0);
			SetDlgItemText(IDC_ASAMPLERATE, buffer);
		}
		else
			SetDlgItemText(IDC_ASAMPLERATE, _T(""));

		if (!bDiffAudioAvgBytesPerSec && ami.audio.nAvgBytesPerSec)
		{
			buffer.Format(_T("%u %s"), (UINT)((ami.audio.nAvgBytesPerSec * 8) / 1000.0 + 0.5), GetResString(IDS_KBITSSEC));
			SetDlgItemText(IDC_ABITRATE, buffer);
		}
		else
			SetDlgItemText(IDC_ABITRATE, _T(""));
	}

	SetDlgItemText(IDC_FULL_FILE_INFO, ami.strInfo.str);
	m_fi.SetSel(0, 0);

	delete paMediaInfo;
	return 0;
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
	if (read(fd, pfccType, sizeof(*pfccType)) != sizeof(*pfccType))
		return FALSE;
	if (read(fd, pdwLength, sizeof(*pdwLength)) != sizeof(*pdwLength))
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
				if (read(hAviFile, &pStrmHdr->hdr, dwLength) != (int)dwLength)
					return FALSE;
				if (dwLength & 1) {
					if (lseek(hAviFile, 1, SEEK_CUR) == -1)
						return FALSE;
				}
			}
			else
			{
				if (read(hAviFile, &pStrmHdr->hdr, sizeof(pStrmHdr->hdr)) != sizeof(pStrmHdr->hdr))
					return FALSE;
				if (lseek(hAviFile, dwLength + (dwLength & 1) - sizeof(pStrmHdr->hdr), SEEK_CUR) == -1)
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
			if (read(hAviFile, pStrmHdr->fmt.dat, dwLength) != (int)dwLength)
				return FALSE;
			if (dwLength & 1)
				if (lseek(hAviFile, 1, SEEK_CUR) == -1)
					return FALSE;
			dwLength = 0;
			break;

		case ckidSTREAMNAME:
			if (dwLength > 512) // expect corrupt data
				return FALSE;
			if ((pStrmHdr->nam = new char[dwLength + 1]) == NULL) {
				errno = ENOMEM;
				return FALSE;
			}
			if (read(hAviFile, pStrmHdr->nam, dwLength) != (int)dwLength)
				return FALSE;
			pStrmHdr->nam[dwLength] = '\0';
			if (dwLength & 1)
				if (lseek(hAviFile, 1, SEEK_CUR) == -1)
					return FALSE;
			dwLength = 0;
			break;
		}

		if (dwLength) {
			if (lseek(hAviFile, dwLength + (dwLength & 1), SEEK_CUR) == -1)
				return FALSE;
		}
	}

	if (dwLengthLeft) {
		if (lseek(hAviFile, dwLengthLeft, SEEK_CUR) == -1)
			return FALSE;
	}

	return TRUE;
}

static BOOL GetRIFFHeaders(LPCTSTR pszFileName, SMediaInfo* mi, bool& rbIsAVI)
{
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
	if (read(hAviFile, &fccMain, sizeof(fccMain)) != sizeof(fccMain))
		goto cleanup;
	if (fccMain == formtypeAVI)
		rbIsAVI = true;
	if (fccMain != formtypeAVI && fccMain != mmioFOURCC('W', 'A', 'V', 'E'))
		goto cleanup;

	BOOL bReadAllStreams;
	bReadAllStreams = FALSE;
	while (!bReadAllStreams && dwLengthLeft >= sizeof(DWORD)*2)
	{
		if (!ReadChunkHeader(hAviFile, &fccType, &dwLength))
			goto inv_format_errno;

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
			if (read(hAviFile, &fccType, sizeof(fccType)) != sizeof(fccType))
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
							if (strmhdr.dwFormatLen && strmhdr.fmt.wav)
							{
								*(PCMWAVEFORMAT*)&mi->audio = *strmhdr.fmt.wav;
								mi->strAudioFormat = GetWaveFormatTagName(mi->audio.wFormatTag);
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
							if (strmhdr.dwFormatLen && strmhdr.fmt.bmi)
							{
								mi->video.bmiHeader = *strmhdr.fmt.bmi;
								mi->strVideoFormat = GetVideoFormatName(mi->video.bmiHeader.biCompression);
								if (mi->video.bmiHeader.biWidth && mi->video.bmiHeader.biHeight)
									mi->fVideoAspectRatio = (float)abs(mi->video.bmiHeader.biWidth) / (float)abs(mi->video.bmiHeader.biHeight);
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
				bReadAllStreams = TRUE;
				break;
			}
			break;

		case ckidAVIMAINHDR:
			if (dwLength == sizeof(MainAVIHeader))
			{
				MainAVIHeader avihdr;
				if (read(hAviFile, &avihdr, sizeof(avihdr)) != sizeof(avihdr))
					goto inv_format_errno;
				if (dwLength & 1)
					if (lseek(hAviFile, 1, SEEK_CUR) == -1)
						goto inv_format_errno;
				dwLength = 0;
			}
			break;

		case ckidAVINEWINDEX:	// idx1
			bReadAllStreams = TRUE;
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
				if (read(hAviFile, strmhdr.fmt.dat, dwLength) != (int)dwLength)
					goto inv_format_errno;
				if (dwLength & 1)
					if (lseek(hAviFile, 1, SEEK_CUR) == -1)
						goto inv_format_errno;
				dwLength = 0;

				strmhdr.hdr.fccType = streamtypeAUDIO;
				if (strmhdr.dwFormatLen)
				{
					mi->iAudioStreams++;
					if (mi->iAudioStreams == 1)
					{
						if (strmhdr.dwFormatLen && strmhdr.fmt.wav)
						{
							*(PCMWAVEFORMAT*)&mi->audio = *strmhdr.fmt.wav;
							mi->strAudioFormat = GetWaveFormatTagName(mi->audio.wFormatTag);
						}
					}
				}
				delete[] strmhdr.fmt.dat;
				delete[] strmhdr.nam;
				iStream++;
				bReadAllStreams = TRUE;
			}
			break;
		}

		if (bReadAllStreams)
			break;
		if (dwLength)
		{
			if (lseek(hAviFile, dwLength + (dwLength & 1), SEEK_CUR) == -1)
				goto inv_format_errno;
		}
	}

	if (fccMain == formtypeAVI)
	{
		mi->strFileFormat = _T("AVI");

		if (mi->fVideoLengthSec)
		{
			DWORD dwVideoFramesOverhead = uVideoFrames * (sizeof(WORD) + sizeof(WORD) + sizeof(DWORD));
			mi->video.dwBitRate = (DWORD)(((dwMovieChunkSize - dwVideoFramesOverhead) / mi->fVideoLengthSec - mi->audio.nAvgBytesPerSec) * 8);
		}
	}
	else if (fccMain == mmioFOURCC('W', 'A', 'V', 'E'))
		mi->strFileFormat = _T("WAV (RIFF)");
	else
		mi->strFileFormat = _T("RIFF");

	bResult = TRUE;

cleanup:
	close(hAviFile);
	return bResult;

inv_format:
	goto cleanup;

inv_format_errno:
	goto cleanup;
}

void WarnAboutWrongFileExtension(SMediaInfo* mi, LPCTSTR pszFileName, LPCTSTR pszExtensions)
{
	if (!mi->strInfo.str.IsEmpty())
		mi->strInfo.str += _T("\r\n");
	mi->strInfo.str.AppendFormat(
		_T("Warning: The file extension of \"%s\" does not match the file extension\r\n") 
		_T("typically used for that file format. You may want to rename the file to\r\n")
		_T("use one of those file extension \"%s\"\r\n")
		_T("for properly using or viewing that file format.\r\n"), pszFileName, pszExtensions);
}

bool GetMimeType(LPCTSTR pszFilePath, CString& rstrMimeType)
{
	int fd = _topen(pszFilePath, O_RDONLY | O_BINARY);
	if (fd != -1)
	{
		BYTE aucBuff[8192];
		int iRead = read(fd, aucBuff, sizeof aucBuff);
		
		close(fd);
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

bool GetMediaInfo(HWND hWndOwner, const CKnownFile* pFile, SMediaInfo* mi, bool bSingleFile)
{
	if (!pFile)
		return false;
	ASSERT( !pFile->GetFilePath().IsEmpty() );

	if (pFile->IsPartFile()) {
		if (((CPartFile*)pFile)->IsComplete(0, 1024, true))
			GetMimeType(pFile->GetFilePath(), mi->strMimeType);
	}
	else {
		GetMimeType(pFile->GetFilePath(), mi->strMimeType);
	}

	mi->ulFileSize = pFile->GetFileSize();

	bool bFoundHeader = false;
	if (pFile->IsPartFile())
	{
		// Do *not* pass a part file which does not have the beginning of file to the following code.
		//	- The MP3 reading code will skip all 0-bytes from the beginning of the file and may block
		//	  the main thread for a long time.
		//
		//	- The RIFF reading code will not work without the file header.
		//
		//	- Most (if not all) other code will also not work without the beginning of the file available.
		if (!((CPartFile*)pFile)->IsComplete(0, 16*1024, true))
			return bFoundHeader || !mi->strMimeType.IsEmpty();
	}
	else
	{
		if (pFile->GetFileSize() < 16*1024)
			return bFoundHeader || !mi->strMimeType.IsEmpty();
	}

	TCHAR szExt[_MAX_EXT];
	_tsplitpath(pFile->GetFileName(), NULL, NULL, NULL, szExt);
	_tcslwr(szExt);

	////////////////////////////////////////////////////////////////////////////
	// Check for AVI file
	//
	bool bIsAVI = false;
	try
	{
		if (GetRIFFHeaders(pFile->GetFilePath(), mi, bIsAVI))
		{
			if (bIsAVI && _tcscmp(szExt, _T(".avi")) != 0)
				WarnAboutWrongFileExtension(mi, pFile->GetFileName(), _T("avi"));
			return true;
		}
	}
	catch(...)
	{
	}

	if (!IsWindow(hWndOwner))
		return false;

	////////////////////////////////////////////////////////////////////////////
	// Check for MPEG Audio file
	//
	if (_tcscmp(szExt, _T(".mp3"))==0 || _tcscmp(szExt, _T(".mp2"))==0 || _tcscmp(szExt, _T(".mp1"))==0 || _tcscmp(szExt, _T(".mpa"))==0)
	{
		try
		{
			USES_CONVERSION;
			ID3_Tag myTag;
			myTag.Link(T2CA(pFile->GetFilePath()));

			const Mp3_Headerinfo* mp3info;
			mp3info = myTag.GetMp3HeaderInfo();
			if (mp3info)
			{
				mi->strFileFormat = _T("MPEG audio");

				if (!bSingleFile)
				{
					//m_fi.SetSelectionCharFormat(m_cfBold);
					if (!mi->strInfo.str.IsEmpty())
						mi->strInfo << _T("\n\n");
					//m_fi.SetSelectionCharFormat(m_cfBold);
					mi->strInfo << _T("File: ") << pFile->GetFileName() << _T("\n");
					mi->strInfo << _T("MP3 Header Info\n");
					//m_fi.SetSelectionCharFormat(m_cfDef);
				}

				switch (mp3info->version)
				{
				case MPEGVERSION_2_5:
					mi->strAudioFormat = _T("MPEG-2.5,");
					mi->audio.wFormatTag = 0x0055;
					break;
				case MPEGVERSION_2:
					mi->strAudioFormat = _T("MPEG-2,");
					mi->audio.wFormatTag = 0x0055;
					break;
				case MPEGVERSION_1:
					mi->strAudioFormat = _T("MPEG-1,");
					mi->audio.wFormatTag = 0x0055;
					break;
				default:
					break;
				}
				mi->strAudioFormat += _T(" ");

				switch (mp3info->layer)
				{
				case MPEGLAYER_III:
					mi->strAudioFormat += _T("Layer 3");
					break;
				case MPEGLAYER_II:
					mi->strAudioFormat += _T("Layer 2");
					break;
				case MPEGLAYER_I:
					mi->strAudioFormat += _T("Layer 1");
					break;
				default:
					break;
				}
				if (!bSingleFile)
				{
					mi->strInfo << _T("   Version:\t") << mi->strAudioFormat << _T("\n");
					mi->strInfo << _T("   Bitrate:\t") << mp3info->bitrate/1000 << _T(" ") << GetResString(IDS_KBITSSEC) << _T("\n");
					mi->strInfo << _T("   Frequency:\t") << mp3info->frequency/1000 << _T(" kHz\n");
				}

				mi->iAudioStreams++;
				mi->audio.nAvgBytesPerSec = mp3info->bitrate/8;
				mi->audio.nSamplesPerSec = mp3info->frequency;

				if (!bSingleFile)
					mi->strInfo << _T("   Mode:\t");
				switch (mp3info->channelmode){
				case MP3CHANNELMODE_STEREO:
					if (!bSingleFile)
						mi->strInfo << _T("Stereo");
					mi->audio.nChannels = 2;
					break;
				case MP3CHANNELMODE_JOINT_STEREO:
					if (!bSingleFile)
						mi->strInfo << _T("Joint Stereo");
					mi->audio.nChannels = 2;
					break;
				case MP3CHANNELMODE_DUAL_CHANNEL:
					if (!bSingleFile)
						mi->strInfo << _T("Dual Channel");
					mi->audio.nChannels = 2;
					break;
				case MP3CHANNELMODE_SINGLE_CHANNEL:
					if (!bSingleFile)
						mi->strInfo << _T("Mono");
					mi->audio.nChannels = 1;
					break;
				}
				if (!bSingleFile)
					mi->strInfo << _T("\n");

				// length
				if (mp3info->time)
				{
					if (!bSingleFile)
					{
						CString strLength;
						SecToTimeLength(mp3info->time, strLength);
						mi->strInfo << _T("   Length:\t") << strLength;
						if (pFile->IsPartFile()){
//							mi->strInfo.SetSelectionCharFormat(m_cfRed);
							mi->strInfo << _T(" (This may not reflect the final total length!)");
//							mi->strInfo.SetSelectionCharFormat(m_cfDef);
						}
						mi->strInfo << "\n";
					}
					mi->fAudioLengthSec = mp3info->time;
				}

				bFoundHeader = true;
				if (mi->strMimeType.IsEmpty())
					mi->strMimeType = _T("audio/mpeg");
			}

			int iTag = 0;
			ID3_Tag::Iterator* iter = myTag.CreateIterator();
			const ID3_Frame* frame;
			while ((frame = iter->GetNext()) != NULL)
			{
				if (iTag == 0)
				{
					if (mp3info && !bSingleFile)
						mi->strInfo << _T("\n");
//					mi->strInfo.SetSelectionCharFormat(m_cfBold);
					mi->strInfo << _T("MP3 Tags\n");
//					mi->strInfo.SetSelectionCharFormat(m_cfDef);
				}
				iTag++;

				LPCSTR desc = frame->GetDescription();
				if (!desc)
					desc = frame->GetTextID();

				CStringStream strFidInfo;
				ID3_FrameID eFrameID = frame->GetID();
				switch (eFrameID)
				{
				case ID3FID_ALBUM:
				case ID3FID_COMPOSER:
				case ID3FID_CONTENTTYPE:
				case ID3FID_COPYRIGHT:
				case ID3FID_DATE:
				case ID3FID_PLAYLISTDELAY:
				case ID3FID_ENCODEDBY:
				case ID3FID_LYRICIST:
				case ID3FID_FILETYPE:
				case ID3FID_TIME:
				case ID3FID_CONTENTGROUP:
				case ID3FID_TITLE:
				case ID3FID_SUBTITLE:
				case ID3FID_INITIALKEY:
				case ID3FID_LANGUAGE:
				case ID3FID_MEDIATYPE:
				case ID3FID_ORIGALBUM:
				case ID3FID_ORIGFILENAME:
				case ID3FID_ORIGLYRICIST:
				case ID3FID_ORIGARTIST:
				case ID3FID_ORIGYEAR:
				case ID3FID_FILEOWNER:
				case ID3FID_LEADARTIST:
				case ID3FID_BAND:
				case ID3FID_CONDUCTOR:
				case ID3FID_MIXARTIST:
				case ID3FID_PARTINSET:
				case ID3FID_PUBLISHER:
				case ID3FID_TRACKNUM:
				case ID3FID_RECORDINGDATES:
				case ID3FID_NETRADIOSTATION:
				case ID3FID_NETRADIOOWNER:
				case ID3FID_SIZE:
				case ID3FID_ISRC:
				case ID3FID_ENCODERSETTINGS:
				case ID3FID_YEAR:
				{
					char *sText = ID3_GetString(frame, ID3FN_TEXT);
					CString strText(sText);
					strText.Trim();
					strFidInfo << strText;
					delete [] sText;
					break;
				}
				case ID3FID_BPM:
				{
					char *sText = ID3_GetString(frame, ID3FN_TEXT);
					long lLength = atol(sText);
					if (lLength) // check for != "0"
						strFidInfo << sText;
					delete [] sText;
					break;
				}
				case ID3FID_SONGLEN:
				{
					char *sText = ID3_GetString(frame, ID3FN_TEXT);
					long lLength = atol(sText) / 1000;
					if (lLength)
					{
						CString strLength;
						SecToTimeLength(lLength, strLength);
						strFidInfo << strLength;
					}
					delete [] sText;
					break;
				}
				case ID3FID_USERTEXT:
				{
					char
					*sText = ID3_GetString(frame, ID3FN_TEXT),
					*sDesc = ID3_GetString(frame, ID3FN_DESCRIPTION);
					CString strText(sText);
					strText.Trim();
					if (!strText.IsEmpty())
					{
						CString strDesc(sDesc);
						strDesc.Trim();
						if (!strDesc.IsEmpty())
							strFidInfo << _T("(") << strDesc << _T(")");

						if (!strDesc.IsEmpty())
							strFidInfo << _T(": ");
						strFidInfo << strText;
					}
					delete [] sText;
					delete [] sDesc;
					break;
				}
				case ID3FID_COMMENT:
				case ID3FID_UNSYNCEDLYRICS:
				{
					char
					*sText = ID3_GetString(frame, ID3FN_TEXT),
					*sDesc = ID3_GetString(frame, ID3FN_DESCRIPTION),
					*sLang = ID3_GetString(frame, ID3FN_LANGUAGE);
					CString strText(sText);
					strText.Trim();
					if (!strText.IsEmpty())
					{
						CString strDesc(sDesc);
						strDesc.Trim();
						if (strDesc == _T("ID3v1 Comment"))
							strDesc.Empty();
						if (!strDesc.IsEmpty())
							strFidInfo << _T("(") << strDesc << _T(")");

						CString strLang(sLang);
						strLang.Trim();
						if (!strLang.IsEmpty())
							strFidInfo << _T("[") << strLang << _T("]");

						if (!strDesc.IsEmpty() || !strLang.IsEmpty())
							strFidInfo << _T(": ");
						strFidInfo << strText;
					}
					delete [] sText;
					delete [] sDesc;
					delete [] sLang;
					break;
				}
				case ID3FID_WWWAUDIOFILE:
				case ID3FID_WWWARTIST:
				case ID3FID_WWWAUDIOSOURCE:
				case ID3FID_WWWCOMMERCIALINFO:
				case ID3FID_WWWCOPYRIGHT:
				case ID3FID_WWWPUBLISHER:
				case ID3FID_WWWPAYMENT:
				case ID3FID_WWWRADIOPAGE:
				{
					char *sURL = ID3_GetString(frame, ID3FN_URL);
					CString strURL(sURL);
					strURL.Trim();
					strFidInfo << strURL;
					delete [] sURL;
					break;
				}
				case ID3FID_WWWUSER:
				{
					char
					*sURL = ID3_GetString(frame, ID3FN_URL),
					*sDesc = ID3_GetString(frame, ID3FN_DESCRIPTION);
					CString strURL(sURL);
					strURL.Trim();
					if (!strURL.IsEmpty())
					{
						CString strDesc(sDesc);
						strDesc.Trim();
						if (!strDesc.IsEmpty())
							strFidInfo << _T("(") << strDesc << _T(")");

						if (!strDesc.IsEmpty())
							strFidInfo << _T(": ");
						strFidInfo << strURL;
					}
					delete [] sURL;
					delete [] sDesc;
					break;
				}
				case ID3FID_INVOLVEDPEOPLE:
				{
					size_t nItems = frame->GetField(ID3FN_TEXT)->GetNumTextItems();
					for (size_t nIndex = 0; nIndex < nItems; nIndex++)
					{
						char *sPeople = ID3_GetString(frame, ID3FN_TEXT, nIndex);
						strFidInfo << sPeople;
						delete [] sPeople;
						if (nIndex + 1 < nItems)
							strFidInfo << _T(", ");
					}
					break;
				}
				case ID3FID_PICTURE:
				{
					char
					*sMimeType = ID3_GetString(frame, ID3FN_MIMETYPE),
					*sDesc	   = ID3_GetString(frame, ID3FN_DESCRIPTION),
					*sFormat   = ID3_GetString(frame, ID3FN_IMAGEFORMAT);
					size_t
					nPicType   = frame->GetField(ID3FN_PICTURETYPE)->Get(),
					nDataSize  = frame->GetField(ID3FN_DATA)->Size();
					strFidInfo << _T("(") << sDesc << _T(")[") << sFormat << _T(", ")
							   << nPicType << _T("]: ") << sMimeType << _T(", ") << nDataSize << _T(" bytes");
					delete [] sMimeType;
					delete [] sDesc;
					delete [] sFormat;
					break;
				}
				case ID3FID_GENERALOBJECT:
				{
					char
					*sMimeType = ID3_GetString(frame, ID3FN_MIMETYPE),
					*sDesc = ID3_GetString(frame, ID3FN_DESCRIPTION),
					*sFileName = ID3_GetString(frame, ID3FN_FILENAME);
					size_t
					nDataSize = frame->GetField(ID3FN_DATA)->Size();
					strFidInfo << _T("(") << sDesc << _T(")[")
						<< sFileName << _T("]: ") << sMimeType << _T(", ") << nDataSize << _T(" bytes");
					delete [] sMimeType;
					delete [] sDesc;
					delete [] sFileName;
					break;
				}
				case ID3FID_UNIQUEFILEID:
				{
					char *sOwner = ID3_GetString(frame, ID3FN_OWNER);
					size_t nDataSize = frame->GetField(ID3FN_DATA)->Size();
					strFidInfo << sOwner << _T(", ") << nDataSize << _T(" bytes");
					delete [] sOwner;
					break;
				}
				case ID3FID_PLAYCOUNTER:
				{
					size_t nCounter = frame->GetField(ID3FN_COUNTER)->Get();
					strFidInfo << nCounter;
					break;
				}
				case ID3FID_POPULARIMETER:
				{
					char *sEmail = ID3_GetString(frame, ID3FN_EMAIL);
					size_t
					nCounter = frame->GetField(ID3FN_COUNTER)->Get(),
					nRating = frame->GetField(ID3FN_RATING)->Get();
					strFidInfo << sEmail << _T(", counter=") << nCounter << _T(" rating=") << nRating;
					delete [] sEmail;
					break;
				}
				case ID3FID_CRYPTOREG:
				case ID3FID_GROUPINGREG:
				{
					char *sOwner = ID3_GetString(frame, ID3FN_OWNER);
					size_t
					nSymbol = frame->GetField(ID3FN_ID)->Get(),
					nDataSize = frame->GetField(ID3FN_DATA)->Size();
					strFidInfo << _T("(") << nSymbol << _T("): ") << sOwner << _T(", ") << nDataSize << _T(" bytes");
					break;
				}
				case ID3FID_SYNCEDLYRICS:
				{
					char
					*sDesc = ID3_GetString(frame, ID3FN_DESCRIPTION),
					*sLang = ID3_GetString(frame, ID3FN_LANGUAGE);
					size_t
					//nTimestamp = frame->GetField(ID3FN_TIMESTAMPFORMAT)->Get(),
					nRating = frame->GetField(ID3FN_CONTENTTYPE)->Get();
					//const char* format = (2 == nTimestamp) ? "ms" : "frames";
					strFidInfo << _T("(") << sDesc << _T(")[") << sLang << _T("]: ");
					switch (nRating)
					{
					case ID3CT_OTHER:    strFidInfo << _T("Other"); break;
					case ID3CT_LYRICS:   strFidInfo << _T("Lyrics"); break;
					case ID3CT_TEXTTRANSCRIPTION:     strFidInfo << _T("Text transcription"); break;
					case ID3CT_MOVEMENT: strFidInfo << _T("Movement/part name"); break;
					case ID3CT_EVENTS:   strFidInfo << _T("Events"); break;
					case ID3CT_CHORD:    strFidInfo << _T("Chord"); break;
					case ID3CT_TRIVIA:   strFidInfo << _T("Trivia/'pop up' information"); break;
					}
					/*ID3_Field* fld = frame->GetField(ID3FN_DATA);
					if (fld)
					{
						ID3_MemoryReader mr(fld->GetRawBinary(), fld->BinSize());
						while (!mr.atEnd())
						{
							strFidInfo << io::readString(mr).c_str();
							strFidInfo << " [" << io::readBENumber(mr, sizeof(uint32)) << " "
								<< format << "] ";
						}
					}*/
					delete [] sDesc;
					delete [] sLang;
					break;
				}
				case ID3FID_AUDIOCRYPTO:
				case ID3FID_EQUALIZATION:
				case ID3FID_EVENTTIMING:
				case ID3FID_CDID:
				case ID3FID_MPEGLOOKUP:
				case ID3FID_OWNERSHIP:
				case ID3FID_PRIVATE:
				case ID3FID_POSITIONSYNC:
				case ID3FID_BUFFERSIZE:
				case ID3FID_VOLUMEADJ:
				case ID3FID_REVERB:
				case ID3FID_SYNCEDTEMPO:
				case ID3FID_METACRYPTO:
					//strFidInfo << _T(" (unimplemented)");
					break;
				default:
					//strFidInfo << _T(" frame");
					break;
				}

				if (!strFidInfo.str.IsEmpty())
				{
					mi->strInfo << _T("   ") << A2CT(desc) << _T(":\t") << strFidInfo.str << _T("\n");
				}
			}
			delete iter;
		}
		catch(...)
		{
			ASSERT(0);
		}

		if (bFoundHeader)
			return true;
	}

	if (!IsWindow(hWndOwner))
		return false;

	// starting the MediaDet object takes a noticeable amount of time.. avoid starting that object
	// for files which are not expected to contain any Audio/Video data.
	// note also: MediaDet does not work well for too short files (e.g. 16K)
	//
	// same applies for MediaInfoLib, its even slower than MediaDet -> avoid calling for non AV files.
	//
	// since we have a thread here, this should not be a performance problem any longer.

	// check again for AV type; MediaDet object has trouble with RAR files (?)
	EED2KFileType eFileType = GetED2KFileTypeID(pFile->GetFileName());
	if (thePrefs.GetInspectAllFileTypes() 
		|| (eFileType == ED2KFT_AUDIO || eFileType == ED2KFT_VIDEO))
	{
		/////////////////////////////////////////////////////////////////////////////
		// Try MediaInfo lib
		//
		// Use MediaInfo only for non AVI files.. Reading potentially broken AVI files 
		// with the VfW API (as MediaInfo is doing) is rather dangerous.
		//
		if (!bIsAVI)
		{
			try
			{
				if (theMediaInfoDLL.Initialize())
				{
					void* Handle = theMediaInfoDLL.Open(pFile->GetFilePath());
					if (Handle)
					{
						CString str;

						mi->strFileFormat = theMediaInfoDLL.Get(Handle, Stream_General, 0, _T("Format_String"), Info_Text, Info_Name);

						if (szExt[0] == _T('.') && szExt[1] != _T('\0'))
						{
							str = theMediaInfoDLL.Get(Handle, Stream_General, 0, _T("Format_Extensions"), Info_Text, Info_Name);
							if (!str.IsEmpty())
							{
								// minor bug in MediaInfo lib.. some file extension lists have a ')' character in there..
								str.Replace(_T(")"), _T(""));
								str.Replace(_T("("), _T(""));

								str.MakeLower();
								bool bFoundExt = false;
								int iPos = 0;
								CString strFmtExt(str.Tokenize(_T(" "), iPos));
								while (!strFmtExt.IsEmpty())
								{
									if (_tcscmp(strFmtExt, szExt + 1) == 0)
									{
										bFoundExt = true;
										break;
									}
									strFmtExt = str.Tokenize(_T(" "), iPos);
								}

								if (!bFoundExt)
									WarnAboutWrongFileExtension(mi, pFile->GetFileName(), str);
							}
						}

						str = theMediaInfoDLL.Get(Handle, Stream_General, 0, _T("PlayTime"), Info_Text, Info_Name);
						float fFileLengthSec = _tstoi(str) / 1000.0F;

						str = theMediaInfoDLL.Get(Handle, Stream_General, 0, _T("VideoCount"), Info_Text, Info_Name);
						int iVideoStreams = _tstoi(str);
						if (iVideoStreams > 0)
						{
							mi->iVideoStreams = iVideoStreams;
							mi->fVideoLengthSec = fFileLengthSec;

							CString strCodec = theMediaInfoDLL.Get(Handle, Stream_Video, 0, _T("Codec"), Info_Text, Info_Name);
							mi->strVideoFormat = strCodec;
							if (!strCodec.IsEmpty()) {
								CStringA strCodecA(strCodec);
								if (!strCodecA.IsEmpty())
									mi->video.bmiHeader.biCompression = *(LPDWORD)(LPCSTR)strCodecA;
							}
							str = theMediaInfoDLL.Get(Handle, Stream_Video, 0, _T("Codec_String"), Info_Text, Info_Name);
							if (!str.IsEmpty())
								mi->strVideoFormat += _T(" (") + str + _T(")");

							str = theMediaInfoDLL.Get(Handle, Stream_Video, 0, _T("Width"), Info_Text, Info_Name);
							mi->video.bmiHeader.biWidth = _tstoi(str);

							str = theMediaInfoDLL.Get(Handle, Stream_Video, 0, _T("Height"), Info_Text, Info_Name);
							mi->video.bmiHeader.biHeight = _tstoi(str);

							str = theMediaInfoDLL.Get(Handle, Stream_Video, 0, _T("FrameRate"), Info_Text, Info_Name);
							mi->fVideoFrameRate = _tstof(str);

							str = theMediaInfoDLL.Get(Handle, Stream_Video, 0, _T("BitRate_Mode"), Info_Text, Info_Name);
							if (str.CompareNoCase(_T("VBR")) == 0) {
								mi->video.dwBitRate = (DWORD)-1;
							}
							else {
								str = theMediaInfoDLL.Get(Handle, Stream_Video, 0, _T("BitRate"), Info_Text, Info_Name);
								mi->video.dwBitRate = _tstoi(str);
							}

							str = theMediaInfoDLL.Get(Handle, Stream_Video, 0, _T("AspectRatio"), Info_Text, Info_Name);
							mi->fVideoAspectRatio = _tstof(str);

							bFoundHeader = true;
						}

						str = theMediaInfoDLL.Get(Handle, Stream_General, 0, _T("AudioCount"), Info_Text, Info_Name);
						int iAudioStreams = _tstoi(str);
						if (iAudioStreams > 0)
						{
							mi->iAudioStreams = iAudioStreams;
							mi->fAudioLengthSec = fFileLengthSec;

							str = theMediaInfoDLL.Get(Handle, Stream_Audio, 0, _T("Codec"), Info_Text, Info_Name);
							if (_stscanf(str, _T("%hx"), &mi->audio.wFormatTag) != 1)
							{
								mi->strAudioFormat = str;
								str = theMediaInfoDLL.Get(Handle, Stream_Audio, 0, _T("Codec_String"), Info_Text, Info_Name);
								if (!str.IsEmpty())
									mi->strAudioFormat += _T(" (") + str + _T(")");
							}
							else
							{
								mi->strAudioFormat = theMediaInfoDLL.Get(Handle, Stream_Audio, 0, _T("Codec_String"), Info_Text, Info_Name);
								str = theMediaInfoDLL.Get(Handle, Stream_Audio, 0, _T("Codec_Info"), Info_Text, Info_Name);
								if (!str.IsEmpty())
									mi->strAudioFormat += _T(" (") + str + _T(")");
							}

							str = theMediaInfoDLL.Get(Handle, Stream_Audio, 0, _T("Channels"), Info_Text, Info_Name);
							mi->audio.nChannels = _tstoi(str);

							str = theMediaInfoDLL.Get(Handle, Stream_Audio, 0, _T("SamplingRate"), Info_Text, Info_Name);
							mi->audio.nSamplesPerSec = _tstoi(str);

							str = theMediaInfoDLL.Get(Handle, Stream_Audio, 0, _T("BitRate"), Info_Text, Info_Name);
							mi->audio.nAvgBytesPerSec = _tstoi(str) / 8;

							bFoundHeader = true;
						}

						theMediaInfoDLL.Close(Handle);

						if (bFoundHeader)
							return true;
					}
				}
			}
			catch(...)
			{
				ASSERT(0);
			}
		}

		if (!IsWindow(hWndOwner))
			return false;

		/////////////////////////////////////////////////////////////////////////////
		// Try MediaDet object
		//
		// Avoid processing of some file types which are known to crash due to bugged DirectShow filters.
		if (thePrefs.GetInspectAllFileTypes() 
			|| (_tcscmp(szExt, _T(".ogm"))!=0 && _tcscmp(szExt, _T(".ogg"))!=0 && _tcscmp(szExt, _T(".mkv"))!=0))
		{
			try
			{
				CComPtr<IMediaDet> pMediaDet;
				HRESULT hr = pMediaDet.CoCreateInstance(__uuidof(MediaDet));
				if (SUCCEEDED(hr))
				{
					USES_CONVERSION;
					if (SUCCEEDED(hr = pMediaDet->put_Filename(CComBSTR(T2CW(pFile->GetFilePath())))))
					{
						long lStreams;
						if (SUCCEEDED(hr = pMediaDet->get_OutputStreams(&lStreams)))
						{
							for (long i = 0; i < lStreams; i++)
							{
								if (SUCCEEDED(hr = pMediaDet->put_CurrentStream(i)))
								{
									GUID major_type;
									if (SUCCEEDED(hr = pMediaDet->get_StreamType(&major_type)))
									{
										if (major_type == MEDIATYPE_Video)
										{
											mi->iVideoStreams++;

											if (mi->iVideoStreams > 1)
											{
												if (!bSingleFile)
												{
													if (!mi->strInfo.str.IsEmpty())
														mi->strInfo << _T("\n\n");
													mi->strInfo << _T("File: ") << pFile->GetFileName() << _T("\n");
												}
												mi->strInfo << _T("Additional Video Stream\n");
											}

											AM_MEDIA_TYPE mt = {0};
											if (SUCCEEDED(hr = pMediaDet->get_StreamMediaType(&mt)))
											{
												if (mt.formattype == FORMAT_VideoInfo)
												{
													VIDEOINFOHEADER* pVIH = (VIDEOINFOHEADER*)mt.pbFormat;

													if (mi->iVideoStreams == 1)
													{
														mi->video = *pVIH;
														if (mi->video.bmiHeader.biWidth && mi->video.bmiHeader.biHeight)
															mi->fVideoAspectRatio = (float)abs(mi->video.bmiHeader.biWidth) / (float)abs(mi->video.bmiHeader.biHeight);
														mi->video.dwBitRate = 0; // don't use this value
														mi->strVideoFormat = GetVideoFormatName(mi->video.bmiHeader.biCompression);
														pMediaDet->get_FrameRate(&mi->fVideoFrameRate);
														bFoundHeader = true;
													}
													else
													{
														mi->strInfo << _T("   Codec:\t") << (LPCTSTR)GetVideoFormatName(pVIH->bmiHeader.biCompression) << _T("\n");
														mi->strInfo << _T("   Width x Height:\t") << abs(pVIH->bmiHeader.biWidth) << _T(" x ") << abs(pVIH->bmiHeader.biHeight) << _T("\n");
														// do not use that 'dwBitRate', whatever this number is, it's not
														// the bitrate of the encoded video stream. seems to be the bitrate
														// of the uncompressed stream divided by 2 !??
														//if (pVIH->dwBitRate)
														//	mi->strInfo << "   Bitrate:\t" << (UINT)(pVIH->dwBitRate / 1000) << " " << GetResString(IDS_KBITSSEC) << "\n";

														double fFrameRate = 0.0;
														if (SUCCEEDED(pMediaDet->get_FrameRate(&fFrameRate)) && fFrameRate)
															mi->strInfo << _T("   Frames/s:\t") << fFrameRate << _T("\n");
													}
												}
											}

											double fLength = 0.0;
											if (SUCCEEDED(pMediaDet->get_StreamLength(&fLength)) && fLength)
											{
												if (mi->iVideoStreams == 1)
													mi->fVideoLengthSec = fLength;
												else
												{
													CString strLength;
													SecToTimeLength((ULONG)fLength, strLength);
													mi->strInfo << _T("   Length:\t") << strLength;
													if (pFile->IsPartFile()){
														mi->strInfo << _T(" (This may not reflect the final total length!)");
													}
													mi->strInfo << _T("\n");
												}
											}

											if (mt.pUnk != NULL)
												mt.pUnk->Release();
											if (mt.pbFormat != NULL)
												CoTaskMemFree(mt.pbFormat);
											if (mi->iVideoStreams > 1)
												mi->strInfo << _T("\n");
										}
										else if (major_type == MEDIATYPE_Audio)
										{
											mi->iAudioStreams++;

											if (mi->iAudioStreams > 1)
											{
												if (!bSingleFile)
												{
													if (!mi->strInfo.str.IsEmpty())
														mi->strInfo << _T("\n\n");
													mi->strInfo << _T("File: ") << pFile->GetFileName() << _T("\n");
												}
												mi->strInfo << _T("Additional Audio Stream\n");
											}

											AM_MEDIA_TYPE mt = {0};
											if (SUCCEEDED(hr = pMediaDet->get_StreamMediaType(&mt)))
											{
												if (mt.formattype == FORMAT_WaveFormatEx)
												{
													WAVEFORMATEX* wfx = (WAVEFORMATEX*)mt.pbFormat;

													if (mi->iAudioStreams == 1)
													{
														memcpy(&mi->audio, wfx, sizeof mi->audio);
														mi->strAudioFormat = GetWaveFormatTagName(wfx->wFormatTag);
													}
													else
													{
														CString strFormat = GetWaveFormatTagName(wfx->wFormatTag);
														mi->strInfo << _T("   Format:\t") << strFormat << _T("\n");
														if (wfx->nAvgBytesPerSec)
															mi->strInfo << _T("   Bitrate:\t") << (UINT)(((wfx->nAvgBytesPerSec * 8.0) + 500.0) / 1000.0) << _T(" ") << GetResString(IDS_KBITSSEC) << _T("\n");
														if (wfx->nSamplesPerSec)
															mi->strInfo << _T("   Samples/s:\t") << wfx->nSamplesPerSec / 1000.0 << _T(" kHz\n");
														if (wfx->wBitsPerSample)
															mi->strInfo << _T("   Bit/sample:\t") << wfx->wBitsPerSample << _T(" Bit\n");

														mi->strInfo << _T("   Mode:\t");
														if (wfx->nChannels == 1)
															mi->strInfo << _T("Mono");
														else if (wfx->nChannels == 2)
															mi->strInfo << _T("Stereo");
														else
															mi->strInfo << wfx->nChannels << _T(" channels");
														mi->strInfo << _T("\n");
													}
													bFoundHeader = true;
												}
											}

											double fLength = 0.0;
											if (SUCCEEDED(pMediaDet->get_StreamLength(&fLength)) && fLength)
											{
												if (mi->iAudioStreams == 1)
													mi->fAudioLengthSec = fLength;
												else
												{
													CString strLength;
													SecToTimeLength((ULONG)fLength, strLength);
													mi->strInfo << _T("   Length:\t") << strLength;
													if (pFile->IsPartFile()){
														mi->strInfo << _T(" (This may not reflect the final total length!)");
													}
													mi->strInfo << _T("\n");
												}
											}

											if (mt.pUnk != NULL)
												mt.pUnk->Release();
											if (mt.pbFormat != NULL)
												CoTaskMemFree(mt.pbFormat);
											if (mi->iAudioStreams > 1)
												mi->strInfo << _T("\n");
										}
										else{
											TRACE(_T("%s - Unknown stream type\n"), pFile->GetFileName());
										}
									}
								}
							}
						}
					}
					else{
						TRACE(_T("Failed to open \"%s\" - %s\n"), pFile->GetFilePath(), GetErrorMessage(hr, 1));
					}
				}
			}
			catch(...){
				ASSERT(0);
			}
		}
	}

	return bFoundHeader || !mi->strMimeType.IsEmpty();
}

void CFileInfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FULL_FILE_INFO, m_fi);
}

void CFileInfoDialog::Localize()
{
	GetDlgItem(IDC_FD_XI1)->SetWindowText(GetResString(IDS_FD_SIZE));
	GetDlgItem(IDC_FD_XI2)->SetWindowText(GetResString(IDS_LENGTH)+_T(":"));
	GetDlgItem(IDC_FD_XI3)->SetWindowText(GetResString(IDS_VIDEO));
	GetDlgItem(IDC_FD_XI4)->SetWindowText(GetResString(IDS_AUDIO));

	GetDlgItem(IDC_FD_XI5)->SetWindowText( GetResString(IDS_CODEC)+_T(":"));
	GetDlgItem(IDC_FD_XI6)->SetWindowText( GetResString(IDS_CODEC)+_T(":"));

	GetDlgItem(IDC_FD_XI7)->SetWindowText( GetResString(IDS_BITRATE)+_T(":"));
	GetDlgItem(IDC_FD_XI8)->SetWindowText( GetResString(IDS_BITRATE)+_T(":"));

	GetDlgItem(IDC_FD_XI9)->SetWindowText( GetResString(IDS_WIDTH)+_T(":"));
	GetDlgItem(IDC_FD_XI11)->SetWindowText( GetResString(IDS_HEIGHT)+_T(":"));
	GetDlgItem(IDC_FD_XI13)->SetWindowText( GetResString(IDS_FPS)+_T(":"));
	GetDlgItem(IDC_FD_XI10)->SetWindowText( GetResString(IDS_CHANNELS)+_T(":"));
	GetDlgItem(IDC_FD_XI12)->SetWindowText( GetResString(IDS_SAMPLERATE)+_T(":"));

	GetDlgItem(IDC_STATICFI)->SetWindowText( GetResString(IDS_FILEFORMAT)+_T(":"));
	GetDlgItem(IDC_FD_XI14)->SetWindowText( GetResString(IDS_ASPECTRATIO)+_T(":"));
}

void CFileInfoDialog::AddFileInfo(LPCTSTR pszFmt, ...)
{
	va_list pArgp;
	va_start(pArgp, pszFmt);
	CString strInfo;
	strInfo.FormatV(pszFmt, pArgp);
	va_end(pArgp);

	m_fi.SetSel(m_fi.GetWindowTextLength(), m_fi.GetWindowTextLength());
	m_fi.ReplaceSel(strInfo);
}
