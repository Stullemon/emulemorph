//this file is part of eMule
// added by quekky
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

// FileInfoDialog.cpp : implementation file

#include "stdafx.h"
#include "eMule.h"
#include "FileInfoDialog.h"

// id3lib
#define _SIZED_TYPES_H_ // ugly, ugly.. TODO change *our* types.h!!!!
#include <id3/tag.h>
#include <id3/misc_support.h>

// Video for Windows API
// Those defines are for 'mmreg.h' which is included by 'vfw.h'
#define NOMMIDS		 // Multimedia IDs are not defined
#define NONEWWAVE	   // No new waveform types are defined except WAVEFORMATEX
#define NONEWRIFF	 // No new RIFF forms are defined
#define NOJPEGDIB	 // No JPEG DIB definitions
#define NONEWIC		 // No new Image Compressor types are defined
#define NOBITMAP	 // No extended bitmap info header definition
// Those defines are for 'vfw.h'
#define NOCOMPMAN
#define NODRAWDIB
#define NOVIDEO
#define NOAVIFMT
#define NOMMREG
//#define NOAVIFILE
#define NOMCIWND
#define NOAVICAP
#define NOMSACM
#define MMNOMMIO
#include <vfw.h>

// DirectShow MediaDet
#include <strmif.h>
#include <uuids.h>
#include <qedit.h>
//#include <amvideo.h>
typedef struct tagVIDEOINFOHEADER {

    RECT            rcSource;          // The bit we really want to use
    RECT            rcTarget;          // Where the video should go
    DWORD           dwBitRate;         // Approximate bit data rate
    DWORD           dwBitErrorRate;    // Bit error rate for this stream
    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

    BITMAPINFOHEADER bmiHeader;

} VIDEOINFOHEADER;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


extern void SecToTimeLength(unsigned long ulSec, CStringA& rstrTimeLength);
extern int GetStreamFormat(const PAVISTREAM pAviStrm, long &rlStrmFmtSize, LPVOID &rpStrmFmt);


/////////////////////////////////////////////////////////////////////////////
// CRichEditCtrlX

BEGIN_MESSAGE_MAP(CRichEditCtrlX, CRichEditCtrl)
	ON_WM_GETDLGCODE()
	ON_NOTIFY_REFLECT_EX(EN_LINK, OnEnLink)
END_MESSAGE_MAP()

UINT CRichEditCtrlX::OnGetDlgCode() 
{
	// Avoid that the edit control will select the entire contents, if the
	// focus is moved via tab into the edit control
	//
	// DLGC_WANTALLKEYS is needed, if the control is within a wizard property
	// page and the user presses the Enter key to invoke the default button of the property sheet!
	return CRichEditCtrl::OnGetDlgCode() & ~(DLGC_HASSETSEL | DLGC_WANTALLKEYS);
}

BOOL CRichEditCtrlX::OnEnLink(NMHDR *pNMHDR, LRESULT *pResult)
{
	BOOL bMsgHandled = FALSE;
	*pResult = 0;
	ENLINK* pEnLink = reinterpret_cast<ENLINK *>(pNMHDR);
	if (pEnLink && pEnLink->msg == WM_LBUTTONDOWN)
	{
		CString strUrl;
		GetTextRange(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax, strUrl);

		ShellExecute(NULL, NULL, strUrl, NULL, NULL, SW_SHOWDEFAULT);
		*pResult = 1;
		bMsgHandled = TRUE; // do not route this message to any parent
	}
	return bMsgHandled;
}

/////////////////////////////////////////////////////////////////////////////
// CFileInfoDialog dialog

IMPLEMENT_DYNAMIC(CFileInfoDialog, CResizablePage)

BEGIN_MESSAGE_MAP(CFileInfoDialog, CResizablePage)
	ON_BN_CLICKED(IDC_ROUNDBIT, OnBnClickedRoundbit)
END_MESSAGE_MAP()

CFileInfoDialog::CFileInfoDialog()
	: CResizablePage(CFileInfoDialog::IDD, 0)
{
	m_strCaption = GetResString(IDS_FILEINFO);
	m_psp.pszTitle = m_strCaption;
	m_psp.dwFlags |= PSP_USETITLE;
	m_bAudioRoundBitrate = FALSE;
	m_lAudioBitrate = 0;
	memset(&m_cfDef, 0, sizeof m_cfDef);
	memset(&m_cfBold, 0, sizeof m_cfBold);
	memset(&m_cfRed, 0, sizeof m_cfRed);
}

CFileInfoDialog::~CFileInfoDialog()
{
}

BOOL CFileInfoDialog::OnInitDialog()
{
	CResizablePage::OnInitDialog();
	InitWindowStyles(this);
	AddAnchor(IDC_FULL_FILE_INFO, TOP_LEFT, BOTTOM_RIGHT);

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

	m_cfDef.cbSize = sizeof m_cfDef;
	if (m_fi.GetSelectionCharFormat(m_cfDef)){
		m_cfBold = m_cfDef;
		m_cfBold.dwMask |= CFM_BOLD;
		m_cfBold.dwEffects |= CFE_BOLD;

		m_cfRed = m_cfDef;
		m_cfRed.dwMask |= CFM_COLOR;
		m_cfRed.dwEffects &= ~CFE_AUTOCOLOR;
		m_cfRed.crTextColor = RGB(255, 0, 0);
	}

	m_bAudioRoundBitrate = 1;
	CResizablePage::UpdateData(FALSE);
	Localize();
	RefreshData();
	return true;
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
		{ 0x0000, "UNKNOWN", "Microsoft Corporation" }, 
		{ 0x0001, "PCM", "Microsoft Corporation" },
		{ 0x0002, "ADPCM", "Microsoft Corporation" }, 
		{ 0x0003, "IEEE_FLOAT", "Microsoft Corporation" }, 
		{ 0x0004, "VSELP", "Compaq Computer Corp." }, 
		{ 0x0005, "IBM_CVSD", "IBM Corporation" }, 
		{ 0x0006, "ALAW", "Microsoft Corporation" }, 
		{ 0x0007, "MULAW", "Microsoft Corporation" }, 
		{ 0x0008, "DTS", "Microsoft Corporation" }, 
		{ 0x0009, "DRM", "Microsoft Corporation" }, 
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
		{ 0x0031, "GSM610", "Microsoft Corporation" }, 
		{ 0x0032, "MSNAUDIO", "Microsoft Corporation" }, 
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
		{ 0x0042, "MSG723", "Microsoft Corporation" }, 
		{ 0x0050, "MPEG", "Microsoft Corporation" }, 
		{ 0x0052, "RT24", "InSoft, Inc." }, 
		{ 0x0053, "PAC", "InSoft, Inc." }, 
		{ 0x0055, "MPEG Layer 3", "ISO/MPEG-1 Layer 3" }, 
		{ 0x0059, "LUCENT_G723", "Lucent Technologies" }, 
		{ 0x0060, "CIRRUS", "Cirrus Logic" }, 
		{ 0x0061, "ESPCM", "ESS Technology" }, 
		{ 0x0062, "VOXWARE", "Voxware Inc" }, 
		{ 0x0063, "CANOPUS_ATRAC", "Canopus, co., Ltd." }, 
		{ 0x0064, "G726_ADPCM", "APICOM" }, 
		{ 0x0065, "G722_ADPCM", "APICOM" }, 
		{ 0x0067, "DSAT_DISPLAY", "Microsoft Corporation" }, 
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
		{ 0x0082, "MSRT24", "Microsoft Corporation" }, 
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
		{ 0x0160, "MSAUDIO1", "Microsoft Corporation" }, 
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
		{ 0x2000, "DVM", "FAST Multimedia AG" }
	};

	for (int i = 0; i < ARRSIZE(WavFmtTag); i++)
	{
		if (WavFmtTag[i].uFmtTag == uWavFmtTag){
			rstrComment = WavFmtTag[i].pszComment;
			return WavFmtTag[i].pszDefine;
		}
	}

	CString strCompression;
	strCompression.Format(_T("0x%04x (unknown)"), uWavFmtTag);
	return strCompression;
}

void CFileInfoDialog::RefreshData()
{
	if (!m_file)
		return;
	ASSERT( !m_file->GetFilePath().IsEmpty() );

	try{
		CFile file;
		if (file.Open(m_file->GetFilePath(), CFile::typeBinary | CFile::modeRead | CFile::shareDenyNone))
		{
			char stemp[5];
			CString buffer;
			uint32 u32;
			uint16 u16;

			file.Seek(8, CFile::begin);
			file.Read(stemp, 4);
			stemp[4] = 0;
			if(strcmp(stemp, "AVI ") == 0)
			{
				// filesize
				long size;
				file.Seek(4, CFile::begin);
				file.Read(&size, 4);
				buffer.Format("%.2f MB",((float)size / 1024 / 1024));
				GetDlgItem(IDC_FILESIZE)->SetWindowText(buffer);

				// header sizes
				long Aviheadersize,Vheadersize;
				file.Seek(28, CFile::begin);
				file.Read(&Aviheadersize, 4);

				long Aviheaderstart = 32;
				long Vheaderstart = Aviheaderstart + Aviheadersize + 20;

				// misc
				long Microsec;
				file.Seek(Aviheaderstart, CFile::begin);
				file.Read(&Microsec, 4);

				long LengthInFrames;
				file.Seek(Aviheaderstart + 16, CFile::begin);
				file.Read(&LengthInFrames, 4);

				// fps
				double fps = (double)1000000 / (double)Microsec;
				buffer.Format("%.2f", fps);
				GetDlgItem(IDC_VFPS)->SetWindowText(buffer);
				
				// length
				long LengthInSec = (long)(LengthInFrames / fps);
				GetDlgItem(IDC_LENGTH)->SetWindowText(CastSecondsToHM(LengthInSec));

				// video width
				file.Seek(Aviheaderstart + 32, CFile::begin);
				file.Read(&u32, 4);
				buffer.Format("%d", u32);
				GetDlgItem(IDC_VWIDTH)->SetWindowText(buffer);

				// video height
				file.Seek(Aviheaderstart + 36, CFile::begin);
				file.Read(&u32, 4);
				buffer.Format("%d", u32);
				GetDlgItem(IDC_VHEIGHT)->SetWindowText(buffer);


				// video codec
				file.Seek(Vheaderstart + 4, CFile::begin);
				file.Read(stemp, 4);
				stemp[4] = 0;
				if(!stricmp(stemp, "div3"))
					GetDlgItem(IDC_VCODEC)->SetWindowText("DivX ;-) MPEG-4 v3");
				else if(!stricmp(stemp, "div4"))
					GetDlgItem(IDC_VCODEC)->SetWindowText("DivX ;-) MPEG-4 v4");
				else if(!stricmp(stemp, "divx"))
					GetDlgItem(IDC_VCODEC)->SetWindowText("DivX 4/5");
				else if(!stricmp(stemp, "div2"))
					GetDlgItem(IDC_VCODEC)->SetWindowText("MS MPEG-4 v2");
				else if(!stricmp(stemp, "mp43"))
					GetDlgItem(IDC_VCODEC)->SetWindowText("Microcrap MPEG-4 v3");
				else if(!stricmp(stemp, "mp42"))
					GetDlgItem(IDC_VCODEC)->SetWindowText("Microcrap MPEG-4 v2");
				else
					GetDlgItem(IDC_VCODEC)->SetWindowText(stemp);


				// header sizes
				file.Seek(Aviheaderstart + Aviheadersize + 4, CFile::begin);
				file.Read(&Vheadersize, 4);

				long Aheaderstart = Vheaderstart + Vheadersize + 8;	//first databyte of audio header

				long Astrhsize;
				file.Seek(Aheaderstart - 4, CFile::begin);
				file.Read(&Astrhsize, 4);

				// audio codec
				file.Seek(Aheaderstart + Astrhsize + 8, CFile::begin);
				file.Read(&u16, 2);
				switch(u16)
				{
					case 0:
					case 1:
						GetDlgItem(IDC_ACODEC)->SetWindowText("PCM");
						break;
					case 353:
						GetDlgItem(IDC_ACODEC)->SetWindowText("DivX ;-) Audio");
						break;
					case 85:
						GetDlgItem(IDC_ACODEC)->SetWindowText("MPEG Layer 3");
						break;
					case 8192:
						GetDlgItem(IDC_ACODEC)->SetWindowText("AC3-Digital");
						break;
					default:{
						CString strComment;
						CString strFormat = GetWaveFormatTagName(u16, strComment);
						GetDlgItem(IDC_ACODEC)->SetWindowText(strFormat + _T(" (") + strComment + _T(")"));
						break;
					}
				}
					
					
				// audio channel
				file.Seek(Aheaderstart + 2 + Astrhsize + 8, CFile::begin);
				file.Read(&u16, 2);
				switch(u16)
				{
					case 1:
						GetDlgItem(IDC_ACHANNEL)->SetWindowText("1 (mono)");
						break;
					case 2:
						GetDlgItem(IDC_ACHANNEL)->SetWindowText("2 (stereo)");
						break;
					case 5:
						GetDlgItem(IDC_ACHANNEL)->SetWindowText("5.1 (surround)");
						break;
					default:
						buffer.Format("%d", u16);
						GetDlgItem(IDC_ACHANNEL)->SetWindowText(buffer);
						break;
				}
					
				// audio samplerate
				file.Seek(Aheaderstart + 4 + Astrhsize + 8, CFile::begin);
				file.Read(&u32, 2);
				buffer.Format("%d", u32);
				GetDlgItem(IDC_ASAMPLERATE)->SetWindowText(buffer);

				// audio bitrate
				file.Seek(Aheaderstart + 8 + Astrhsize + 8, CFile::begin);
				file.Read(&m_lAudioBitrate, 4);
				OnBnClickedRoundbit();

				// video bitrate
				buffer.Format("%d Kbit/s", (size / LengthInSec - m_lAudioBitrate) / 128);
				GetDlgItem(IDC_VBITRATE)->SetWindowText(buffer);
			}
		}
	}
	catch(CFileException* error){
		OUTPUT_DEBUG_TRACE();
		error->Delete();	//memleak fix
	}

	if (m_file->IsPartFile())
	{
		// Do *not* pass a part file which does not have the beginning of file to the following code.
		//	- The MP3 reading code will skip all 0-bytes from the beginning of the file and may block
		//	  the main thread for a long time.
		//
		//TODO: To avoid any troubles we need to spawn a thread here..
		if (!((CPartFile*)m_file)->IsComplete(0, 16*1024))
			return;
	}

	TCHAR szExt[_MAX_EXT];
	_tsplitpath(m_file->GetFileName(), NULL, NULL, NULL, szExt);
	_tcslwr(szExt);
	if (_tcscmp(szExt, _T(".mp3"))==0 || _tcscmp(szExt, _T(".mp2"))==0 || _tcscmp(szExt, _T(".mp1"))==0 || _tcscmp(szExt, _T(".mpa"))==0)
	{
		try{
			ID3_Tag myTag;
			myTag.Link(m_file->GetFilePath(), ID3TT_ALL);

			const Mp3_Headerinfo* mp3info;
			mp3info = myTag.GetMp3HeaderInfo();
			if (mp3info)
			{
				m_fi.SetSelectionCharFormat(m_cfBold);
				m_fi << "MP3 Header Info\n";
				m_fi.SetSelectionCharFormat(m_cfDef);

				m_fi << "   Version:\t";
				switch (mp3info->version)
				{
				case MPEGVERSION_2_5:
					m_fi << "MPEG-2.5,";
					break;
				case MPEGVERSION_2:
					m_fi << "MPEG-2,";
					break;
				case MPEGVERSION_1:
					m_fi << "MPEG-1,";
					break;
				default:
					break;
				}
				m_fi << " ";

				switch (mp3info->layer)
				{
				case MPEGLAYER_III:
					m_fi << "Layer 3";
					break;
				case MPEGLAYER_II:
					m_fi << "Layer 2";
					break;
				case MPEGLAYER_I:
					m_fi << "Layer 1";
					break;
				default:
					break;
				}
				m_fi << "\n";
				m_fi << "   Bitrate:\t" << mp3info->bitrate/1000 << " kBit/s\n";
				m_fi << "   Frequency:\t" << mp3info->frequency/1000 << " kHz\n";
			
				m_fi << "   Mode:\t";
				switch (mp3info->channelmode){
				case MP3CHANNELMODE_STEREO:
					m_fi << "Stereo";
					break;
				case MP3CHANNELMODE_JOINT_STEREO:
					m_fi << "Joint Stereo";
					break;
				case MP3CHANNELMODE_DUAL_CHANNEL:
					m_fi << "Dual Channel";
					break;
				case MP3CHANNELMODE_SINGLE_CHANNEL:
					m_fi << "Mono";
					break;
				}
				m_fi << "\n";

				// length
				if (mp3info->time){
					CStringA strLength;
					SecToTimeLength(mp3info->time, strLength);
					m_fi << "   Length:\t" << (LPCSTR)strLength;
					if (m_file->IsPartFile()){
						m_fi.SetSelectionCharFormat(m_cfRed);
						m_fi << " (This may not reflect the final total length!)";
						m_fi.SetSelectionCharFormat(m_cfDef);
					}
					m_fi << "\n";
				}
			}

			int iTag = 0;
			ID3_Tag::Iterator* iter = myTag.CreateIterator();
			const ID3_Frame* frame;
			while ((frame = iter->GetNext()) != NULL)
			{
				if (iTag == 0)
				{
					if (mp3info)
						m_fi << "\n";
					m_fi.SetSelectionCharFormat(m_cfBold);
					m_fi << "MP3 Tags\n";
					m_fi.SetSelectionCharFormat(m_cfDef);
				}
				iTag++;

				LPCSTR desc = frame->GetDescription();
				if (!desc)
					desc = frame->GetTextID();
				m_fi << "   " << desc << ":\t";

				ID3_FrameID eFrameID = frame->GetID();
				switch (eFrameID)
				{
				case ID3FID_ALBUM:
				case ID3FID_BPM:
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
				case ID3FID_SONGLEN:
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
					m_fi << sText << "\n";
					delete [] sText;
					break;
				}
				case ID3FID_USERTEXT:
				{
					char 
					*sText = ID3_GetString(frame, ID3FN_TEXT), 
					*sDesc = ID3_GetString(frame, ID3FN_DESCRIPTION);
					m_fi << "(" << sDesc << "): " << sText << "\n";
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
					m_fi << "(" << sDesc << ")[" << sLang << "]: " << sText << "\n";
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
					m_fi << sURL << "\n";
					delete [] sURL;
					break;
				}
				case ID3FID_WWWUSER:
				{
					char 
					*sURL = ID3_GetString(frame, ID3FN_URL),
					*sDesc = ID3_GetString(frame, ID3FN_DESCRIPTION);
					m_fi << "(" << sDesc << "): " << sURL << "\n";
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
						m_fi << sPeople;
						delete [] sPeople;
						if (nIndex + 1 < nItems)
							m_fi << ", ";
					}
					m_fi << "\n";
					break;
				}
				case ID3FID_PICTURE:
				{
					char
					*sMimeType = ID3_GetString(frame, ID3FN_MIMETYPE),
					*sDesc     = ID3_GetString(frame, ID3FN_DESCRIPTION),
					*sFormat   = ID3_GetString(frame, ID3FN_IMAGEFORMAT);
					size_t
					nPicType   = frame->GetField(ID3FN_PICTURETYPE)->Get(),
					nDataSize  = frame->GetField(ID3FN_DATA)->Size();
					m_fi << "(" << sDesc << ")[" << sFormat << ", "
						<< nPicType << "]: " << sMimeType << ", " << nDataSize
						<< " bytes" << "\n";
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
					m_fi << "(" << sDesc << ")[" 
						<< sFileName << "]: " << sMimeType << ", " << nDataSize
						<< " bytes" << "\n";
					delete [] sMimeType;
					delete [] sDesc;
					delete [] sFileName;
					break;
				}
				case ID3FID_UNIQUEFILEID:
				{
					char *sOwner = ID3_GetString(frame, ID3FN_OWNER);
					size_t nDataSize = frame->GetField(ID3FN_DATA)->Size();
					m_fi << sOwner << ", " << nDataSize
						<< " bytes" << "\n";
					delete [] sOwner;
					break;
				}
				case ID3FID_PLAYCOUNTER:
				{
					size_t nCounter = frame->GetField(ID3FN_COUNTER)->Get();
					m_fi << nCounter << "\n";
					break;
				}
				case ID3FID_POPULARIMETER:
				{
					char *sEmail = ID3_GetString(frame, ID3FN_EMAIL);
					size_t
					nCounter = frame->GetField(ID3FN_COUNTER)->Get(),
					nRating = frame->GetField(ID3FN_RATING)->Get();
					m_fi << sEmail << ", counter=" 
						<< nCounter << " rating=" << nRating << "\n";
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
					m_fi << "(" << nSymbol << "): " << sOwner
						<< ", " << nDataSize << " bytes" << "\n";
					break;
				}
				case ID3FID_SYNCEDLYRICS:
				{
					char 
					*sDesc = ID3_GetString(frame, ID3FN_DESCRIPTION), 
					*sLang = ID3_GetString(frame, ID3FN_LANGUAGE);
					size_t
					nTimestamp = frame->GetField(ID3FN_TIMESTAMPFORMAT)->Get(),
					nRating = frame->GetField(ID3FN_CONTENTTYPE)->Get();
					const char* format = (2 == nTimestamp) ? "ms" : "frames";
					m_fi << "(" << sDesc << ")[" << sLang << "]: ";
					switch (nRating)
					{
					case ID3CT_OTHER:    m_fi << "Other"; break;
					case ID3CT_LYRICS:   m_fi << "Lyrics"; break;
					case ID3CT_TEXTTRANSCRIPTION:     m_fi << "Text transcription"; break;
					case ID3CT_MOVEMENT: m_fi << "Movement/part name"; break;
					case ID3CT_EVENTS:   m_fi << "Events"; break;
					case ID3CT_CHORD:    m_fi << "Chord"; break;
					case ID3CT_TRIVIA:   m_fi << "Trivia/'pop up' information"; break;
					}
					m_fi << "\n";
					/*ID3_Field* fld = frame->GetField(ID3FN_DATA);
					if (fld)
					{
						ID3_MemoryReader mr(fld->GetRawBinary(), fld->BinSize());
						while (!mr.atEnd())
						{
							m_fi << io::readString(mr).c_str();
							m_fi << " [" << io::readBENumber(mr, sizeof(uint32)) << " " 
								<< format << "] ";
						}
					}
					m_fi << "\n";*/
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
					m_fi << " (unimplemented)" << "\n";
					break;
				default:
					m_fi << " frame" << "\n";
					break;
				}
			}
			delete iter;
		}
		catch(...){
			ASSERT(0);
		}
	}
	else
	{
		// starting the MediaDet object takes a noticeable amount of time.. avoid starting that object
		// for files which are not expected to contain any Audio/Video data.
		// note also: MediaDet does not work well for too short files (e.g. 16K)
		EED2KFileType eFileType = GetED2KFileTypeID(m_file->GetFileName());
		if ((eFileType == ED2KFT_AUDIO || eFileType == ED2KFT_VIDEO) && (!m_file->IsPartFile() || m_file->GetFileSize() >= 32768))
		{
			try{
				CComPtr<IMediaDet> pMediaDet;
				HRESULT hr = pMediaDet.CoCreateInstance(__uuidof(MediaDet));
				if (SUCCEEDED(hr))
				{
					USES_CONVERSION;
					if (SUCCEEDED(hr = pMediaDet->put_Filename(CComBSTR(T2W(m_file->GetFilePath())))))
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
											m_fi.SetSelectionCharFormat(m_cfBold);
											m_fi << "Video Stream\n";
											m_fi.SetSelectionCharFormat(m_cfDef);

											AM_MEDIA_TYPE mt = {0};
											if (SUCCEEDED(hr = pMediaDet->get_StreamMediaType(&mt)))
											{
												if (mt.formattype == FORMAT_VideoInfo)
												{
													VIDEOINFOHEADER* pVIH = (VIDEOINFOHEADER*)mt.pbFormat;

													CStringA strCodec;
													if (pVIH->bmiHeader.biCompression == BI_RGB)
														strCodec = "RGB";
													else if (pVIH->bmiHeader.biCompression == BI_RLE8)
														strCodec = "RLE8";
													else if (pVIH->bmiHeader.biCompression == BI_RLE4)
														strCodec = "RLE4";
													else if (pVIH->bmiHeader.biCompression == BI_BITFIELDS)
														strCodec = "BITFIELDS";
													else{
														memcpy(strCodec.GetBuffer(4), &pVIH->bmiHeader.biCompression, 4);
														strCodec.ReleaseBuffer(4);
														strCodec.MakeUpper();
													}
													m_fi << "   Codec:\t" << (LPCSTR)strCodec << "\n";

													m_fi << "   Width x Height:\t" << abs(pVIH->bmiHeader.biWidth) << " x " << abs(pVIH->bmiHeader.biHeight) << "\n";
													if (pVIH->dwBitRate)
														m_fi << "   Bitrate:\t" << (UINT)(pVIH->dwBitRate / 1000) << " kBit/s\n";

													double fFrameRate = 0.0;
													if (SUCCEEDED(pMediaDet->get_FrameRate(&fFrameRate)) && fFrameRate)
														m_fi << "   Frames/Sec:\t" << fFrameRate << "\n";
												}
											}

											double fLength = 0.0;
											if (SUCCEEDED(pMediaDet->get_StreamLength(&fLength)) && fLength)
											{
												CStringA strLength;
												SecToTimeLength(fLength, strLength);
												m_fi << "   Length:\t" << (LPCSTR)strLength;
												if (m_file->IsPartFile()){
													m_fi.SetSelectionCharFormat(m_cfRed);
													m_fi << " (This may not reflect the final total length!)";
													m_fi.SetSelectionCharFormat(m_cfDef);
												}
												m_fi << "\n";
											}

											if (mt.pUnk != NULL)
												mt.pUnk->Release();
											if (mt.pbFormat != NULL)
												CoTaskMemFree(mt.pbFormat);
											m_fi << "\n";
										}
										else if (major_type == MEDIATYPE_Audio)
										{
											m_fi.SetSelectionCharFormat(m_cfBold);
											m_fi << "Audio Stream\n";
											m_fi.SetSelectionCharFormat(m_cfDef);

											AM_MEDIA_TYPE mt = {0};
											if (SUCCEEDED(hr = pMediaDet->get_StreamMediaType(&mt)))
											{
												if (mt.formattype == FORMAT_WaveFormatEx)
												{
													WAVEFORMATEX* wfx = (WAVEFORMATEX*)mt.pbFormat;

													CString strComment;
													CString strFormat = GetWaveFormatTagName(wfx->wFormatTag, strComment);
													m_fi << "   Format:\t" << strFormat << " (" << strComment << ")\n";
													if (wfx->nAvgBytesPerSec)
														m_fi << "   Bitrate:\t" << (UINT)(((wfx->nAvgBytesPerSec * 8.0) + 500.0) / 1000.0) << " kBit/s\n";
													if (wfx->nSamplesPerSec)
														m_fi << "   Samples/Sec:\t" << wfx->nSamplesPerSec / 1000.0 << " kHz\n";
													if (wfx->wBitsPerSample)
														m_fi << "   Bit/Sample:\t" << wfx->wBitsPerSample << " Bit\n";

													m_fi << "   Mode:\t";
													if (wfx->nChannels == 1)
														m_fi << "Mono";
													else if (wfx->nChannels == 2)
														m_fi << "Stereo";
													else
														m_fi << wfx->nChannels << " channels";
													m_fi << "\n";
												}
											}

											double fLength = 0.0;
											if (SUCCEEDED(pMediaDet->get_StreamLength(&fLength)) && fLength)
											{
												CStringA strLength;
												SecToTimeLength(fLength, strLength);
												m_fi << "   Length:\t" << (LPCSTR)strLength;
												if (m_file->IsPartFile()){
													m_fi.SetSelectionCharFormat(m_cfRed);
													m_fi << " (This may not reflect the final total length!)";
													m_fi.SetSelectionCharFormat(m_cfDef);
												}
												m_fi << "\n";
											}

											if (mt.pUnk != NULL)
												mt.pUnk->Release();
											if (mt.pbFormat != NULL)
												CoTaskMemFree(mt.pbFormat);
											m_fi << "\n";
										}
										else{
											TRACE("%s - Unknown stream type\n", m_file->GetFileName());
										}
									}
								}
							}
						}
					}
					else{
						TRACE("Failed to open \"%s\" - %s\n", m_file->GetFilePath(), GetErrorMessage(hr, 1));
					}
				}
			}
			catch(...){
				ASSERT(0);
			}
		}
	}
}

void CFileInfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_ROUNDBIT, m_bAudioRoundBitrate);
	DDX_Control(pDX, IDC_FULL_FILE_INFO, m_fi);
}

void CFileInfoDialog::OnBnClickedRoundbit()
{
	char buffer[100];
	if(m_lAudioBitrate != 0)
	{
		CResizablePage::UpdateData();
		if(m_bAudioRoundBitrate)
		{
			long t = m_lAudioBitrate / (1024 / 8);
			if(t>=246 && t<=260)
				GetDlgItem(IDC_ABITRATE)->SetWindowText("256 Kbit/s");
			else if(t>=216 && t<=228)
				GetDlgItem(IDC_ABITRATE)->SetWindowText("224 Kbit/s");
			else if(t>=187 && t<=196)
				GetDlgItem(IDC_ABITRATE)->SetWindowText("192 Kbit/s");
			else if(t>=156 && t<=164)
				GetDlgItem(IDC_ABITRATE)->SetWindowText("160 Kbit/s");
			else if(t>=124 && t<=132)
				GetDlgItem(IDC_ABITRATE)->SetWindowText("128 Kbit/s");
			else if(t>=108 && t<=116)
				GetDlgItem(IDC_ABITRATE)->SetWindowText("112 Kbit/s");
			else if(t>=92 && t<=100)
				GetDlgItem(IDC_ABITRATE)->SetWindowText("96 Kbit/s");
			else if(t>=60 && t<=68)
				GetDlgItem(IDC_ABITRATE)->SetWindowText("64 Kbit/s");
			else
			{
				sprintf(buffer, "%d Kbit/s", t);
				GetDlgItem(IDC_ABITRATE)->SetWindowText(buffer);
			}
		}
		else
		{
			sprintf(buffer, "%d Kbit/s", m_lAudioBitrate / (1024 / 8));
			GetDlgItem(IDC_ABITRATE)->SetWindowText(buffer);
		}
	}
	else
		GetDlgItem(IDC_ABITRATE)->SetWindowText("");
}

void CFileInfoDialog::Localize()
{
	GetDlgItem(IDC_FD_XI1)->SetWindowText(GetResString(IDS_FD_SIZE));
	GetDlgItem(IDC_FD_XI2)->SetWindowText(GetResString(IDS_LENGTH)+":");
	GetDlgItem(IDC_FD_XI3)->SetWindowText(GetResString(IDS_VIDEO));
	GetDlgItem(IDC_FD_XI4)->SetWindowText(GetResString(IDS_AUDIO));

	GetDlgItem(IDC_FD_XI5)->SetWindowText( GetResString(IDS_CODEC)+":");
	GetDlgItem(IDC_FD_XI6)->SetWindowText( GetResString(IDS_CODEC)+":");
	
	GetDlgItem(IDC_FD_XI7)->SetWindowText( GetResString(IDS_BITRATE)+":");
	GetDlgItem(IDC_FD_XI8)->SetWindowText( GetResString(IDS_BITRATE)+":");
	
	GetDlgItem(IDC_FD_XI9)->SetWindowText( GetResString(IDS_WIDTH)+":");
	GetDlgItem(IDC_FD_XI11)->SetWindowText( GetResString(IDS_HEIGHT)+":");
	GetDlgItem(IDC_FD_XI13)->SetWindowText( GetResString(IDS_FPS)+":");
	GetDlgItem(IDC_FD_XI10)->SetWindowText( GetResString(IDS_CHANNELS)+":");
	GetDlgItem(IDC_FD_XI12)->SetWindowText( GetResString(IDS_SAMPLERATE)+":");
	GetDlgItem(IDC_ROUNDBIT)->SetWindowText( GetResString(IDS_ROUNDBITRATE)+":");
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
