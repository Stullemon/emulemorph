// parts of this file are based on work from pan One (http://home-3.tiscali.nl/~meost/pms/)
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
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <share.h>
#ifdef _DEBUG
#include "DebugHelpers.h"
#endif
#include "emule.h"
#include "KnownFile.h"
#include "KnownFileList.h"
#include "SharedFileList.h"
#include "UpDownClient.h"
#include "MMServer.h"
#include "ClientList.h"
#include "opcodes.h"
#include "ini2.h"
#define NOMD4MACROS
#include "kademlia/utils/md4.h"
#include "FrameGrabThread.h"
#include "CxImage/xImage.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "PartFile.h"
#include "Packets.h"
#include "Kademlia/Kademlia/SearchManager.h"
#include "SafeFile.h"
#include "shahashset.h"
#include "UploadQueue.h"

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
#include <qedit.h>
typedef struct tagVIDEOINFOHEADER {
    RECT            rcSource;          // The bit we really want to use
    RECT            rcTarget;          // Where the video should go
    DWORD           dwBitRate;         // Approximate bit data rate
    DWORD           dwBitErrorRate;    // Bit error rate for this stream
    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)
    BITMAPINFOHEADER bmiHeader;
} VIDEOINFOHEADER;

#include "emuledlg.h"
#include "SharedFilesWnd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define	META_DATA_VER	1

void CFileStatistic::MergeFileStats(CFileStatistic *toMerge)
{
	requested += toMerge->GetRequests();
	accepted += toMerge->GetAccepts();
	transferred += toMerge->GetTransferred();
	alltimerequested += toMerge->GetAllTimeRequests();
	alltimetransferred += toMerge->GetAllTimeTransferred();
	alltimeaccepted += toMerge->GetAllTimeAccepts();

	// SLUGFILLER: Spreadbars
	if (!toMerge->spreadlist.IsEmpty()) {
		POSITION pos = toMerge->spreadlist.GetHeadPosition();
		uint32 start = toMerge->spreadlist.GetKeyAt(pos);
		uint32 count = toMerge->spreadlist.GetValueAt(pos);
		toMerge->spreadlist.GetNext(pos);
		while (pos){
			uint32 end = toMerge->spreadlist.GetKeyAt(pos);
			if (count)
				AddBlockTransferred(start, end, count);
			start = end;
			count = toMerge->spreadlist.GetValueAt(pos);
			toMerge->spreadlist.GetNext(pos);
		}
	}
	// SLUGFILLER: Spreadbars
}

void CFileStatistic::AddRequest(){
	requested++;
	alltimerequested++;
	theApp.knownfiles->requested++;
	theApp.sharedfiles->UpdateFile(fileParent);
}
	
void CFileStatistic::AddAccepted(){
	accepted++;
	alltimeaccepted++;
	theApp.knownfiles->accepted++;
	theApp.sharedfiles->UpdateFile(fileParent);
}
	
void CFileStatistic::AddTransferred(uint32 start, uint32 bytes){	//MORPH - Added by IceCream, SLUGFILLER: Spreadbars
	transferred += bytes;
	alltimetransferred += bytes;
	theApp.knownfiles->transferred += bytes;
	AddBlockTransferred(start, start+bytes+1, 1);	//MORPH - Added by IceCream, SLUGFILLER: Spreadbars
	theApp.sharedfiles->UpdateFile(fileParent);
	m_bInChangedEqualChanceValue = false;	//Morph - added by AndCycle, Equal Chance For Each File
}

//MORPH START - Added by IceCream, SLUGFILLER: Spreadbars
void CFileStatistic::AddBlockTransferred(uint32 start, uint32 end, uint32 count){
	if (start >= end || !count)
		return;
	//MORPH START - Added by SiRoB, Reduce SpreadBar CPU consumption
	InChangedSpreadSortValue = false;
	InChangedFullSpreadCount = false;
	InChangedSpreadBar = false;
	//MORPH START - Added by SiRoB, Reduce SpreadBar CPU consumption
	
	if (spreadlist.IsEmpty())
		spreadlist.SetAt(0, 0);

	POSITION endpos = spreadlist.FindFirstKeyAfter(end+1);

	if (endpos)
		spreadlist.GetPrev(endpos);
	else
		endpos = spreadlist.GetTailPosition();

	ASSERT(endpos != NULL);

	uint32 endcount = spreadlist.GetValueAt(endpos);
	endpos = spreadlist.SetAt(end, endcount);

	POSITION startpos = spreadlist.FindFirstKeyAfter(start+1);

	for (POSITION pos = startpos; pos != endpos; spreadlist.GetNext(pos)) {
		spreadlist.SetValueAt(pos, spreadlist.GetValueAt(pos)+count);
	}

	spreadlist.GetPrev(startpos);

	ASSERT(startpos != NULL);

	uint32 startcount = spreadlist.GetValueAt(startpos)+count;
	startpos = spreadlist.SetAt(start, startcount);

	POSITION prevpos = startpos;
	spreadlist.GetPrev(prevpos);
	if (prevpos && spreadlist.GetValueAt(prevpos) == startcount)
		spreadlist.RemoveAt(startpos);

	prevpos = endpos;
	spreadlist.GetPrev(prevpos);
	if (prevpos && spreadlist.GetValueAt(prevpos) == endcount)
		spreadlist.RemoveAt(endpos);
}

CBarShader CFileStatistic::s_SpreadBar(16);

//MORPH START - Changed by SiRoB, Reduce SpreadBar CPU consumption
void CFileStatistic::DrawSpreadBar(CDC* dc, RECT* rect, bool bFlat) /*const*/
{
	int iWidth=rect->right - rect->left;
	if (iWidth <= 0)	return;
	int iHeight=rect->bottom - rect->top;
	uint32 filesize = fileParent->GetFileSize()?fileParent->GetFileSize():1;
	if (m_bitmapSpreadBar == (HBITMAP)NULL)
		VERIFY(m_bitmapSpreadBar.CreateBitmap(1, 1, 1, 8, NULL)); 
	CDC cdcStatus;
	HGDIOBJ hOldBitmap;
	cdcStatus.CreateCompatibleDC(dc);

	if(!InChangedSpreadBar || lastSize!=iWidth || lastbFlat!= bFlat){
		InChangedSpreadBar = true;
		lastSize=iWidth;
		lastbFlat=bFlat;
			m_bitmapSpreadBar.DeleteObject();
		m_bitmapSpreadBar.CreateCompatibleBitmap(dc,  iWidth, iHeight); 
		m_bitmapSpreadBar.SetBitmapDimension(iWidth,  iHeight); 
		hOldBitmap = cdcStatus.SelectObject(m_bitmapSpreadBar);
			
		s_SpreadBar.SetHeight(iHeight);
		s_SpreadBar.SetWidth(iWidth);
			
		s_SpreadBar.SetFileSize(filesize);
		s_SpreadBar.Fill(RGB(0, 0, 0));

		for(POSITION pos = spreadlist.GetHeadPosition(); pos; ){
			uint32 count = spreadlist.GetValueAt(pos);
			uint32 start = spreadlist.GetKeyAt(pos);
			spreadlist.GetNext(pos);
			if (!pos)
				break;
			uint32 end = spreadlist.GetKeyAt(pos);
			if (count)
				s_SpreadBar.FillRange(start, end, RGB(0,
				(232<22*count)? 0:232-22*count
				,255));
		}
		s_SpreadBar.Draw(&cdcStatus, 0, 0, bFlat);
	}
	else
		hOldBitmap = cdcStatus.SelectObject(m_bitmapSpreadBar);
	dc->BitBlt(rect->left, rect->top, iWidth, iHeight, &cdcStatus, 0, 0, SRCCOPY);
	cdcStatus.SelectObject(hOldBitmap);
}
//MORPH END  - Changed by SiRoB, Reduce SpreadBar CPU consumption
float CFileStatistic::GetSpreadSortValue() /*const*/
{
	//MORPH START - Added by SiRoB, Reduce SpreadBar CPU consumption
	if (InChangedSpreadSortValue) return lastSpreadSortValue;
	InChangedSpreadSortValue=true;
	//MORPH START - Added by SiRoB, Reduce SpreadBar CPU consumption
	float avg, calc;
	uint32 total = 0;
	uint32 filesize = fileParent->GetFileSize();

	if (!filesize || spreadlist.IsEmpty())
		return 0;

	POSITION pos = spreadlist.GetHeadPosition();
	uint32 start = spreadlist.GetKeyAt(pos);
	uint32 count = spreadlist.GetValueAt(pos);
	spreadlist.GetNext(pos);
	while (pos){
		uint32 end = spreadlist.GetKeyAt(pos);
		total += (end-start)*count;
		start = end;
		count = spreadlist.GetValueAt(pos);
		spreadlist.GetNext(pos);
	}

	avg = (float)total/filesize;
	calc = 0;
	pos = spreadlist.GetHeadPosition();
	start = spreadlist.GetKeyAt(pos);
	count = spreadlist.GetValueAt(pos);
	spreadlist.GetNext(pos);
	while (pos){
		uint32 end = spreadlist.GetKeyAt(pos);
		if ((float)count > avg)
			calc += avg*(end-start);
		else
			calc += count*(end-start);
		start = end;
		count = spreadlist.GetValueAt(pos);
		spreadlist.GetNext(pos);
	}
	calc /= filesize;
	//MORPH START - Added by SiRoB, Reduce Spread CPU consumption
	lastSpreadSortValue = calc;
	return calc;
	//MORPH START - Added by SiRoB, Reduce Spread CPU consumption
}

float CFileStatistic::GetFullSpreadCount() /*const*/
{
	//MORPH START - Added by SiRoB, Reduce SpreadBar CPU consumption
	if (InChangedFullSpreadCount) return lastFullSpreadCount;
	InChangedFullSpreadCount=true;
	//MORPH START - Added by SiRoB, Reduce SpreedBar CPU consumption
	float next;
	uint32 min;
	uint32 filesize = fileParent->GetFileSize();

	if (!filesize || spreadlist.IsEmpty())
		return 0;

	POSITION pos = spreadlist.GetHeadPosition();
	min = spreadlist.GetValueAt(pos);
	spreadlist.GetNext(pos);
	while (pos && spreadlist.GetKeyAt(pos) < filesize){
		uint32 count = spreadlist.GetValueAt(pos);
		if (min > count)
			min = count;
		spreadlist.GetNext(pos);
	}

	next = 0;
	pos = spreadlist.GetHeadPosition();
	uint32 start = spreadlist.GetKeyAt(pos);
	uint32 count = spreadlist.GetValueAt(pos);
	spreadlist.GetNext(pos);
	while (pos){
		uint32 end = spreadlist.GetKeyAt(pos);
		if (count > min)
			next += end-start;
		start = end;
		count = spreadlist.GetValueAt(pos);
		spreadlist.GetNext(pos);
	}
	next /= filesize;
	//MORPH START - Added by SiRoB, Reduce SpreadBar CPU consumption
	return lastFullSpreadCount = min+next;
	//MORPH END   - Added by SiRoB, Reduce SpreadBar CPU consumption
}
//MORPH END   - Added by IceCream, SLUGFILLER: Spreadbars

//Morph Start - added by AndCycle, Equal Chance For Each File
double CFileStatistic::GetEqualChanceValue()
{
	if(!thePrefs.IsEqualChanceEnable()){
		return 0;
	}
	//Morph - Added by AndCycle, Equal Chance For Each File, reduce CPU power
	else if(m_bInChangedEqualChanceValue){
		return m_dLastEqualChanceSemiValue/GetSharedTime();
	}
	m_bInChangedEqualChanceValue = true;
	//Morph - Added by AndCycle, Equal Chance For Each File, reduce CPU power

	//smaller value means greater priority
	m_dLastEqualChanceSemiValue = (double)GetAllTimeTransferred()/fileParent->GetFileSize();
	return m_dLastEqualChanceSemiValue/GetSharedTime();
}

CString CFileStatistic::GetEqualChanceValueString(bool detail){

	CString tempString;

	if(thePrefs.IsEqualChanceEnable())	{
		detail ?
			tempString.Format(_T("%s : %.2f = %s/%s"), CastSecondsToHM(GetSharedTime()), m_dLastEqualChanceSemiValue, CastItoXBytes(GetAllTimeTransferred()), CastItoXBytes(fileParent->GetFileSize())) :
			tempString.Format(_T("%s : %.2f"), CastSecondsToHM(GetSharedTime()), m_dLastEqualChanceSemiValue) ;
	}
	else{
		tempString.Empty();
	}

	return tempString;
}
//Morph End - added by AndCycle, Equal Chance For Each File

IMPLEMENT_DYNAMIC(CAbstractFile, CObject)

CAbstractFile::CAbstractFile()
{
	md4clr(m_abyFileHash);
	m_nFileSize = 0;
	m_uRating = 0;
}

#ifdef _DEBUG
void CAbstractFile::AssertValid() const
{
	CObject::AssertValid();
	(void)m_strFileName;
	(void)m_abyFileHash[16];
	(void)m_nFileSize;
	(void)m_strComment;
	(void)m_uRating;
	(void)m_strFileType;
	taglist.AssertValid();
}

void CAbstractFile::Dump(CDumpContext& dc) const
{
	CObject::Dump(dc);
}
#endif

IMPLEMENT_DYNAMIC(CKnownFile, CAbstractFile)

CKnownFile::CKnownFile()
{
	m_iPartCount = 0;
	m_iED2KPartCount = 0;
	m_iED2KPartHashCount = 0;
	m_tUtcLastModified = 0;
	if(thePrefs.GetNewAutoUp()){
		m_iUpPriority = PR_HIGH;
		m_bAutoUpPriority = true;
	}
	else{
		m_iUpPriority = PR_NORMAL;
		m_bAutoUpPriority = false;
	}
	statistic.fileParent = this;
	m_bCommentLoaded = false;
	(void)m_strComment;
	m_PublishedED2K = false;
	kadFileSearchID = 0;
	m_PublishedKadSrc = 0;
	m_lastPublishTimeKadSrc = 0;
	m_nCompleteSourcesTime = time(NULL);
	m_nCompleteSourcesCount = 1;
	m_nCompleteSourcesCountLo = 1;
	m_nCompleteSourcesCountHi = 1;
	m_uMetaDataVer = 0;
	m_pAICHHashSet = new CAICHHashSet(this);

	//MORPH START - Added by SiRoB, Show Permission
	m_iPermissions = -1;
	//MORPH END   - Added by SiRoB, Show Permission
	//MORPH START - Added by SiRoB, HIDEOS
	m_iHideOS = -1;
	m_iSelectiveChunk = -1;
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	m_iShareOnlyTheNeed = -1;
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	//MORPH START - Added by SiRoB, Avoid misusing of powershare
	m_powershared = -1;
	m_bPowerShareAuthorized = true;
	m_bPowerShareAuto = false;
	m_nVirtualCompleteSourcesCount = 0;
	//MORPH END   - Added by SiRoB, Avoid misusing of powershare
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	m_iPowerShareLimit = -1;
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	//MORPH START - Added by SiRoB, Reduce SharedStatusBAr CPU consumption
	InChangedSharedStatusBar = false;
	m_pbitmapOldSharedStatusBar = NULL;
	//MORPH END   - Added by SiRoB, Reduce SharedStatusBAr CPU consumption

	// Mighty Knife: CRC32-Tag
	m_sCRC32 [0] = '\0';
	// [end] Mighty Knife
}

CKnownFile::~CKnownFile()
{
	for (int i = 0; i < hashlist.GetSize(); i++)
		delete[] hashlist[i];
	for (int i = 0; i < taglist.GetSize(); i++)
		delete taglist[i];
	delete m_pAICHHashSet; 
	//MORPH START - Added by SiRoB, Reduce SharedStatusBar CPU consumption
	if(m_pbitmapOldSharedStatusBar != NULL) m_dcSharedStatusBar.SelectObject(m_pbitmapOldSharedStatusBar);
	//MORPH END   - Added by SiRoB, Reduce SharedStatusBar CPU consumption
}

#ifdef _DEBUG
void CKnownFile::AssertValid() const
{
	CAbstractFile::AssertValid();

	(void)m_tUtcLastModified;
	(void)statistic;
	(void)m_nCompleteSourcesTime;
	(void)m_nCompleteSourcesCount;
	(void)m_nCompleteSourcesCountLo;
	(void)m_nCompleteSourcesCountHi;
	m_ClientUploadList.AssertValid();
	m_AvailPartFrequency.AssertValid();
	hashlist.AssertValid();
	(void)m_strDirectory;
	(void)m_strFilePath;
	CHECK_BOOL(m_bCommentLoaded);
	(void)m_iPartCount;
	(void)m_iED2KPartCount;
	(void)m_iED2KPartHashCount;
	ASSERT( m_iUpPriority == PR_VERYLOW || m_iUpPriority == PR_LOW || m_iUpPriority == PR_NORMAL || m_iUpPriority == PR_HIGH || m_iUpPriority == PR_VERYHIGH );
	CHECK_BOOL(m_bAutoUpPriority);
	(void)s_ShareStatusBar;
	CHECK_BOOL(m_PublishedED2K);
	(void)kadFileSearchID;
	(void)m_lastPublishTimeKadSrc;
	(void)m_PublishedKadSrc;
	(void)wordlist;
}

void CKnownFile::Dump(CDumpContext& dc) const
{
	CAbstractFile::Dump(dc);
}
#endif

CBarShader CKnownFile::s_ShareStatusBar(16);
//MORPH START - Changed by SIRoB, Maella -Code Improvement-
//MORPH START - Modified by SiRoB, Reduce ShareStatusBar CPU consumption
void CKnownFile::DrawShareStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool  bFlat) /*const*/
{ 
	int iWidth=rect->right - rect->left;
	if (iWidth <= 0)	return;
	int iHeight=rect->bottom - rect->top;

	if (m_bitmapSharedStatusBar == (HBITMAP)NULL)
		VERIFY(m_bitmapSharedStatusBar.CreateBitmap(1, 1, 1, 8, NULL)); 
	CDC cdcStatus;
	HGDIOBJ hOldBitmap;
	cdcStatus.CreateCompatibleDC(dc);

	if(!InChangedSharedStatusBar || lastSize!=iWidth || lastonlygreyrect!=onlygreyrect || lastbFlat!=bFlat){
		InChangedSharedStatusBar = true;
		lastSize=iWidth;
		lastonlygreyrect=onlygreyrect;
		lastbFlat=bFlat;

		m_bitmapSharedStatusBar.DeleteObject();
		m_bitmapSharedStatusBar.CreateCompatibleBitmap(dc,  iWidth, iHeight); 
		m_bitmapSharedStatusBar.SetBitmapDimension(iWidth,  iHeight); 
		hOldBitmap = cdcStatus.SelectObject(m_bitmapSharedStatusBar);

		const COLORREF crMissing = RGB(255, 0, 0);
		s_ShareStatusBar.SetFileSize(GetFileSize()); 
		s_ShareStatusBar.SetHeight(iHeight);
		s_ShareStatusBar.SetWidth(iWidth);
		s_ShareStatusBar.Fill(crMissing); 
		
		if (!onlygreyrect && !m_AvailPartFrequency.IsEmpty()) { 
			COLORREF crProgress;
			COLORREF crHave;
			COLORREF crPending;

			if(bFlat) { 
				crProgress = RGB(0, 150, 0);
				crHave = RGB(0, 0, 0);
				crPending = RGB(255,208,0);
			} else { 
				crProgress = RGB(0, 224, 0);
				crHave = RGB(104, 104, 104);
				crPending = RGB(255, 208, 0);
			} 
			for (int i = 0; i < GetPartCount(); i++)
				if(m_AvailPartFrequency[i] > 0 ){
					COLORREF color = RGB(0, (210-(22*(m_AvailPartFrequency[i]-1)) <  0)? 0:210-(22*(m_AvailPartFrequency[i]-1)), 255);
						s_ShareStatusBar.FillRange(PARTSIZE*(i),PARTSIZE*(i+1),color);
				}
		}
		s_ShareStatusBar.Draw(&cdcStatus, 0, 0, bFlat); 
	}
	else
		hOldBitmap = cdcStatus.SelectObject(m_bitmapSharedStatusBar);
	dc->BitBlt(rect->left, rect->top, iWidth, iHeight, &cdcStatus, 0, 0, SRCCOPY);
	cdcStatus.SelectObject(hOldBitmap);
} 
//MORPH END - Changed by SiRoB, Maella -Code Improvement-

// SLUGFILLER: heapsortCompletesrc
static void HeapSort(CArray<uint16,uint16> &count, uint32 first, uint32 last){
	uint32 r;
	for ( r = first; !(r & 0x80000000) && (r<<1) < last; ){
		uint32 r2 = (r<<1)+1;
		if (r2 != last)
			if (count[r2] < count[r2+1])
				r2++;
		if (count[r] < count[r2]){
			uint16 t = count[r2];
			count[r2] = count[r];
			count[r] = t;
			r = r2;
		}
		else
			break;
	}
}
// SLUGFILLER: heapsortCompletesrc

void CKnownFile::UpdatePartsInfo()
{
	// Cache part count
	uint16 partcount = GetPartCount();
	bool flag = (time(NULL) - m_nCompleteSourcesTime > 0); 
	
	// Reset part counters
	if(m_AvailPartFrequency.GetSize() < partcount)
		m_AvailPartFrequency.SetSize(partcount);
	for(int i = 0; i < partcount; i++)
		m_AvailPartFrequency[i] = 1;

	CArray<uint16, uint16> count;
	if (flag)
		count.SetSize(0, m_ClientUploadList.GetSize());
	for (POSITION pos = m_ClientUploadList.GetHeadPosition(); pos != 0; )
	{
		CUpDownClient* cur_src = m_ClientUploadList.GetNext(pos);
		//This could be a partfile that just completed.. Maybe of these clients will not have this information.
		if(cur_src->m_abyUpPartStatus && cur_src->GetUpPartCount() == partcount )
		{
			for (uint16 i = 0; i < partcount; i++)
			{
				if (cur_src->IsUpPartAvailable(i))
					m_AvailPartFrequency[i] += 1;
			}
			if ( flag )
				count.Add(cur_src->GetUpCompleteSourcesCount());
		}
	}

	if (flag)
	{
		m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;

		if( partcount > 0)
			m_nCompleteSourcesCount = m_AvailPartFrequency[0];
		for (uint16 i = 1; i < partcount; i++)
		{
			if( m_nCompleteSourcesCount > m_AvailPartFrequency[i])
				m_nCompleteSourcesCount = m_AvailPartFrequency[i];
		}
	
		count.Add(m_nCompleteSourcesCount);
	
		int n = count.GetSize();
		if (n > 0)
		{
			// SLUGFILLER: heapsortCompletesrc
			int r;
			for (r = n/2; r--; )
				HeapSort(count, r, n-1);
			for (r = n; --r; ){
				uint16 t = count[r];
				count[r] = count[0];
				count[0] = t;
				HeapSort(count, 0, r-1);
			}
			// SLUGFILLER: heapsortCompletesrc
			
			// calculate range
			int i = n >> 1;			// (n / 2)
			int j = (n * 3) >> 2;	// (n * 3) / 4
			int k = (n * 7) >> 3;	// (n * 7) / 8

			//For complete files, trust the people your uploading to more...

			//For low guess and normal guess count
			//	If we see more sources then the guessed low and normal, use what we see.
			//	If we see less sources then the guessed low, adjust network accounts for 100%, we account for 0% with what we see and make sure we are still above the normal.
			//For high guess
			//  Adjust 100% network and 0% what we see.
			if (n < 20)
			{
				if ( count.GetAt(i) < m_nCompleteSourcesCount )
					m_nCompleteSourcesCountLo = m_nCompleteSourcesCount;
				else
				m_nCompleteSourcesCountLo= count.GetAt(i);
				m_nCompleteSourcesCount= m_nCompleteSourcesCountLo;
				m_nCompleteSourcesCountHi= count.GetAt(j);
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount )
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
			}
			else
			//Many sources..
			//For low guess
			//	Use what we see.
			//For normal guess
			//	Adjust network accounts for 100%, we account for 0% with what we see and make sure we are still above the low.
			//For high guess
			//  Adjust network accounts for 100%, we account for 0% with what we see and make sure we are still above the normal.
			{
				m_nCompleteSourcesCountLo= m_nCompleteSourcesCount;
				m_nCompleteSourcesCount= count.GetAt(j);
				if( m_nCompleteSourcesCount < m_nCompleteSourcesCountLo )
					m_nCompleteSourcesCount = m_nCompleteSourcesCountLo;
				m_nCompleteSourcesCountHi= count.GetAt(k);
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount )
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
			}
		}
		m_nCompleteSourcesTime = time(NULL) + (60);
	}
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	m_nVirtualCompleteSourcesCount = (uint16)-1;
	for (uint16 i = 0; i < partcount; i++){
		if((m_AvailPartFrequency[i]) < m_nVirtualCompleteSourcesCount)
			m_nVirtualCompleteSourcesCount = m_AvailPartFrequency[i];
	}
	UpdatePowerShareLimit(m_nCompleteSourcesCountHi<200, m_nCompleteSourcesCountHi==1 && m_nVirtualCompleteSourcesCount==1,m_nCompleteSourcesCountHi>((GetPowerShareLimit()>=0)?GetPowerShareLimit():thePrefs.GetPowerShareLimit()));
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by SiRoB, Reduce ShareStatusBar CPU consumption
	InChangedSharedStatusBar = false;
	//MORPH END   - Added by SiRoB, Reduce ShareStatusBar CPU consumption
	if (theApp.emuledlg->sharedfileswnd->m_hWnd)
		theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
}

void CKnownFile::AddUploadingClient(CUpDownClient* client){
	POSITION pos = m_ClientUploadList.Find(client); // to be sure
	if(pos == NULL){
		m_ClientUploadList.AddTail(client);
		UpdateAutoUpPriority();
	}
}

void CKnownFile::RemoveUploadingClient(CUpDownClient* client){
	POSITION pos = m_ClientUploadList.Find(client); // to be sure
	if(pos != NULL){
		m_ClientUploadList.RemoveAt(pos);
		UpdateAutoUpPriority();
	}
}

#ifdef _DEBUG
void Dump(const Kademlia::WordList& wordlist)
{
	Kademlia::WordList::const_iterator it;
	for (it = wordlist.begin(); it != wordlist.end(); it++)
	{
		const CStringW& rstrKeyword = *it;
		TRACE("  %ls\n", rstrKeyword);
	}
}
#endif

void CKnownFile::SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars)
{ 
	CAbstractFile::SetFileName(pszFileName, bReplaceInvalidFileSystemChars);

	wordlist.clear();
	Kademlia::CSearchManager::getWords(GetFileName(), &wordlist);
} 

void CKnownFile::SetPath(LPCTSTR path)
{
	m_strDirectory = path;
}

void CKnownFile::SetFilePath(LPCTSTR pszFilePath)
{
	m_strFilePath = pszFilePath;
}

bool CKnownFile::CreateFromFile(LPCTSTR in_directory, LPCTSTR in_filename, LPVOID pvProgressParam)
{
	SetPath(in_directory);
	SetFileName(in_filename);

	// open file
	CString strFilePath;
	_tmakepath(strFilePath.GetBuffer(MAX_PATH), NULL, in_directory, in_filename, NULL);
	strFilePath.ReleaseBuffer();
	SetFilePath(strFilePath);
	FILE* file = _tfsopen(strFilePath, _T("rbS"), _SH_DENYNO); // can not use _SH_DENYWR because we may access a completing part file
	if (!file){
		theApp.QueueLogLine(false, GetResString(IDS_ERR_FILEOPEN) + _T(" - %hs"), strFilePath, _T(""), strerror(errno));
		return false;
	}

	// set filesize
	if (_filelengthi64(file->_file)>=4294967296){
		fclose(file);
		return false; // not supported by network
	}
	SetFileSize(_filelength(file->_file));

	// we are reading the file data later in 8K blocks, adjust the internal file stream buffer accordingly
	setvbuf(file, NULL, _IOFBF, 1024*8*2);

	m_AvailPartFrequency.SetSize(GetPartCount());
	for (uint32 i = 0; i < GetPartCount();i++)
		m_AvailPartFrequency[i] = 0;

	// create hashset
	uint32 togo = m_nFileSize;
	for (uint16 hashcount = 0; togo >= PARTSIZE; ) {
		uchar* newhash = new uchar[16];
		CAICHHashTree* pBlockAICHHashTree = m_pAICHHashSet->m_pHashTree.FindHash(hashcount*PARTSIZE, PARTSIZE);
		ASSERT( pBlockAICHHashTree != NULL );
		CreateHashFromFile(file, PARTSIZE, newhash, pBlockAICHHashTree);
		// SLUGFILLER: SafeHash - quick fallback
		if (theApp.emuledlg==NULL || !theApp.emuledlg->IsRunning()){ // in case of shutdown while still hashing
			fclose(file);
			delete[] newhash;
			return false;
		}
		// SLUGFILLER: SafeHash
		hashlist.Add(newhash);
		togo -= PARTSIZE;
		hashcount++;

		if (pvProgressParam && theApp.emuledlg && theApp.emuledlg->IsRunning()){
			ASSERT( ((CKnownFile*)pvProgressParam)->IsKindOf(RUNTIME_CLASS(CKnownFile)) );
			ASSERT( ((CKnownFile*)pvProgressParam)->GetFileSize() == GetFileSize() );
			UINT uProgress = ((ULONGLONG)(GetFileSize() - togo) * 100) / GetFileSize();
			ASSERT( uProgress <= 100 );
			VERIFY( PostMessage(theApp.emuledlg->GetSafeHwnd(), TM_FILEOPPROGRESS, uProgress, (LPARAM)pvProgressParam) );
		}
	}
	uchar* lasthash = new uchar[16];
	md4clr(lasthash);

	CAICHHashTree* pBlockAICHHashTree;
	if (togo == 0)
		pBlockAICHHashTree = NULL; // sha hashtree doesnt takes hash of 0-sized data
	else{
		pBlockAICHHashTree = m_pAICHHashSet->m_pHashTree.FindHash(hashcount*PARTSIZE, togo);
		ASSERT( pBlockAICHHashTree != NULL );
	}
	
	CreateHashFromFile(file, togo, lasthash, pBlockAICHHashTree);	
	
	m_pAICHHashSet->ReCalculateHash(false);
	if ( m_pAICHHashSet->VerifyHashTree(true) ){
		m_pAICHHashSet->SetStatus(AICH_HASHSETCOMPLETE);
		if (!m_pAICHHashSet->SaveHashSet()){
			theApp.QueueLogLine(true, GetResString(IDS_SAVEACFAILED));
		}
	}
	else{
		// now something went pretty wrong
		theApp.QueueDebugLogLine(true,_T("Failed to calculate AICH Hashset from file %s"), GetFileName());
	}


	if (!hashcount){
		md4cpy(m_abyFileHash, lasthash);
		delete[] lasthash; // i_a: memleak 
	} 
	else {
		hashlist.Add(lasthash);
		uchar* buffer = new uchar[hashlist.GetCount()*16];
		for (int i = 0; i < hashlist.GetCount(); i++)
			md4cpy(buffer+(i*16), hashlist[i]);
		CreateHashFromString(buffer, hashlist.GetCount()*16, m_abyFileHash);
		delete[] buffer;
	}

	if (pvProgressParam && theApp.emuledlg && theApp.emuledlg->IsRunning()){
		ASSERT( ((CKnownFile*)pvProgressParam)->IsKindOf(RUNTIME_CLASS(CKnownFile)) );
		ASSERT( ((CKnownFile*)pvProgressParam)->GetFileSize() == GetFileSize() );
		UINT uProgress = 100;
		ASSERT( uProgress <= 100 );
		VERIFY( PostMessage(theApp.emuledlg->GetSafeHwnd(), TM_FILEOPPROGRESS, uProgress, (LPARAM)pvProgressParam) );
	}

	// set lastwrite date
	struct _stat fileinfo = {0};
	_fstat(file->_file, &fileinfo);
	m_tUtcLastModified = fileinfo.st_mtime;
	AdjustNTFSDaylightFileTime(m_tUtcLastModified, strFilePath);
	//Morph Start - Added by AndCycle, Equal Chance For Each File
	statistic.SetSharedTime(0); //first time shared file
	//Morph End - Added by AndCycle, Equal Chance For Each File

	fclose(file);
	file = NULL;

	// Add filetags
	UpdateMetaDataTags();

	UpdatePartsInfo();

	// Mighty Knife: Report hashing files
	if (thePrefs.GetReportHashingFiles ()) {
		CString hashfilename;
		hashfilename.Format (_T("%s\\%s"),in_directory, in_filename);
		if (hashfilename.Find (_T("\\\\")) >= 0) hashfilename.Format (_T("%s%s"),in_directory, in_filename);
		AddLogLine(false, _T("Completed hashing of file '%s'."), hashfilename);
	}
	// [end] Mighty Knife
	return true;	
}

bool CKnownFile::CreateAICHHashSetOnly()
{
	ASSERT( !IsPartFile() );
	m_pAICHHashSet->FreeHashSet();
	FILE* file = _tfsopen(GetFilePath(), _T("rbS"), _SH_DENYNO); // can not use _SH_DENYWR because we may access a completing part file
	if (!file){
		theApp.QueueLogLine(false, GetResString(IDS_ERR_FILEOPEN) + _T(" - %hs"), GetFilePath(), _T(""), strerror(errno));
		return false;
	}
	// we are reading the file data later in 8K blocks, adjust the internal file stream buffer accordingly
	setvbuf(file, NULL, _IOFBF, 1024*8*2);

	// create aichhashset
	uint32 togo = m_nFileSize;
	for (uint16 hashcount = 0; togo >= PARTSIZE; ) {
		CAICHHashTree* pBlockAICHHashTree = m_pAICHHashSet->m_pHashTree.FindHash(hashcount*PARTSIZE, PARTSIZE);
		ASSERT( pBlockAICHHashTree != NULL );
		CreateHashFromFile(file, PARTSIZE, NULL, pBlockAICHHashTree);
		// SLUGFILLER: SafeHash - quick fallback
		if (theApp.emuledlg==NULL || !theApp.emuledlg->IsRunning()){ // in case of shutdown while still hashing
			fclose(file);
			return false;
		}

		togo -= PARTSIZE;
		hashcount++;
	}

	if (togo != 0){
		CAICHHashTree* pBlockAICHHashTree = m_pAICHHashSet->m_pHashTree.FindHash(hashcount*PARTSIZE, togo);
		ASSERT( pBlockAICHHashTree != NULL );
		CreateHashFromFile(file, togo, NULL, pBlockAICHHashTree);
	}
	
	m_pAICHHashSet->ReCalculateHash(false);
	if ( m_pAICHHashSet->VerifyHashTree(true) ){
		m_pAICHHashSet->SetStatus(AICH_HASHSETCOMPLETE);
		if (!m_pAICHHashSet->SaveHashSet()){
			theApp.QueueLogLine(true, GetResString(IDS_SAVEACFAILED));
		}
	}
	else{
		// now something went pretty wrong
		theApp.QueueDebugLogLine(true,_T("Failed to calculate AICH Hashset from file %s"), GetFileName());
	}
	
	fclose(file);
	file = NULL;
	return true;	
}

void CKnownFile::SetFileSize(uint32 nFileSize)
{
	CAbstractFile::SetFileSize(nFileSize);
	m_pAICHHashSet->SetFileSize(nFileSize);

	// Examples of parthashs, hashsets and filehashs for different filesizes
	// according the ed2k protocol
	//----------------------------------------------------------------------
	//
	//File size: 3 bytes
	//File hash: 2D55E87D0E21F49B9AD25F98531F3724
	//Nr. hashs: 0
	//
	//
	//File size: 1*PARTSIZE
	//File hash: A72CA8DF7F07154E217C236C89C17619
	//Nr. hashs: 2
	//Hash[  0]: 4891ED2E5C9C49F442145A3A5F608299
	//Hash[  1]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
	//
	//
	//File size: 1*PARTSIZE + 1 byte
	//File hash: 2F620AE9D462CBB6A59FE8401D2B3D23
	//Nr. hashs: 2
	//Hash[  0]: 121795F0BEDE02DDC7C5426D0995F53F
	//Hash[  1]: C329E527945B8FE75B3C5E8826755747
	//
	//
	//File size: 2*PARTSIZE
	//File hash: A54C5E562D5E03CA7D77961EB9A745A4
	//Nr. hashs: 3
	//Hash[  0]: B3F5CE2A06BF403BFB9BFFF68BDDC4D9
	//Hash[  1]: 509AA30C9EA8FC136B1159DF2F35B8A9
	//Hash[  2]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
	//
	//
	//File size: 3*PARTSIZE
	//File hash: 5E249B96F9A46A18FC2489B005BF2667
	//Nr. hashs: 4
	//Hash[  0]: 5319896A2ECAD43BF17E2E3575278E72
	//Hash[  1]: D86EF157D5E49C5ED502EDC15BB5F82B
	//Hash[  2]: 10F2D5B1FCB95C0840519C58D708480F
	//Hash[  3]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
	//
	//
	//File size: 3*PARTSIZE + 1 byte
	//File hash: 797ED552F34380CAFF8C958207E40355
	//Nr. hashs: 4
	//Hash[  0]: FC7FD02CCD6987DCF1421F4C0AF94FB8
	//Hash[  1]: 2FE466AF8A7C06DA3365317B75A5ACFE
	//Hash[  2]: 873D3BF52629F7C1527C6E8E473C1C30
	//Hash[  3]: BCE50BEE7877BB07BB6FDA56BFE142FB
	//

	// File size       Data parts      ED2K parts      ED2K part hashs
	// ---------------------------------------------------------------
	// 1..PARTSIZE-1   1               1               0(!)
	// PARTSIZE        1               2(!)            2(!)
	// PARTSIZE+1      2               2               2
	// PARTSIZE*2      2               3(!)            3(!)
	// PARTSIZE*2+1    3               3               3

	if (nFileSize == 0){
// Mighty Knife: No Assert here; function works !
//		ASSERT(0);
// [end] Mighty Knife
		m_iPartCount = 0;
		m_iED2KPartCount = 0;
		m_iED2KPartHashCount = 0;
		return;
	}

	// nr. of data parts
	ASSERT( (uint64)(((uint64)nFileSize + (PARTSIZE - 1)) / PARTSIZE) <= (UINT)USHRT_MAX );
	m_iPartCount = ((uint64)nFileSize + (PARTSIZE - 1)) / PARTSIZE;

	// nr. of parts to be used with OP_FILESTATUS
	m_iED2KPartCount = nFileSize / PARTSIZE + 1;

	// nr. of parts to be used with OP_HASHSETANSWER
	m_iED2KPartHashCount = nFileSize / PARTSIZE;
	if (m_iED2KPartHashCount != 0)
		m_iED2KPartHashCount += 1;
}

// needed for memfiles. its probably better to switch everything to CFile...
bool CKnownFile::LoadHashsetFromFile(CFileDataIO* file, bool checkhash){
	uchar checkid[16];
	file->ReadHash16(checkid);
	//TRACE("File size: %u (%u full parts + %u bytes)\n", GetFileSize(), GetFileSize()/PARTSIZE, GetFileSize()%PARTSIZE);
	//TRACE("File hash: %s\n", md4str(checkid));
	ASSERT( hashlist.GetCount() == 0 );
	UINT parts = file->ReadUInt16();
	//TRACE("Nr. hashs: %u\n", (UINT)parts);
	for (UINT i = 0; i < parts; i++){
		uchar* cur_hash = new uchar[16];
		file->ReadHash16(cur_hash);
		//TRACE("Hash[%3u]: %s\n", i, md4str(cur_hash));
		hashlist.Add(cur_hash);
	}

	// SLUGFILLER: SafeHash - always check for valid hashlist
	if (!checkhash){
		md4cpy(m_abyFileHash, checkid);
		if (parts <= 1)	// nothing to check
			return true;
	}
	else if (md4cmp(m_abyFileHash, checkid)){
		// delete hashset
		for (int i = 0; i < hashlist.GetSize(); i++)
			delete[] hashlist[i];
		hashlist.RemoveAll();
		return false;	// wrong file?
	}
	else{
		if (parts != GetED2KPartHashCount()){
			// delete hashset
			for (int i = 0; i < hashlist.GetSize(); i++)
				delete[] hashlist[i];
			hashlist.RemoveAll();
			return false;
		}
	}
	// SLUGFILLER: SafeHash

	if (!hashlist.IsEmpty()){
		uchar* buffer = new uchar[hashlist.GetCount()*16];
		for (int i = 0; i < hashlist.GetCount(); i++)
			md4cpy(buffer+(i*16), hashlist[i]);
		CreateHashFromString(buffer, hashlist.GetCount()*16, checkid);
		delete[] buffer;
	}
	if (!md4cmp(m_abyFileHash, checkid))
		return true;
	else{
		// delete hashset
		for (int i = 0; i < hashlist.GetSize(); i++)
			delete[] hashlist[i];
		hashlist.RemoveAll();
		return false;
	}
}

bool CKnownFile::SetHashset(const CArray<uchar*, uchar*>& aHashset)
{
	// delete hashset
	for (int i = 0; i < hashlist.GetSize(); i++)
		delete[] hashlist[i];
	hashlist.RemoveAll();

	// set new hash
	for (int i = 0; i < aHashset.GetSize(); i++)
	{
		uchar* pucHash = new uchar[16];
		md4cpy(pucHash, aHashset.GetAt(i));
		hashlist.Add(pucHash);
	}

	// verify new hash
	if (hashlist.IsEmpty())
		return true;

	uchar aucHashsetHash[16];
	uchar* buffer = new uchar[hashlist.GetCount()*16];
	for (int i = 0; i < hashlist.GetCount(); i++)
		md4cpy(buffer+(i*16), hashlist[i]);
	CreateHashFromString(buffer, hashlist.GetCount()*16, aucHashsetHash);
	delete[] buffer;

	bool bResult = (md4cmp(aucHashsetHash, m_abyFileHash) == 0);
	if (!bResult)
	{
		// delete hashset
		for (int i = 0; i < hashlist.GetSize(); i++)
			delete[] hashlist[i];
		hashlist.RemoveAll();
	}
	return bResult;
}

bool CKnownFile::LoadTagsFromFile(CFileDataIO* file)
{
	//MORPH START - Added by SiRoB, SLUGFILLER: Spreadbars
	// SLUGFILLER: Spreadbars
	CMap<uint16,uint16,uint32,uint32> spread_start_map;
	CMap<uint16,uint16,uint32,uint32> spread_end_map;
	CMap<uint16,uint16,uint32,uint32> spread_count_map;
	// SLUGFILLER: Spreadbars
	//MORPH END - Added by SiRoB, SLUGFILLER: Spreadbars
	UINT tagcount = file->ReadUInt32();
	for (UINT j = 0; j < tagcount; j++){
		CTag* newtag = new CTag(file, false);
		switch (newtag->GetNameID()){
			case FT_FILENAME:{
				ASSERT( newtag->IsStr() );
				if (newtag->IsStr()){
#ifdef _UNICODE
					if (GetFileName().IsEmpty())
#endif
						SetFileName(newtag->GetStr());
				}
				delete newtag;
				break;
			}
			case FT_FILESIZE:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
				{
					SetFileSize(newtag->GetInt());
					m_AvailPartFrequency.SetSize(GetPartCount());
					for (uint32 i = 0; i < GetPartCount();i++)
						m_AvailPartFrequency[i] = 0;
				}
				delete newtag;
				break;
			}
			case FT_ATTRANSFERED:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					statistic.alltimetransferred = newtag->GetInt();
				delete newtag;
				break;
			}
			case FT_ATTRANSFEREDHI:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
				{
					uint32 hi,low;
					low=statistic.alltimetransferred;
					hi = newtag->GetInt();
					uint64 hi2;
					hi2=hi;
					hi2=hi2<<32;
					statistic.alltimetransferred=low+hi2;
				}
				delete newtag;
				break;
			}
			case FT_ATREQUESTED:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					statistic.alltimerequested = newtag->GetInt();
				delete newtag;
				break;
			}
 			case FT_ATACCEPTED:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					statistic.alltimeaccepted = newtag->GetInt();
				delete newtag;
				break;
			}
			case FT_ULPRIORITY:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
				{
					m_iUpPriority = newtag->GetInt();
					if( m_iUpPriority == PR_AUTO ){
						m_iUpPriority = PR_HIGH;
						m_bAutoUpPriority = true;
					}
					else{
						if (m_iUpPriority != PR_VERYLOW && m_iUpPriority != PR_LOW && m_iUpPriority != PR_NORMAL && m_iUpPriority != PR_HIGH && m_iUpPriority != PR_VERYHIGH)
							m_iUpPriority = PR_NORMAL;
						m_bAutoUpPriority = false;
					}
				}
				delete newtag;
				break;
			}
			case FT_KADLASTPUBLISHSRC:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					m_lastPublishTimeKadSrc = newtag->GetInt();
				delete newtag;
				break;
			}
			case FT_FLAGS:
				// Misc. Flags
				// ------------------------------------------------------------------------------
				// Bits  3-0: Meta data version
				//				0 = Unknown
				//				1 = we have created that meta data by examining the file contents.
				// Bits 31-4: Reserved
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					m_uMetaDataVer = newtag->GetInt() & 0x0F;
				delete newtag;
				break;
			// old tags: as long as they are not needed, take the chance to purge them
			//MORPH START - Added by SiRoB, Show Permission
			case FT_PERMISSIONS:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
				{
					m_iPermissions = newtag->GetInt();
					// Mighty Knife: Community visible filelist
					if (m_iPermissions != PERM_ALL && m_iPermissions != PERM_FRIENDS && m_iPermissions != PERM_NOONE && m_iPermissions != PERM_COMMUNITY)
						m_iPermissions = -1;
					// [end] Mighty Knife
				}
				delete newtag;
				break;
			}
			//MORPH END  - Added by SiRoB, Show Permission
			case FT_KADLASTPUBLISHKEY:
				ASSERT( newtag->IsInt() );
				delete newtag;
				break;
			case FT_AICH_HASH:{
				if(!newtag->IsStr()){
					//ASSERT( false ); uncomment later
					break;
				}
				CAICHHash hash;
				if (DecodeBase32(newtag->GetStr(),hash) == CAICHHash::GetHashSize())
					m_pAICHHashSet->SetMasterHash(hash, AICH_HASHSETCOMPLETE);
				else
					ASSERT( false );
				delete newtag;
				break;
			}
			// EastShare START - Added by TAHO, .met file control
			case FT_LASTUSED:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					statistic.SetLastUsed(newtag->GetInt());
				delete newtag;
				break;
			}
			// EastShare END - Added by TAHO, .met file control
			//Morph - Added by AndCycle, Equal Chance For Each File
			case FT_ATSHARED:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					statistic.SetSharedTime(newtag->GetInt());
				delete newtag;
				break;
			}
			//Morph - Added by AndCycle, Equal Chance For Each File
			default:
				//MORPH START - Changed by SiRoB, SLUGFILLER: Spreadbars
				// Mighty Knife: Take care of corrupted tags !!!
				if(!newtag->GetNameID() && newtag->IsInt() && newtag->GetName()){
				// [end] Mighty Knife
					uint16 spreadkey = atoi(&newtag->GetName()[1]);
					if (newtag->GetName()[0] == FT_SPREADSTART)
						spread_start_map.SetAt(spreadkey, newtag->GetInt());
					else if (newtag->GetName()[0] == FT_SPREADEND)
						spread_end_map.SetAt(spreadkey, newtag->GetInt());
					else if (newtag->GetName()[0] == FT_SPREADCOUNT)
						spread_count_map.SetAt(spreadkey, newtag->GetInt());
					//MORPH START - Added by SiRoB, ZZ Upload System				
					else if(strcmp(newtag->GetName(), FT_POWERSHARE) == 0)
						//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
						//SetPowerShared(newtag->GetInt() == 1);
						SetPowerShared((newtag->GetInt()<=3)?newtag->GetInt():-1);
						//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
					//MORPH END   - Added by SiRoB, ZZ Upload System
					//MORPH START - Added by SiRoB, POWERSHARE Limit
					else if(strcmp(newtag->GetName(), FT_POWERSHARE_LIMIT) == 0)
						SetPowerShareLimit((newtag->GetInt()<=200)?newtag->GetInt():-1);
					//MORPH END   - Added by SiRoB, POWERSHARE Limit
					//MORPH START - Added by SiRoB, HIDEOS
					else if(strcmp(newtag->GetName(), FT_HIDEOS) == 0)
						SetHideOS((newtag->GetInt()<=10)?newtag->GetInt():-1);
					else if(strcmp(newtag->GetName(), FT_SELECTIVE_CHUNK) == 0)
						SetSelectiveChunk(newtag->GetInt()<=1?newtag->GetInt():-1);
					//MORPH END   - Added by SiRoB, HIDEOS
					//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
					else if(strcmp(newtag->GetName(), FT_SHAREONLYTHENEED) == 0)
						SetShareOnlyTheNeed(newtag->GetInt()<=1?newtag->GetInt():-1);
					//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
					delete newtag;
					break;
				}
				//MORPH END - Changed by SiRoB, SLUGFILLER: Spreadbars
				ConvertED2KTag(newtag);
				if (newtag)
					taglist.Add(newtag);
		}	
	}
	//MORPH START - Added by SiRoB, SLUGFILLER: Spreadbars - Now to flush the map into the list
	for (POSITION pos = spread_start_map.GetStartPosition(); pos != NULL; ){
		uint16 spreadkey;
		uint32 spread_start;
		uint32 spread_end;
		uint32 spread_count;
		spread_start_map.GetNextAssoc(pos, spreadkey, spread_start);
		if (!spread_end_map.Lookup(spreadkey, spread_end))
			continue;
		if (!spread_count_map.Lookup(spreadkey, spread_count))
			continue;
		if (!spread_count || spread_start >= spread_end)
			continue;
		statistic.AddBlockTransferred(spread_start, spread_end, spread_count);	// All tags accounted for
	}
	//MORPH END   - Added by SiRoB, SLUGFILLER: Spreadbars

	// 05-Jän-2004 [bc]: ed2k and Kad are already full of totally wrong and/or not properly attached meta data. Take
	// the chance to clean any available meta data tags and provide only tags which were determined by us.
	// It's a brute force method, but that wrong meta data is driving me crazy because wrong meta data is even worse than
	// missing meta data.
	if (m_uMetaDataVer == 0)
		RemoveMetaDataTags();

	return true;
}

bool CKnownFile::LoadDateFromFile(CFileDataIO* file){
	m_tUtcLastModified = file->ReadUInt32();
	//Morph Start - Added by AndCycle, Equal Chance For Each File
	//init for file exist in known
	statistic.SetSharedTime(time(NULL) - m_tUtcLastModified);
	//Morph End - Added by AndCycle, Equal Chance For Each File
	return true;
}

bool CKnownFile::LoadFromFile(CFileDataIO* file){
	// SLUGFILLER: SafeHash - load first, verify later
	bool ret1 = LoadDateFromFile(file);
	bool ret2 = LoadHashsetFromFile(file,false);
	bool ret3 = LoadTagsFromFile(file);
	UpdatePartsInfo();
	return ret1 && ret2 && ret3 && GetED2KPartHashCount()==GetHashCount();// Final hash-count verification, needs to be done after the tags are loaded.
	// SLUGFILLER: SafeHash
}

bool CKnownFile::WriteToFile(CFileDataIO* file)
{
	// date
	file->WriteUInt32(m_tUtcLastModified);

	// hashset
	file->WriteHash16(m_abyFileHash);
	UINT parts = hashlist.GetCount();
	file->WriteUInt16(parts);
	for (UINT i = 0; i < parts; i++)
		file->WriteHash16(hashlist[i]);
	//tags
	uint32 uTagCount = 0;
	ULONG uTagCountFilePos = (ULONG)file->GetPosition();
	file->WriteUInt32(uTagCount);
	
#ifdef _UNICODE
	if (WriteOptED2KUTF8Tag(file, GetFileName(), FT_FILENAME))
		uTagCount++;
#endif
	CTag nametag(FT_FILENAME, GetFileName());
	nametag.WriteTagToFile(file);
	uTagCount++;
	
	CTag sizetag(FT_FILESIZE, m_nFileSize);
	sizetag.WriteTagToFile(file);
	uTagCount++;
	
	// statistic
	if (statistic.alltimetransferred){
		CTag attag1(FT_ATTRANSFERED, (uint32)statistic.alltimetransferred);
		attag1.WriteTagToFile(file);
		uTagCount++;
		
		CTag attag4(FT_ATTRANSFEREDHI, (uint32)(statistic.alltimetransferred >> 32));
		attag4.WriteTagToFile(file);
		uTagCount++;
	}

	if (statistic.GetAllTimeRequests()){
		CTag attag2(FT_ATREQUESTED, statistic.GetAllTimeRequests());
		attag2.WriteTagToFile(file);
		uTagCount++;
	}
	
	if (statistic.GetAllTimeAccepts()){
		CTag attag3(FT_ATACCEPTED, statistic.GetAllTimeAccepts());
		attag3.WriteTagToFile(file);
		uTagCount++;
	}

	// priority N permission
	CTag priotag(FT_ULPRIORITY, IsAutoUpPriority() ? PR_AUTO : m_iUpPriority);
	priotag.WriteTagToFile(file);
	uTagCount++;

	//AICH Filehash
	if (m_pAICHHashSet->HasValidMasterHash() && (m_pAICHHashSet->GetStatus() == AICH_HASHSETCOMPLETE || m_pAICHHashSet->GetStatus() == AICH_VERIFIED)){
		CTag aichtag(FT_AICH_HASH, m_pAICHHashSet->GetMasterHash().GetString());
		aichtag.WriteTagToFile(file);
		uTagCount++;
	}


	if (m_lastPublishTimeKadSrc){
		CTag kadLastPubSrc(FT_KADLASTPUBLISHSRC, m_lastPublishTimeKadSrc);
		kadLastPubSrc.WriteTagToFile(file);
		uTagCount++;
	}

	if (m_uMetaDataVer > 0)
	{
		// Misc. Flags
		// ------------------------------------------------------------------------------
		// Bits  3-0: Meta data version
		//				0 = Unknown
		//				1 = we have created that meta data by examining the file contents.
		// Bits 31-4: Reserved
		ASSERT( m_uMetaDataVer <= 0x0F );
		uint32 uFlags = m_uMetaDataVer & 0x0F;
		CTag tagFlags(FT_FLAGS, uFlags);
		tagFlags.WriteTagToFile(file);
		uTagCount++;
	}

	//MOPRH START - Added by SiRoB, Show Permissions
	if (GetPermissions()>=0){
		CTag permtag(FT_PERMISSIONS, GetPermissions());
		permtag.WriteTagToFile(file);
		uTagCount++;
	}
	//MOPRH END   - Added by SiRoB, Show Permissions

	//EastShare START - Added by TAHO, .met file control
	CTag lastUsedTag(FT_LASTUSED, ( theApp.sharedfiles->GetFileByID(GetFileHash())) ? time(NULL) : statistic.GetLastUsed());
	lastUsedTag.WriteTagToFile(file);
	uTagCount++;
	//EastShare END - Added by TAHO, .met file control

	//Morph Start - Added by AndCycle, Equal Chance For Each File
	CTag sharedTime(FT_ATSHARED, statistic.GetSharedTime());
	sharedTime.WriteTagToFile(file);
	uTagCount++;
	//Morph End - Added by AndCycle, Equal Chance For Each File

	//MORPH START - Added by SiRoB, HIDEOS
	if (GetHideOS()>=0){
		CTag hideostag(FT_HIDEOS, GetHideOS());
		hideostag.WriteTagToFile(file);
		uTagCount++;
	}
	if (GetSelectiveChunk()>=0){
		CTag selectivechunktag(FT_SELECTIVE_CHUNK, GetSelectiveChunk());
		selectivechunktag.WriteTagToFile(file);
		uTagCount++;
	}
	//MORPH END   - Added by SiRoB, HIDEOS

	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	if (GetShareOnlyTheNeed()>=0){
		CTag shareonlytheneedtag(FT_SHAREONLYTHENEED, GetShareOnlyTheNeed());
		shareonlytheneedtag.WriteTagToFile(file);
		uTagCount++;
	}
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea

	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	if (GetPowerSharedMode()>=0){
		CTag powersharetag(FT_POWERSHARE, GetPowerSharedMode());
		powersharetag.WriteTagToFile(file);
		uTagCount++;
	}
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	if (GetPowerShareLimit()>=0){
		CTag powersharelimittag(FT_POWERSHARE_LIMIT, GetPowerShareLimit());
		powersharelimittag.WriteTagToFile(file);
		uTagCount++;
	}
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	//MORPH START - Added by IceCream, SLUGFILLER: Spreadbars
	char namebuffer[10];
	char* number = &namebuffer[1];
	uint16 i_pos = 0;
	for (POSITION pos = statistic.spreadlist.GetHeadPosition(); pos; ){
		uint32 count = statistic.spreadlist.GetValueAt(pos);
		if (!count) {
			statistic.spreadlist.GetNext(pos);
			continue;
		}
		uint32 start = statistic.spreadlist.GetKeyAt(pos);
		statistic.spreadlist.GetNext(pos);
		ASSERT(pos != NULL);	// Last value should always be 0
		uint32 end = statistic.spreadlist.GetKeyAt(pos);
		itoa(i_pos,number,10);
		namebuffer[0] = FT_SPREADSTART;
		CTag(namebuffer,start).WriteTagToFile(file);
		namebuffer[0] = FT_SPREADEND;
		CTag(namebuffer,end).WriteTagToFile(file);
		namebuffer[0] = FT_SPREADCOUNT;
		CTag(namebuffer,count).WriteTagToFile(file);
		uTagCount+=3;
		i_pos++;
	}
	//MORPH END   - Added by IceCream, SLUGFILLER: Spreadbars

	//other tags
	for (int j = 0; j < taglist.GetCount(); j++){
		if (taglist[j]->IsStr() || taglist[j]->IsInt()){
			taglist[j]->WriteTagToFile(file);
			uTagCount++;
		}
	}

	file->Seek(uTagCountFilePos, CFile::begin);
	file->WriteUInt32(uTagCount);
	file->Seek(0, CFile::end);

	return true;
}

void CKnownFile::CreateHashFromInput(FILE* file,CFile* file2, int Length, uchar* Output, uchar* in_string, CAICHHashTree* pShaHashOut) const
{
	ASSERT( Output != NULL || pShaHashOut != NULL);
	// time critial
	bool PaddingStarted = false;
	uint32 Hash[4];
	Hash[0] = 0x67452301;
	Hash[1] = 0xEFCDAB89;
	Hash[2] = 0x98BADCFE;
	Hash[3] = 0x10325476;
	CFile* data = NULL;
	if (in_string)
		data = new CMemFile(in_string,Length);
	uint32 Required = Length;
	uchar   X[64*128];  

	uint32	posCurrentEMBlock = 0;
	uint32	nIACHPos = 0;
	CAICHHashAlgo* pHashAlg = m_pAICHHashSet->GetNewHashAlgo();

	while (Required >= 64){
        uint32 len = Required / 64; 
        if (len > sizeof(X)/(64 * sizeof(X[0]))) 
             len = sizeof(X)/(64 * sizeof(X[0])); 
		if (in_string)
			data->Read(&X,len*64);
		else if (file)
            fread(&X,len*64,1,file); 
		else if (file2)
			file2->Read(&X,len*64);

		// SHA hash needs 180KB blocks
		if (pShaHashOut != NULL){
			if (nIACHPos + len*64 >= EMBLOCKSIZE){
				uint32 nToComplete = EMBLOCKSIZE - nIACHPos;
				pHashAlg->Add(X, nToComplete);
				ASSERT( nIACHPos + nToComplete == EMBLOCKSIZE );
				pShaHashOut->SetBlockHash(EMBLOCKSIZE, posCurrentEMBlock, pHashAlg);
				posCurrentEMBlock += EMBLOCKSIZE;
				pHashAlg->Reset();
				pHashAlg->Add(X+nToComplete,(len*64) - nToComplete);
				nIACHPos = (len*64) - nToComplete;
			}
			else{
				pHashAlg->Add(X, len*64);
				nIACHPos += len*64;
			}
		}

		if (Output != NULL){
			for (uint32 i = 0; i < len; i++) 
        	{ 
        	   MD4Transform(Hash, (uint32*)(X + i*64)); 
        	}
		}
		Required -= len*64;
	}
	// bytes to read
	Required = Length % 64;
	if (Required != 0){
		if (in_string)
			data->Read(&X,Required);
		else if (file)
			fread(&X,Required,1,file);
		else if (file2)
			file2->Read(&X,Required);

		if (pShaHashOut != NULL){
			if (nIACHPos + Required >= EMBLOCKSIZE){
				uint32 nToComplete = EMBLOCKSIZE - nIACHPos;
				pHashAlg->Add(X, nToComplete);
				ASSERT( nIACHPos + nToComplete == EMBLOCKSIZE );
				pShaHashOut->SetBlockHash(EMBLOCKSIZE, posCurrentEMBlock, pHashAlg);
				posCurrentEMBlock += EMBLOCKSIZE;
				pHashAlg->Reset();
				pHashAlg->Add(X+nToComplete, Required - nToComplete);
				nIACHPos = Required - nToComplete;
			}
			else{
				pHashAlg->Add(X, Required);
				nIACHPos += Required;
			}

		}
	}
	if (pShaHashOut != NULL){
		if(nIACHPos > 0){
			pShaHashOut->SetBlockHash(nIACHPos, posCurrentEMBlock, pHashAlg);
			posCurrentEMBlock += nIACHPos;
		}
		ASSERT( posCurrentEMBlock == Length );
		VERIFY( pShaHashOut->ReCalculateHash(pHashAlg, false) );
	}

	if (Output != NULL){
	// in byte scale 512 = 64, 448 = 56
	if (Required >= 56){
		X[Required] = 0x80;
		PaddingStarted = TRUE;
		memset(&X[Required + 1], 0, 63 - Required);
		MD4Transform(Hash, (uint32*)X);
		Required = 0;
	}
	if (!PaddingStarted)
		X[Required++] = 0x80;
		memset(&X[Required], 0, 64 - Required);
		// add size (convert to bits)
		uint32 Length2 = Length >> 29;
		Length <<= 3;
		memcpy(&X[56], &Length, 4);
		memcpy(&X[60], &Length2, 4);
		MD4Transform(Hash, (uint32*)X);
		md4cpy(Output, Hash);
	}

	delete pHashAlg;
	if (data)
		delete data;
}

uchar* CKnownFile::GetPartHash(uint16 part) const {
	if (part >= hashlist.GetCount())
		return 0;
	return hashlist[part];
}

static void MD4Transform(uint32 Hash[4], uint32 x[16])
{
  uint32 a = Hash[0];
  uint32 b = Hash[1];
  uint32 c = Hash[2];
  uint32 d = Hash[3];

  /* Round 1 */
  MD4_FF(a, b, c, d, x[ 0], S11); // 01
  MD4_FF(d, a, b, c, x[ 1], S12); // 02
  MD4_FF(c, d, a, b, x[ 2], S13); // 03
  MD4_FF(b, c, d, a, x[ 3], S14); // 04
  MD4_FF(a, b, c, d, x[ 4], S11); // 05
  MD4_FF(d, a, b, c, x[ 5], S12); // 06
  MD4_FF(c, d, a, b, x[ 6], S13); // 07
  MD4_FF(b, c, d, a, x[ 7], S14); // 08
  MD4_FF(a, b, c, d, x[ 8], S11); // 09
  MD4_FF(d, a, b, c, x[ 9], S12); // 10
  MD4_FF(c, d, a, b, x[10], S13); // 11
  MD4_FF(b, c, d, a, x[11], S14); // 12
  MD4_FF(a, b, c, d, x[12], S11); // 13
  MD4_FF(d, a, b, c, x[13], S12); // 14
  MD4_FF(c, d, a, b, x[14], S13); // 15
  MD4_FF(b, c, d, a, x[15], S14); // 16

  /* Round 2 */
  MD4_GG(a, b, c, d, x[ 0], S21); // 17
  MD4_GG(d, a, b, c, x[ 4], S22); // 18
  MD4_GG(c, d, a, b, x[ 8], S23); // 19
  MD4_GG(b, c, d, a, x[12], S24); // 20
  MD4_GG(a, b, c, d, x[ 1], S21); // 21
  MD4_GG(d, a, b, c, x[ 5], S22); // 22
  MD4_GG(c, d, a, b, x[ 9], S23); // 23
  MD4_GG(b, c, d, a, x[13], S24); // 24
  MD4_GG(a, b, c, d, x[ 2], S21); // 25
  MD4_GG(d, a, b, c, x[ 6], S22); // 26
  MD4_GG(c, d, a, b, x[10], S23); // 27
  MD4_GG(b, c, d, a, x[14], S24); // 28
  MD4_GG(a, b, c, d, x[ 3], S21); // 29
  MD4_GG(d, a, b, c, x[ 7], S22); // 30
  MD4_GG(c, d, a, b, x[11], S23); // 31
  MD4_GG(b, c, d, a, x[15], S24); // 32

  /* Round 3 */
  MD4_HH(a, b, c, d, x[ 0], S31); // 33
  MD4_HH(d, a, b, c, x[ 8], S32); // 34
  MD4_HH(c, d, a, b, x[ 4], S33); // 35
  MD4_HH(b, c, d, a, x[12], S34); // 36
  MD4_HH(a, b, c, d, x[ 2], S31); // 37
  MD4_HH(d, a, b, c, x[10], S32); // 38
  MD4_HH(c, d, a, b, x[ 6], S33); // 39
  MD4_HH(b, c, d, a, x[14], S34); // 40
  MD4_HH(a, b, c, d, x[ 1], S31); // 41
  MD4_HH(d, a, b, c, x[ 9], S32); // 42
  MD4_HH(c, d, a, b, x[ 5], S33); // 43
  MD4_HH(b, c, d, a, x[13], S34); // 44
  MD4_HH(a, b, c, d, x[ 3], S31); // 45
  MD4_HH(d, a, b, c, x[11], S32); // 46
  MD4_HH(c, d, a, b, x[ 7], S33); // 47
  MD4_HH(b, c, d, a, x[15], S34); // 48

  Hash[0] += a;
  Hash[1] += b;
  Hash[2] += c;
  Hash[3] += d;
}

void CAbstractFile::SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars, bool bAutoSetFileType)
{ 
	m_strFileName = pszFileName;
	if (bReplaceInvalidFileSystemChars){
		m_strFileName.Replace(_T('/'), _T('-'));
		m_strFileName.Replace(_T('>'), _T('-'));
		m_strFileName.Replace(_T('<'), _T('-'));
		m_strFileName.Replace(_T('*'), _T('-'));
		m_strFileName.Replace(_T(':'), _T('-'));
		m_strFileName.Replace(_T('?'), _T('-'));
		m_strFileName.Replace(_T('\"'), _T('-'));
		m_strFileName.Replace(_T('\\'), _T('-'));
		m_strFileName.Replace(_T('|'), _T('-'));
	}
	if (bAutoSetFileType)
		SetFileType(GetFileTypeByName(m_strFileName));
} 
      
void CAbstractFile::SetFileType(LPCTSTR pszFileType)
{ 
	m_strFileType = pszFileType;
} 

CString CAbstractFile::GetFileTypeDisplayStr() const
{
	CString strFileTypeDisplayStr(GetFileTypeDisplayStrFromED2KFileType(GetFileType()));
	if (strFileTypeDisplayStr.IsEmpty())
		strFileTypeDisplayStr = GetFileType();
	return strFileTypeDisplayStr;
}


void CAbstractFile::SetFileHash(const uchar* pucFileHash)
{
	md4cpy(m_abyFileHash, pucFileHash);
}

bool CAbstractFile::HasNullHash() const
{
	return isnulmd4(m_abyFileHash);
}

uint32 CAbstractFile::GetIntTagValue(uint8 tagname) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		const CTag* pTag = taglist[i];
		if (pTag->GetNameID()==tagname && pTag->IsInt())
			return pTag->GetInt();
	}
	return NULL;
}

bool CAbstractFile::GetIntTagValue(uint8 tagname, uint32& ruValue) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		const CTag* pTag = taglist[i];
		if (pTag->GetNameID()==tagname && pTag->IsInt()){
			ruValue = pTag->GetInt();
			return true;
		}
	}
	return false;
}

uint32 CAbstractFile::GetIntTagValue(LPCSTR tagname) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		const CTag* pTag = taglist[i];
		if (pTag->GetNameID()==0 && pTag->IsInt() && CmpED2KTagName(pTag->GetName(), tagname)==0)
			return pTag->GetInt();
	}
	return NULL;
}

const CString& CAbstractFile::GetStrTagValue(uint8 tagname) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		const CTag* pTag = taglist[i];
		if (pTag->GetNameID()==tagname && pTag->IsStr())
			return pTag->GetStr();
	}
	static const CString _strEmpty;
	return _strEmpty;
}

const CString& CAbstractFile::GetStrTagValue(LPCSTR tagname) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		const CTag* pTag = taglist[i];
		if (pTag->GetNameID()==0 && pTag->IsStr() && CmpED2KTagName(pTag->GetName(), tagname)==0)
			return pTag->GetStr();
	}
	static const CString _strEmpty;
	return _strEmpty;
}

CTag* CAbstractFile::GetTag(uint8 tagname, uint8 tagtype) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		CTag* pTag = taglist[i];
		if (pTag->GetNameID()==tagname && pTag->GetType()==tagtype)
			return pTag;
	}
	return NULL;
}

CTag* CAbstractFile::GetTag(LPCSTR tagname, uint8 tagtype) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		CTag* pTag = taglist[i];
		if (pTag->GetNameID()==0 && pTag->GetType()==tagtype && CmpED2KTagName(pTag->GetName(), tagname)==0)
			return pTag;
	}
	return NULL;
}

CTag* CAbstractFile::GetTag(uint8 tagname) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		CTag* pTag = taglist[i];
		if (pTag->GetNameID()==tagname)
			return pTag;
	}
	return NULL;
}

CTag* CAbstractFile::GetTag(LPCSTR tagname) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		CTag* pTag = taglist[i];
		if (pTag->GetNameID()==0 && CmpED2KTagName(pTag->GetName(), tagname)==0)
			return pTag;
	}
	return NULL;
}

void CAbstractFile::AddTagUnique(CTag* pTag)
{
	for (int i = 0; i < taglist.GetSize(); i++){
		const CTag* pCurTag = taglist[i];
		if ( (   (pCurTag->GetNameID()!=0 && pCurTag->GetNameID()==pTag->GetNameID())
			  || (pCurTag->GetName()!=NULL && pTag->GetName()!=NULL && CmpED2KTagName(pCurTag->GetName(), pTag->GetName())==0)
			 )
			 && pCurTag->GetType() == pTag->GetType()){
			delete pCurTag;
			taglist.SetAt(i, pTag);
			return;
		}
	}
	taglist.Add(pTag);
}

Packet*	CKnownFile::CreateSrcInfoPacket(CUpDownClient* forClient) const
{
	if(m_ClientUploadList.IsEmpty())
		return NULL;

	CSafeMemFile data(1024);
	uint16 nCount = 0;

	data.WriteHash16(forClient->GetUploadFileID());
	data.WriteUInt16(nCount);
	uint32 cDbgNoSrc = 0;
	for( POSITION pos = m_ClientUploadList.GetHeadPosition();pos != 0;)
	{
		const CUpDownClient *cur_src = m_ClientUploadList.GetNext(pos);
		if(cur_src->HasLowID() || cur_src == forClient || !(cur_src->GetUploadState() == US_UPLOADING || cur_src->GetUploadState() == US_ONUPLOADQUEUE))
			continue;
		if (!cur_src->IsEd2kClient())
			continue;

		bool bNeeded = false;
		uint8* rcvstatus = forClient->GetUpPartStatus();
		if( rcvstatus )
		{
			uint8* srcstatus = cur_src->GetUpPartStatus();
			if( srcstatus )
			{
				if( cur_src->GetUpPartCount() == forClient->GetUpPartCount() )
				{
					for (int x = 0; x < GetPartCount(); x++ )
					{
						if( srcstatus[x] && !rcvstatus[x] )
						{
							// We know the recieving client needs a chunk from this client.
							bNeeded = true;
							break;
						}
					}
				}
				else
				{
					// should never happen
					if (thePrefs.GetVerbose())
						DEBUG_ONLY(AddDebugLogLine(false,_T("*** %hs - found source (%s) with wrong partcount (%u) attached to file \"%s\" (partcount=%u)"), __FUNCTION__, cur_src->DbgGetClientInfo(), cur_src->GetUpPartCount(), GetFileName(), GetPartCount()));
				}
			}
			else
			{
				cDbgNoSrc++;
				// This client doesn't support upload chunk status. So just send it and hope for the best.
				bNeeded = true;
			}
		}
		else
		{
			TRACE(_T("CKnownFile::CreateSrcInfoPacket, requesting client has no chunk status - %s"), forClient->DbgGetClientInfo());
			// remote client does not support upload chunk status, search sources which have at least one complete part
			// we could even sort the list of sources by available chunks to return as much sources as possible which
			// have the most available chunks. but this could be a noticeable performance problem.
			uint8* srcstatus = cur_src->GetUpPartStatus();
			if( srcstatus )
			{
				for (int x = 0; x < GetPartCount(); x++ )
				{
					if( srcstatus[x] )
					{
						// this client has at least one chunk
						bNeeded = true;
						break;
					}
				}
			}
			else
			{
				// This client doesn't support upload chunk status. So just send it and hope for the best.
				bNeeded = true;
			}
		}

		if( bNeeded )
		{
			nCount++;
			uint32 dwID;
			if(forClient->GetSourceExchangeVersion() > 2)
				dwID = cur_src->GetUserIDHybrid();
			else
				dwID = cur_src->GetIP();
		    data.WriteUInt32(dwID);
		    data.WriteUInt16(cur_src->GetUserPort());
		    data.WriteUInt32(cur_src->GetServerIP());
		    data.WriteUInt16(cur_src->GetServerPort());
			if (forClient->GetSourceExchangeVersion() > 1)
			    data.WriteHash16(cur_src->GetUserHash());
			if (nCount > 500)
				break;
		}
	}
	TRACE(_T("CKnownFile::CreateSrcInfoPacket: Out of %u clients, %u had no valid chunk status\n\r"),m_ClientUploadList.GetCount(), cDbgNoSrc);
	if (!nCount)
		return 0;
	data.Seek(16,0);
	data.WriteUInt16(nCount);

	Packet* result = new Packet(&data, OP_EMULEPROT);
	result->opcode = OP_ANSWERSOURCES;
	// 16+2+501*(4+2+4+2+16) = 14046 bytes max.
	if ( result->size > 354 )
		result->PackPacket();
	if (thePrefs.GetDebugSourceExchange())
		AddDebugLogLine(false, _T("SXSend: Client source response; Count=%u, %s, File=\"%s\""), nCount, forClient->DbgGetClientInfo(), GetFileName());
	return result;
}

void CKnownFile::LoadComment()
{
	CIni ini(thePrefs.GetFileCommentsFilePath(), md4str(GetFileHash()));
	m_strComment = ini.GetStringUTF8(_T("Comment")).Left(MAXFILECOMMENTLEN);
	m_uRating = ini.GetInt(_T("Rate"), 0);
	m_bCommentLoaded=true;
}    

const CString& CKnownFile::GetFileComment() /*const*/
{
	if (!m_bCommentLoaded)
		LoadComment();
	return m_strComment;
}

void CKnownFile::SetFileComment(LPCTSTR pszComment)
{
	if (m_strComment.Compare(pszComment) != 0)
	{
		CIni ini(thePrefs.GetFileCommentsFilePath(), md4str(GetFileHash()));
		ini.WriteStringUTF8(_T("Comment"), pszComment);
		m_strComment = pszComment;
   
		for (POSITION pos = m_ClientUploadList.GetHeadPosition();pos != 0;)
			m_ClientUploadList.GetNext(pos)->SetCommentDirty();
	}
}

uint8 CKnownFile::GetFileRating() /*const*/
{
	if (!m_bCommentLoaded)
		LoadComment();
	return m_uRating;
}
// For File rate 
void CKnownFile::SetFileRating(uint8 uRating)
{
	if (m_uRating != uRating)
	{
		CIni ini(thePrefs.GetFileCommentsFilePath(), md4str(GetFileHash()));
		ini.WriteInt(_T("Rate"), uRating);
		m_uRating = uRating;

		for (POSITION pos = m_ClientUploadList.GetHeadPosition();pos != 0;)
			m_ClientUploadList.GetNext(pos)->SetCommentDirty();
	}
}

void CKnownFile::UpdateAutoUpPriority(){
	if( !IsAutoUpPriority() )
		return;
	if ( GetQueuedCount() > 20 ){
		if( GetUpPriority() != PR_LOW ){
			SetUpPriority( PR_LOW );
			theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
		}
		return;
	}
	if ( GetQueuedCount() > 1 ){
		if( GetUpPriority() != PR_NORMAL ){
			SetUpPriority( PR_NORMAL );
			theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
		}
		return;
	}
	if( GetUpPriority() != PR_HIGH){
		SetUpPriority( PR_HIGH );
		theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
	}
}

void CKnownFile::SetUpPriority(uint8 iNewUpPriority, bool bSave)
{
	m_iUpPriority = iNewUpPriority;
	ASSERT( m_iUpPriority == PR_VERYLOW || m_iUpPriority == PR_LOW || m_iUpPriority == PR_NORMAL || m_iUpPriority == PR_HIGH || m_iUpPriority == PR_VERYHIGH );

	if( IsPartFile() && bSave )
		((CPartFile*)this)->SavePartFile();
}

//MOPRH - Added by SiRoB, Keep Permission flag
void CKnownFile::SetPermissions(int iNewPermissions)
{
	// Mighty Knife: Community visible filelist
	ASSERT( m_iPermissions == PERM_ALL || m_iPermissions == PERM_FRIENDS || m_iPermissions == PERM_NOONE || m_iPermissions == PERM_COMMUNITY);
	// [end] Mighty Knife
	m_iPermissions = iNewPermissions;
}

void SecToTimeLength(unsigned long ulSec, CStringA& rstrTimeLength)
{
	// this function creates the content for the "length" ed2k meta tag which was introduced by eDonkeyHybrid 
	// with the data type 'string' :/  to save some bytes we do not format the duration with leading zeros
	if (ulSec >= 3600){
		UINT uHours = ulSec/3600;
		UINT uMin = (ulSec - uHours*3600)/60;
		UINT uSec = ulSec - uHours*3600 - uMin*60;
		rstrTimeLength.Format("%u:%02u:%02u", uHours, uMin, uSec);
	}
	else{
		UINT uMin = ulSec/60;
		UINT uSec = ulSec - uMin*60;
		rstrTimeLength.Format("%u:%02u", uMin, uSec);
	}
}

void SecToTimeLength(unsigned long ulSec, CStringW& rstrTimeLength)
{
	// this function creates the content for the "length" ed2k meta tag which was introduced by eDonkeyHybrid 
	// with the data type 'string' :/  to save some bytes we do not format the duration with leading zeros
	if (ulSec >= 3600){
		UINT uHours = ulSec/3600;
		UINT uMin = (ulSec - uHours*3600)/60;
		UINT uSec = ulSec - uHours*3600 - uMin*60;
		rstrTimeLength.Format(L"%u:%02u:%02u", uHours, uMin, uSec);
	}
	else{
		UINT uMin = ulSec/60;
		UINT uSec = ulSec - uMin*60;
		rstrTimeLength.Format(L"%u:%02u", uMin, uSec);
	}
}

void CKnownFile::RemoveMetaDataTags()
{
	static const struct
	{
		uint8	nID;
		uint8	nType;
	} _aEmuleMetaTags[] = 
	{
		{ FT_MEDIA_ARTIST,  2 },
		{ FT_MEDIA_ALBUM,   2 },
		{ FT_MEDIA_TITLE,   2 },
		{ FT_MEDIA_LENGTH,  3 },
		{ FT_MEDIA_BITRATE, 3 },
		{ FT_MEDIA_CODEC,   2 }
	};

	// 05-Jän-2004 [bc]: ed2k and Kad are already full of totally wrong and/or not properly attached meta data. Take
	// the chance to clean any available meta data tags and provide only tags which were determined by us.
	// Remove all meta tags. Never ever trust the meta tags received from other clients or servers.
	for (int j = 0; j < ARRSIZE(_aEmuleMetaTags); j++)
	{
		int i = 0;
		while (i < taglist.GetSize())
		{
			const CTag* pTag = taglist[i];
			if (pTag->GetNameID() == _aEmuleMetaTags[j].nID)
			{
				delete pTag;
				taglist.RemoveAt(i);
			}
			else
				i++;
		}
	}

	m_uMetaDataVer = 0;
}

void CKnownFile::UpdateMetaDataTags()
{
	// 05-Jän-2004 [bc]: ed2k and Kad are already full of totally wrong and/or not properly attached meta data. Take
	// the chance to clean any available meta data tags and provide only tags which were determined by us.
	RemoveMetaDataTags();

	if (thePrefs.GetExtractMetaData() == 0)
		return;

	TCHAR szExt[_MAX_EXT];
	_tsplitpath(GetFileName(), NULL, NULL, NULL, szExt);
	_tcslwr(szExt);
	if (_tcscmp(szExt, _T(".mp3"))==0 || _tcscmp(szExt, _T(".mp2"))==0 || _tcscmp(szExt, _T(".mp1"))==0 || _tcscmp(szExt, _T(".mpa"))==0)
	{
		TCHAR szFullPath[MAX_PATH];
		_tmakepath(szFullPath, NULL, GetPath(), GetFileName(), NULL);

		try{
			USES_CONVERSION;
			ID3_Tag myTag;
			myTag.Link(T2A(szFullPath));

			const Mp3_Headerinfo* mp3info;
			mp3info = myTag.GetMp3HeaderInfo();
			if (mp3info)
			{
				// length
				if (mp3info->time){
					CTag* pTag = new CTag(FT_MEDIA_LENGTH, (uint32)mp3info->time);
					AddTagUnique(pTag);
					m_uMetaDataVer = META_DATA_VER;
				}

				// here we could also create a "codec" ed2k meta tag.. though it would probable not be worth the
				// extra bytes which would have to be sent to the servers..

				// bitrate
				UINT uBitrate = mp3info->bitrate/1000;
				if (uBitrate){
					CTag* pTag = new CTag(FT_MEDIA_BITRATE, (uint32)uBitrate);
					AddTagUnique(pTag);
					m_uMetaDataVer = META_DATA_VER;
				}
			}

			ID3_Tag::Iterator* iter = myTag.CreateIterator();
			const ID3_Frame* frame;
			while ((frame = iter->GetNext()) != NULL)
			{
				ID3_FrameID eFrameID = frame->GetID();
				switch (eFrameID)
				{
					case ID3FID_LEADARTIST:{
						char* pszText = ID3_GetString(frame, ID3FN_TEXT);
						CString strText(pszText);
						strText.Trim();
						if (!strText.IsEmpty()){
							CTag* pTag = new CTag(FT_MEDIA_ARTIST, strText);
							AddTagUnique(pTag);
							m_uMetaDataVer = META_DATA_VER;
						}
						delete[] pszText;
						break;
					}
					case ID3FID_ALBUM:{
						char* pszText = ID3_GetString(frame, ID3FN_TEXT);
						CString strText(pszText);
						strText.Trim();
						if (!strText.IsEmpty()){
							CTag* pTag = new CTag(FT_MEDIA_ALBUM, strText);
							AddTagUnique(pTag);
							m_uMetaDataVer = META_DATA_VER;
						}
						delete[] pszText;
						break;
					}
					case ID3FID_TITLE:{
						char* pszText = ID3_GetString(frame, ID3FN_TEXT);
						CString strText(pszText);
						strText.Trim();
						if (!strText.IsEmpty()){
							CTag* pTag = new CTag(FT_MEDIA_TITLE, strText);
							AddTagUnique(pTag);
							m_uMetaDataVer = META_DATA_VER;
						}
						delete[] pszText;
						break;
					}
				}
			}
			delete iter;
		}
		catch(...){
			if (thePrefs.GetVerbose())
				AddDebugLogLine(false, _T("Unhandled exception while extracting file meta (MP3) data from \"%s\""), szFullPath);
			ASSERT(0);
		}
	}
	else if (thePrefs.GetExtractMetaData() > 1)
	{
		// starting the MediaDet object takes a noticeable amount of time.. avoid starting that object
		// for files which are not expected to contain any Audio/Video data.
		// note also: MediaDet does not work well for too short files (e.g. 16K)
		EED2KFileType eFileType = GetED2KFileTypeID(GetFileName());
		if ((eFileType == ED2KFT_AUDIO || eFileType == ED2KFT_VIDEO) && GetFileSize() >= 32768)
		{
			// Avoid processing of some file types which are known to crash due to bugged DirectShow filters.
			TCHAR szExt[_MAX_EXT];
			_tsplitpath(GetFileName(), NULL, NULL, NULL, szExt);
			_tcslwr(szExt);
			if (_tcscmp(szExt, _T(".ogm"))!=0 && _tcscmp(szExt, _T(".ogg"))!=0 && _tcscmp(szExt, _T(".mkv"))!=0)
			{
TCHAR szFullPath[MAX_PATH];
				_tmakepath(szFullPath, NULL, GetPath(), GetFileName(), NULL);
				try{
					CComPtr<IMediaDet> pMediaDet;
					HRESULT hr = pMediaDet.CoCreateInstance(__uuidof(MediaDet));
					if (SUCCEEDED(hr))
					{
						USES_CONVERSION;
						if (SUCCEEDED(hr = pMediaDet->put_Filename(CComBSTR(T2W(szFullPath)))))
						{
							// Get the first audio/video streams
							long lAudioStream = -1;
							long lVideoStream = -1;
							double fVideoStreamLengthSec = 0.0;
							DWORD dwVideoBitRate = 0;
							DWORD dwVideoCodec = 0;
							double fAudioStreamLengthSec = 0.0;
							DWORD dwAudioBitRate = 0;
							//DWORD dwAudioCodec = 0;
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
												if (lVideoStream == -1){
													lVideoStream = i;
													pMediaDet->get_StreamLength(&fVideoStreamLengthSec);

													AM_MEDIA_TYPE mt = {0};
													if (SUCCEEDED(hr = pMediaDet->get_StreamMediaType(&mt))){
														if (mt.formattype == FORMAT_VideoInfo){
															VIDEOINFOHEADER* pVIH = (VIDEOINFOHEADER*)mt.pbFormat;
															// do not use that 'dwBitRate', whatever this number is, it's not
															// the bitrate of the encoded video stream. seems to be the bitrate
															// of the uncompressed stream divided by 2 !??
															//dwVideoBitRate = pVIH->dwBitRate / 1000;

															// for AVI files this gives that used codec
															// for MPEG(1) files this just gives "Y41P"
															dwVideoCodec = pVIH->bmiHeader.biCompression;
														}
													}

													if (mt.pUnk != NULL)
														mt.pUnk->Release();
													if (mt.pbFormat != NULL)
														CoTaskMemFree(mt.pbFormat);
												}
											}
											else if (major_type == MEDIATYPE_Audio)
											{
												if (lAudioStream == -1){
													lAudioStream = i;
													pMediaDet->get_StreamLength(&fAudioStreamLengthSec);

													AM_MEDIA_TYPE mt = {0};
													if (SUCCEEDED(hr = pMediaDet->get_StreamMediaType(&mt))){
														if (mt.formattype == FORMAT_WaveFormatEx){
															WAVEFORMATEX* wfx = (WAVEFORMATEX*)mt.pbFormat;
															dwAudioBitRate = ((wfx->nAvgBytesPerSec * 8.0) + 500.0) / 1000.0;
														}
													}

													if (mt.pUnk != NULL)
														mt.pUnk->Release();
													if (mt.pbFormat != NULL)
														CoTaskMemFree(mt.pbFormat);
												}
											}
											else{
												TRACE("%s - Unknown stream type\n", GetFileName());
											}

											if (lVideoStream != -1 && lAudioStream != -1)
												break;
										}
									}
								}
							}

							uint32 uLengthSec = 0.0;
							CStringA strCodec;
							uint32 uBitrate = 0;
							if (fVideoStreamLengthSec > 0.0){
								uLengthSec = fVideoStreamLengthSec;
								if (dwVideoCodec == BI_RGB)
									strCodec = "rgb";
								else if (dwVideoCodec == BI_RLE8)
									strCodec = "rle8";
								else if (dwVideoCodec == BI_RLE4)
									strCodec = "rle4";
								else if (dwVideoCodec == BI_BITFIELDS)
									strCodec = "bitfields";
								else{
								memcpy(strCodec.GetBuffer(4), &dwVideoCodec, 4);
									strCodec.ReleaseBuffer(4);
									strCodec.MakeLower();
								}
								uBitrate = dwVideoBitRate;
							}
							else if (fAudioStreamLengthSec > 0.0){
								uLengthSec = fAudioStreamLengthSec;
								uBitrate = dwAudioBitRate;
							}

							if (uLengthSec){
								CTag* pTag = new CTag(FT_MEDIA_LENGTH, (uint32)uLengthSec);
								AddTagUnique(pTag);
								m_uMetaDataVer = META_DATA_VER;
							}

							if (!strCodec.IsEmpty()){
								CTag* pTag = new CTag(FT_MEDIA_CODEC, CString(strCodec));
								AddTagUnique(pTag);
								m_uMetaDataVer = META_DATA_VER;
							}

							if (uBitrate){
								CTag* pTag = new CTag(FT_MEDIA_BITRATE, (uint32)uBitrate);
								AddTagUnique(pTag);
								m_uMetaDataVer = META_DATA_VER;
							}
						}
					}
				}
				catch(...){
					if (thePrefs.GetVerbose())
						AddDebugLogLine(false, _T("Unhandled exception while extracting meta data (MediaDet) from \"%s\""), szFullPath);
					ASSERT(0);
				}
			}
		}
	}
}

void CKnownFile::SetPublishedED2K(bool val){
	m_PublishedED2K = val;
	theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
}

void CKnownFile::SetPublishedKadSrc(){
	m_PublishedKadSrc++;
	theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
}

bool CKnownFile::PublishSrc()
{
	if( m_lastPublishTimeKadSrc > 0)
	{
		if( ((uint32)time(NULL)-m_lastPublishTimeKadSrc) < KADEMLIAREPUBLISHTIMES)
		{
			return false;
		}
	}
	m_lastPublishTimeKadSrc = (uint32)time(NULL);
	return true;
}

bool CKnownFile::IsMovie() const
{
	return (ED2KFT_VIDEO == GetED2KFileTypeID(GetFileName()) );
}
//MORPH START - Added by IceCream, music preview and defeat 0-filler
bool CKnownFile::IsMusic() const
{
	return (ED2KFT_AUDIO == GetED2KFileTypeID(GetFileName()) );
}

bool CKnownFile::IsCDImage() const
{
	return (ED2KFT_CDIMAGE == GetED2KFileTypeID(GetFileName()) );
}

bool CKnownFile::IsDocument() const
{
	return (ED2KFT_DOCUMENT == GetED2KFileTypeID(GetFileName()) );
}
//MORPH END   - Added by IceCream, music preview and defeat 0-filler

// function assumes that this file is shared and that any needed permission to preview exists. checks have to be done before calling! 
bool CKnownFile::GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender)
{
	return GrabImage(GetPath() + CString("\\") + GetFileName(), nFramesToGrab,  dStartTime, bReduceColor, nMaxWidth, pSender);
}

bool CKnownFile::GrabImage(CString strFileName,uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender)
{
	if (!IsMovie())
		return false;
	CFrameGrabThread* framegrabthread = (CFrameGrabThread*) AfxBeginThread(RUNTIME_CLASS(CFrameGrabThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
	framegrabthread->SetValues(this,strFileName,nFramesToGrab,  dStartTime, bReduceColor, nMaxWidth, pSender);
	framegrabthread->ResumeThread();
	return true;
}

// imgResults[i] can be NULL
void CKnownFile::GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender)
{
	// continue processing
	if (pSender == theApp.mmserver){
		theApp.mmserver->PreviewFinished(imgResults, nFramesGrabbed);
	}
	else if (theApp.clientlist->IsValidClient((CUpDownClient*)pSender)){
		((CUpDownClient*)pSender)->SendPreviewAnswer(this, imgResults, nFramesGrabbed);
	}
	else{
		//probably a client which got deleted while grabbing the frames for some reason
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Couldn't find Sender of FrameGrabbing Request"));
	}
	//cleanup
	for (int i = 0; i != nFramesGrabbed; i++){
		if (imgResults[i] != NULL)
			delete imgResults[i];
	}
	delete[] imgResults;

}

void CKnownFile::SetPowerShared(int newValue) {
    int oldValue = m_powershared;
    m_powershared = newValue;
    if(theApp.uploadqueue && oldValue != newValue)
        theApp.uploadqueue->ReSortUploadSlots(true);
}

/*// #zegzav:updcliuplst
void CKnownFile::UpdateClientUploadList()
{
	// remove non-existent clients / add missing clients
	theApp.clientlist->GetClientListByFileID(&m_ClientUploadList, GetFileHash());
	m_iQueuedCount = m_ClientUploadList.GetCount();
	UpdateAutoUpPriority();
}
*/
// SLUGFILLER: hideOS
uint16 CKnownFile::CalcPartSpread(CArray<uint32, uint32>& partspread, CUpDownClient* client)
{
	UINT parts = GetED2KPartCount();
	UINT realparts = GetPartCount();
	uint32 min;
	UINT mincount;
	UINT i;

	ASSERT(client != NULL);

	partspread.RemoveAll();

	if (!parts)
		return 0;

	CArray<bool, bool> partsavail;
	bool usepartsavail = false;

	partspread.SetSize(parts);
	partsavail.SetSize(parts);
	for (i = 0; i < parts; i++) {
		partspread[i] = 0;
		partsavail[i] = true;
	}

	if(statistic.spreadlist.IsEmpty())
		return parts;

	if (IsPartFile())
		for (i = 0; i < realparts; i++)
			if (!((CPartFile*)this)->IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){
				partsavail[i] = false;
				usepartsavail = true;
			}
	if (client->m_abyUpPartStatus)
		for (i = 0; i < realparts; i++)
			if (client->IsUpPartAvailable(i)) {
				partsavail[i] = false;
				usepartsavail = true;
			}
	if (parts > realparts)
		partsavail[parts-1] = false;	// Couldn't care less if a 0-sized part wasn't spread

	POSITION pos = statistic.spreadlist.GetHeadPosition();
	uint16 last = 0;
	uint32 count = statistic.spreadlist.GetValueAt(pos);
	min = count;
	statistic.spreadlist.GetNext(pos);
	while (pos && last < parts){
		uint32 next = statistic.spreadlist.GetKeyAt(pos);
		if (next >= GetFileSize())
			break;
		next /= PARTSIZE;
		while (last < next) {
			partspread[last] = count;
			last++;
		}
		if (last >= parts || !(statistic.spreadlist.GetKeyAt(pos) % PARTSIZE)) {
			count = statistic.spreadlist.GetValueAt(pos);
			if (min > count)
				min = count;
			statistic.spreadlist.GetNext(pos);
			continue;
		}
		partspread[last] = count;
		while (next == last) {
			count = statistic.spreadlist.GetValueAt(pos);
			if (min > count)
				min = count;
			if (partspread[last] > count)
				partspread[last] = count;
			statistic.spreadlist.GetNext(pos);
			if (!pos)
				break;
			next = statistic.spreadlist.GetKeyAt(pos);
			if (next >= GetFileSize())
				break;
			next /= PARTSIZE;
		}
		last++;
	}
	while (last < parts) {
		partspread[last] = count;
		last++;
	}
	if (usepartsavail) {		// Special case, ignore unshareables for min calculation
		for (i = 0; i < parts; i++)
			if (partsavail[i]){
				min = partspread[i];
				break;
			}
		for (i++; i < parts; i++){
			if (partsavail[i])
				if (min > partspread[i])
					min = partspread[i];
		}
	}
	for (i = 0; i < parts; i++) {
		if (partspread[i] > min)
			partspread[i] -= min;
		else
			partspread[i] = 0;
	}

	if ((GetSelectiveChunk()>=0)?!GetSelectiveChunk():!thePrefs.IsSelectiveShareEnabled())
		return parts;

	uint8 hideOS = (GetHideOS()>=0)?GetHideOS():thePrefs.GetHideOvershares();
	ASSERT(hideOS != 0);

	bool resetSentCount = false;
	
	if (m_PartSentCount.GetSize() != partspread.GetSize())
		resetSentCount = true;
	else {
		min = hideOS;
		for (i = 0; i < parts; i++) {
			if (!partsavail[i])
				continue;
			if (m_PartSentCount[i] < partspread[i])
				m_PartSentCount[i] = partspread[i];
			if (m_PartSentCount[i] < min) {
				min = m_PartSentCount[i];
				mincount = 1;
			}
			else if (m_PartSentCount[i] == min)
				mincount++;
		}
		if (min >= hideOS)
			resetSentCount = true;
	}

	if (resetSentCount) {
		min = 0;
		mincount = 0;
		m_PartSentCount.SetSize(parts);
		for (i = 0; i < parts; i++){
			m_PartSentCount[i] = partspread[i];
			if (partsavail[i] && !partspread[i])
				mincount++;
		}
		if (!mincount)
		  return parts; // We're a no-needed source already
	}

	mincount = (rand() % mincount) + 1;
	for (i = 0; i < parts; i++) {
		if (!partsavail[i])
			continue;
		if (m_PartSentCount[i] > min)
			continue;
		ASSERT(m_PartSentCount[i] == min);
		mincount--;
		if (!mincount)
			break;
	}
	ASSERT(i < parts);
	m_PartSentCount[i]++;
	mincount = i;
	for (i = 0; i < mincount; i++)
		partspread[i] = hideOS;
	for (i = mincount+1; i < parts; i++)
		partspread[i] = hideOS;

	return parts;
};

bool CKnownFile::HideOvershares(CSafeMemFile* file, CUpDownClient* client){
	uint8 hideOS = (GetHideOS()>=0)?GetHideOS():thePrefs.GetHideOvershares();
	
	if (!hideOS)
		return FALSE;
	CArray<uint32, uint32> partspread;
	UINT parts = CalcPartSpread(partspread, client);
	if (!parts)
		return FALSE;
	uint32 max;
	max = partspread[0];
	for (UINT i = 1; i < parts; i++)
		if (partspread[i] > max)
			max = partspread[i];
	if (max < hideOS)
		return FALSE;
	//MORPH START - Added by SiRoB, See chunk that we hide
	bool revelatleastonechunk = false;
	if (client->m_abyUpPartStatusHidden){
		delete[] client->m_abyUpPartStatusHidden;
		client->m_abyUpPartStatusHidden = NULL;
	}
	client->m_abyUpPartStatusHidden = new uint8[parts];
	client->m_bUpPartStatusHiddenBySOTN = false;
	memset(client->m_abyUpPartStatusHidden,0,parts);
	for (UINT i = 0; i < parts; i++)
		if (partspread[i] < hideOS && client->IsPartAvailable(i)==false)
			revelatleastonechunk = true;
	if (revelatleastonechunk==false)
		return false;
	//MORPH END   - Added by SiRoB, See chunk that we hide

	file->WriteUInt16(parts);
	UINT done = 0;
	while (done != parts){
		uint8 towrite = 0;
		for (UINT i = 0;i < 8;i++){
			if (partspread[done] < hideOS)
				towrite |= (1<<i);
			//MORPH START - Added by SiRoB, See chunk that we hide
			else
				client->m_abyUpPartStatusHidden[done] = 1;
			//MORPH END   - Added by SiRoB, See chunk that we hide
			done++;
			if (done == parts)
				break;
		}
		file->WriteUInt8(towrite);
	}
	return TRUE;
}
// SLUGFILLER: hideOS

//MORPH - Changed by SiRoB, Avoid Sharing Nothing :( the return should be conditional
//Wistily : Share only the need START (Inspired by lovelace release feature, adapted from Slugfiller hideOS code)
bool CKnownFile::ShareOnlyTheNeed(CSafeMemFile* file, CUpDownClient* client)
{
	if (((GetShareOnlyTheNeed()>=0)?GetShareOnlyTheNeed():thePrefs.GetShareOnlyTheNeed())==0)
		return false;
	UINT parts = GetED2KPartCount();
	if (client->m_abyUpPartStatusHidden){
		delete[] client->m_abyUpPartStatusHidden;
		client->m_abyUpPartStatusHidden = NULL;
	}
	client->m_bUpPartStatusHiddenBySOTN = true;
	client->m_abyUpPartStatusHidden = new uint8[parts];
	memset(client->m_abyUpPartStatusHidden,0,parts);
	
	bool revelatleastonechunk = false;
	if (!m_AvailPartFrequency.IsEmpty())
		for (UINT i = 0; i < parts; i++)
			if (!client->IsPartAvailable(i) && m_AvailPartFrequency[i] <= 2)
				revelatleastonechunk = true;
	if (revelatleastonechunk==false)
		return false;
	UINT done = 0;
	file->WriteUInt16(parts);
	while (done != parts){
		uint8 towrite = 0;
		for (UINT i = 0;i < 8;i++){
			if (m_AvailPartFrequency[done] <= 2)
				towrite |= (1<<i);
			else
				client->m_abyUpPartStatusHidden[done] = 1;
			done++;
			if (done == parts)
				break;
		}
		file->WriteUInt8(towrite);
	}
	return true;
}
//Wistily : Share only the need STOP

//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
bool CKnownFile::GetPowerShared() const
{
	int temppowershared = (m_powershared>=0)?m_powershared:thePrefs.GetPowerShareMode();
	return ((temppowershared&1) || ((temppowershared == 2) && m_bPowerShareAuto)) && m_bPowerShareAuthorized && !((temppowershared == 3) && m_bPowerShareLimited);
}
//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
	