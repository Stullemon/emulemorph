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

#include "StdAfx.h"
#include "knownfile.h"
#include "opcodes.h"
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "emule.h"
#include "ini2.h"
#define NOMD4MACROS
#include "kademlia/utils/md4.h"
#include "QArray.h"
#include "framegrabthread.h"
#include "CxImage/xImage.h"

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
}

// SLUGFILLER: mergeKnown
void CFileStatistic::Merge(CFileStatistic* other){
	transferred += other->transferred;
	requested += other->requested;
	accepted += other->accepted;
	alltimetransferred += other->alltimetransferred;
	alltimerequested += other->alltimerequested;
	alltimeaccepted += other->alltimeaccepted;
	//MORPH START - Added by SiRoB, SLUGFILLER: Spreadbars
	if (!other->spreadlist.IsEmpty()) {
		POSITION pos = other->spreadlist.GetHeadPosition();
		uint32 start = other->spreadlist.GetKeyAt(pos);
		uint32 count = other->spreadlist.GetValueAt(pos);
		other->spreadlist.GetNext(pos);
		while (pos){
			uint32 end = other->spreadlist.GetKeyAt(pos);
			if (count)
				AddBlockTransferred(start, end, count);
			start = end;
			count = other->spreadlist.GetValueAt(pos);
			other->spreadlist.GetNext(pos);
		}
	}
	//MORPH END - Added by SiRoB, SLUGFILLER: Spreadbars
}
// SLUGFILLER: mergeKnown

CAbstractFile::CAbstractFile()
{
	md4clr(m_abyFileHash);
	m_nFileSize = 0;
	m_iRate = 0;
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
void CFileStatistic::DrawSpreadBar(CDC* dc, RECT* rect, bool bFlat){
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
float CFileStatistic::GetSpreadSortValue(){
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

float CFileStatistic::GetFullSpreadCount(){
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
	lastFullSpreadCount = min+next;
	return lastFullSpreadCount;
	//MORPH END   - Added by SiRoB, Reduce SpreadBar CPU consumption
}
//MORPH END   - Added by IceCream, SLUGFILLER: Spreadbars

CKnownFile::CKnownFile()
{
	m_iPartCount = 0;
	m_iED2KPartCount = 0;
	m_iED2KPartHashCount = 0;
	date = 0;
	dateC =0;
	if(theApp.glob_prefs->GetNewAutoUp()){
		m_iUpPriority = PR_HIGH;
		m_bAutoUpPriority = true;
	}
	else{
		m_iUpPriority = PR_NORMAL;
		m_bAutoUpPriority = false;
	}
	m_iQueuedCount = 0;
	m_iPermissions = PERM_ALL;
	statistic.fileParent = this;
	m_bCommentLoaded = false;
	(void)m_strComment;
	m_PublishedED2K = false;
	kadFileSearchID = 0;
	m_PublishedKadKey = 0;
	m_PublishedKadSrc = 0;
	m_keywordcount = 0;
	m_lastPublishTimeKadKey = 0;
	m_lastPublishTimeKadSrc = 0;
	m_nCompleteSourcesTime = time(NULL);
	m_nCompleteSourcesCount = 1;
	m_nCompleteSourcesCountLo = 1;
	m_nCompleteSourcesCountHi = 1;

	//MORPH START - Added by SiRoB, ZZ Upload System 20030807-1911
	//MORPH START - Changed by SiRoB, Avoid misusing of powershare
	//m_powershared = 0;
	m_powershared = theApp.glob_prefs->IsAutoPowershareNewDownloadFile()?2:0;
	//MORPH END   - Changed by SiRoB, Avoid misusing of powershare
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030723-0133
	//MORPH START - Added by SiRoB, Avoid misusing of powershare
	m_bPowerShareAuthorized = true;
	m_bPowerShareAuto = false;
	m_nVirtualCompleteSourcesCountMin = 0;
	m_nVirtualCompleteSourcesCountMax = 0;
	//MORPH END   - Added by SiRoB, Avoid misusing of powershare
	//MORPH START - Added by SiRoB, Reduce SharedStatusBAr CPU consumption
	InChangedSharedStatusBar = false;
	m_pbitmapOldSharedStatusBar = NULL;
	//MORPH END   - Added by SiRoB, Reduce SharedStatusBAr CPU consumption
}

CKnownFile::~CKnownFile(){
	for (int i = 0; i < hashlist.GetSize(); i++)
		if (hashlist[i])
			delete[] hashlist[i];
	for (int i = 0; i < taglist.GetSize(); i++)
		safe_delete(taglist[i]);
	m_AvailPartFrequency.RemoveAll();
	//MORPH START - Added by SiRoB, Reduce SharedStatusBar CPU consumption
	if(m_pbitmapOldSharedStatusBar != NULL) m_dcSharedStatusBar.SelectObject(m_pbitmapOldSharedStatusBar);
	//MORPH END   - Added by SiRoB, Reduce SharedStatusBar CPU consumption
}

CBarShader CKnownFile::s_ShareStatusBar(16);

//MORPH START - Modified by SiRoB, Reduce ShareStatusBar CPU consumption
void CKnownFile::DrawShareStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat){ 
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
			
			COLORREF crProgress;
			COLORREF crHave;
			COLORREF crPending;
			COLORREF crMissing = RGB(255, 0, 0);

			if(bFlat) { 
				crProgress = RGB(0, 150, 0);
				crHave = RGB(0, 0, 0);
				crPending = RGB(255,208,0);
			} else { 
				crProgress = RGB(0, 224, 0);
				crHave = RGB(104, 104, 104);
				crPending = RGB(255, 208, 0);
			} 

			s_ShareStatusBar.SetFileSize(this->GetFileSize()); 
		s_ShareStatusBar.SetHeight(iHeight); 
		s_ShareStatusBar.SetWidth(iWidth); 
			s_ShareStatusBar.Fill(crMissing); 
			COLORREF color;
			if (!onlygreyrect && !m_AvailPartFrequency.IsEmpty()) { 
				for (int i = 0;i < GetPartCount();i++)
					if(m_AvailPartFrequency[i] > 0 ){
						color = RGB(0, (210-(22*(m_AvailPartFrequency[i]-1)) <  0)? 0:210-(22*(m_AvailPartFrequency[i]-1)), 255);
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
//MORPH END - Modified by SiRoB,  Reduce ShareStatusBar CPU consumption

// SLUGFILLER: heapsortCompletesrc
static void HeapSort(CArray<uint16,uint16> &count, int32 first, int32 last){
	int32 r;
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

void CKnownFile::NewAvailPartsInfo(){
	// Cache part count
	uint16 partcount = GetPartCount();
	bool flag = (time(NULL) - m_nCompleteSourcesTime > 0); 
	
	CArray<uint16,uint16> count;	// SLUGFILLER: heapsortCompletesrc
	count.SetSize(0, m_ClientUploadList.GetSize());

	if(m_AvailPartFrequency.GetSize() < partcount)
	{
		m_AvailPartFrequency.SetSize(partcount);
	}

	// Reset part counters
	for(int i = 0; i < partcount; i++)
	{
		m_AvailPartFrequency[i] = 1;
	}
	CUpDownClient* cur_src;
	if(this->IsPartFile())
	{
		cur_src = NULL;
	}
	
	uint16 cur_count = 0;
	for (POSITION pos = m_ClientUploadList.GetHeadPosition(); pos != 0; )
	{
		cur_src = m_ClientUploadList.GetNext(pos);
		for (uint16 i = 0; i < partcount; i++)
		{
			if (cur_src->IsUpPartAvailable(i))
			{
				m_AvailPartFrequency[i] +=1;
			}
		}
		cur_count= cur_src->GetUpCompleteSourcesCount();
		if ( flag && cur_count )
		{
			count.Add(cur_count);
		}
	}
	if(flag)
	{
		m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;

		if( partcount > 0)
			m_nCompleteSourcesCount = m_AvailPartFrequency[0];
		for (uint16 i = 1; i < partcount; i++)
		{
			if( m_nCompleteSourcesCount > m_AvailPartFrequency[i])
			{
				m_nCompleteSourcesCount = m_AvailPartFrequency[i];
			}
		}
	
		if (m_nCompleteSourcesCount)
		{
			count.Add(m_nCompleteSourcesCount);
		}
	
		count.FreeExtra();
	
		int32 n = count.GetSize();
		if (n > 0)
		{
			// SLUGFILLER: heapsortCompletesrc
			int32 r;
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
			int32 i= n >> 1;		// (n / 2)
			int32 j= (n * 3) >> 2;	// (n * 3) / 4
			int32 k= (n * 7) >> 3;	// (n * 7) / 8
			if (n == 1)
			{
				m_nCompleteSourcesCount= 1;
				m_nCompleteSourcesCountLo= 1;
				m_nCompleteSourcesCountHi= 1;
			}
			if (n < 5)
			{
				m_nCompleteSourcesCount= count.GetAt(i);
				m_nCompleteSourcesCountLo= 1;
				m_nCompleteSourcesCountHi= m_nCompleteSourcesCount;
			}
			else if (n < 10)
			{
				m_nCompleteSourcesCount= count.GetAt(i);
				m_nCompleteSourcesCountLo= count.GetAt(i - 1);
				m_nCompleteSourcesCountHi= count.GetAt(i + 1);
			}
			else if (n < 20)
			{
				m_nCompleteSourcesCount= count.GetAt(i);
				m_nCompleteSourcesCountLo= count.GetAt(i);
				m_nCompleteSourcesCountHi= count.GetAt(j);
			}
			else
			{
				m_nCompleteSourcesCount= count.GetAt(j);
				m_nCompleteSourcesCountLo= m_nCompleteSourcesCount;
				m_nCompleteSourcesCountHi= count.GetAt(k);
			}
		}
		m_nCompleteSourcesTime = time(NULL) + (60);
	}
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	m_nVirtualCompleteSourcesCountMin = (uint16)-1;
	m_nVirtualCompleteSourcesCountMax = 0;
	for (uint16 i = 0; i < partcount; i++){
		if(m_AvailPartFrequency[i] > m_nVirtualCompleteSourcesCountMax)
			m_nVirtualCompleteSourcesCountMax = m_AvailPartFrequency[i];
		if(m_nVirtualCompleteSourcesCountMin > m_AvailPartFrequency[i])
			m_nVirtualCompleteSourcesCountMin = m_AvailPartFrequency[i];
	}

	UpdatePowerShareLimit((m_nCompleteSourcesCountHi<51)?true:(m_nVirtualCompleteSourcesCountMin<11), m_nVirtualCompleteSourcesCountMin==1);//rechanged think should be right tell me [SiRoB]// changed (temporaly perhaps) [Yun.SF3]
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by SiRoB, Reduce ShareStatusBar CPU consumption
	InChangedSharedStatusBar = false;
	//MORPH END   - Added by SiRoB, Reduce ShareStatusBar CPU consumption
	if (theApp.emuledlg->sharedfileswnd.m_hWnd)
		theApp.emuledlg->sharedfileswnd.sharedfilesctrl.UpdateFile(this);

}

void CKnownFile::AddUploadingClient(CUpDownClient* client){
	POSITION pos = m_ClientUploadList.Find(client); // to be sure
	if(pos == NULL){
		m_ClientUploadList.AddTail(client);
	}
}

void CKnownFile::RemoveUploadingClient(CUpDownClient* client){
	POSITION pos = m_ClientUploadList.Find(client); // to be sure
	if(pos != NULL){
		m_ClientUploadList.RemoveAt(pos);
	}
}

void CKnownFile::SetPath(LPCTSTR path){
	m_strDirectory = path;
}

void CKnownFile::SetFilePath(LPCTSTR pszFilePath)
{
	m_strFilePath = pszFilePath;
}

bool CKnownFile::CreateFromFile(LPCTSTR in_directory, LPCTSTR in_filename)
{
	SetPath(in_directory);
	SetFileName(in_filename);

	// open file
	CString namebuffer;
	namebuffer.Format("%s\\%s", in_directory, in_filename);
	SetFilePath(namebuffer);
	FILE* file = fopen(namebuffer, "rbS");
	if (!file){
		AddLogLine(false, GetResString(IDS_ERR_FILEOPEN) + CString(_T(" - %s")), namebuffer, _T(""), strerror(errno));
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
		m_AvailPartFrequency.Add(0);
	// create hashset
	uint32 togo = m_nFileSize;
	for (uint16 hashcount = 0; togo >= PARTSIZE; ) {
		uchar* newhash = new uchar[16];
		CreateHashFromFile(file, PARTSIZE, newhash);
		// SLUGFILLER: SafeHash - quick fallback
		if (!theApp.emuledlg->IsRunning()){	// in case of shutdown while still hashing
			fclose(file);
			delete[] newhash;
			return false;
		}
		// SLUGFILLER: SafeHash
		hashlist.Add(newhash);
		togo -= PARTSIZE;
		hashcount++;
	}
	uchar* lasthash = new uchar[16];
	md4clr(lasthash);
	CreateHashFromFile(file, togo, lasthash);
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

	// set lastwrite date
	struct _stat fileinfo;

	_fstat(file->_file, &fileinfo);
	date = fileinfo.st_mtime;
	
	fclose(file);
	file = NULL;

	// Add filetags
	if (theApp.glob_prefs->GetExtractMetaData() > 0)
		GetMetaDataTags();

	NewAvailPartsInfo();
	return true;	
}

void CKnownFile::SetFileSize(uint32 nFileSize)
{
	CAbstractFile::SetFileSize(nFileSize);

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
		ASSERT(0);
		m_iPartCount = 0;
		m_iED2KPartCount = 0;
		m_iED2KPartHashCount = 0;
		return;
	}

	// nr. of data parts
	m_iPartCount = (nFileSize + (PARTSIZE - 1)) / PARTSIZE;

	// nr. of parts to be used with OP_FILESTATUS
	m_iED2KPartCount = nFileSize / PARTSIZE + 1;

	// nr. of parts to be used with OP_HASHSETANSWER
	m_iED2KPartHashCount = nFileSize / PARTSIZE;
	if (m_iED2KPartHashCount != 0)
		m_iED2KPartHashCount += 1;
}

// needed for memfiles. its probably better to switch everything to CFile...
bool CKnownFile::LoadHashsetFromFile(CFile* file, bool checkhash){
	uchar checkid[16];
	file->Read(&checkid, 16);
	//TRACE("File size: %u (%u full parts + %u bytes)\n", GetFileSize(), GetFileSize()/PARTSIZE, GetFileSize()%PARTSIZE);
	//TRACE("File hash: %s\n", md4str(checkid));
	uint16	parts;
	file->Read(&parts, 2);
	//TRACE("Nr. hashs: %u\n", (UINT)parts);
	for (UINT i = 0; i < (UINT)parts; i++){
		uchar* cur_hash = new uchar[16];
		file->Read(cur_hash, 16);
		//TRACE("Hash[%3u]: %s\n", i, md4str(cur_hash));
		hashlist.Add(cur_hash);
	}

	// SLUGFILLER: SafeHash - always check for valid hashlist
	if (!checkhash){
		md4cpy(m_abyFileHash, checkid);
		if (parts <= 1)	// nothing to check
		return true;
	}
	else if (md4cmp(m_abyFileHash, checkid))
		return false;	// wrong file?
	else{
		if (parts != GetED2KPartHashCount())
			return false;
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
		for (int i = 0; i < hashlist.GetSize(); i++)
			delete[] hashlist[i];
		hashlist.RemoveAll();
		return false;
	}
}

bool CKnownFile::LoadTagsFromFile(CFile* file){
	uint32 tagcount;
	//MORPH START - Added by SiRoB, SLUGFILLER: Spreadbars
	// SLUGFILLER: Spreadbars
	CMap<uint16,uint16,uint32,uint32> spread_start_map;
	CMap<uint16,uint16,uint32,uint32> spread_end_map;
	CMap<uint16,uint16,uint32,uint32> spread_count_map;
	// SLUGFILLER: Spreadbars
	//MORPH END - Added by SiRoB, SLUGFILLER: Spreadbars
	file->Read(&tagcount,4);
	for (uint32 j = 0; j != tagcount;j++){
		CTag* newtag = new CTag(file);
		switch(newtag->tag.specialtag){
			case FT_FILENAME:{
				SetFileName(newtag->tag.stringvalue);
				delete newtag;
				break;
			}
			case FT_FILESIZE:{
				SetFileSize(newtag->tag.intvalue);
				m_AvailPartFrequency.SetSize(GetPartCount());
				for (uint32 i = 0; i < GetPartCount();i++)
					m_AvailPartFrequency.Add(0);
				delete newtag;
				break;
			}
			case FT_ATTRANSFERED:{
				statistic.alltimetransferred = newtag->tag.intvalue;
				delete newtag;
				break;
			}
			case FT_ATTRANSFEREDHI:{
				uint32 hi,low;
				low=statistic.alltimetransferred;
				hi = newtag->tag.intvalue;
				uint64 hi2;
				hi2=hi;
				hi2=hi2<<32;
				statistic.alltimetransferred=low+hi2;
				delete newtag;
				break;
			}
			case FT_ATREQUESTED:{
				statistic.alltimerequested = newtag->tag.intvalue;
				delete newtag;
				break;
			}
 			case FT_ATACCEPTED:{
				statistic.alltimeaccepted = newtag->tag.intvalue;
				delete newtag;
				break;
			}
			case FT_ULPRIORITY:{
				uint8 autoprio = PR_AUTO;
				m_iUpPriority = newtag->tag.intvalue;
				if( m_iUpPriority == autoprio ){
					m_iUpPriority = PR_HIGH;
					m_bAutoUpPriority = true;
				}
				else
					m_bAutoUpPriority = false;
				delete newtag;
				break;
			}
			case FT_PERMISSIONS:{
				m_iPermissions = newtag->tag.intvalue;
				delete newtag;
				break;
			}
			case FT_KADLASTPUBLISHKEY:{
				m_lastPublishTimeKadKey = newtag->tag.intvalue;
				delete newtag;
				break;
			}
			case FT_KADLASTPUBLISHSRC:{
				m_lastPublishTimeKadSrc = newtag->tag.intvalue;
				delete newtag;
				break;
			}
			// EastShare START - Added by TAHO, .met file control
			case FT_LASTUSED:{
				statistic.SetLastUsed(newtag->tag.intvalue);
				delete newtag;
				break;
			}
			// EastShare END - Added by TAHO, .met file control
			default:
				//MORPH START - Added by SiRoB, ZZ Upload System
				if((!newtag->tag.specialtag) && strcmp(newtag->tag.tagname, FT_POWERSHARE) == 0) {
					//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
					//SetPowerShared(newtag->tag.intvalue == 1);
					SetPowerShared((newtag->tag.intvalue<3)?newtag->tag.intvalue:2);
					//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
					delete newtag;
				} else
				//MORPH END - Added by SiRoB, ZZ Upload System
				//MORPH START - Added by SiRoB, SLUGFILLER: Spreadbars
				if ((!newtag->tag.specialtag) &&
					(newtag->tag.tagname[0] == FT_SPREADSTART ||
					newtag->tag.tagname[0] == FT_SPREADEND ||
					newtag->tag.tagname[0] == FT_SPREADCOUNT)){
					uint16 spreadkey = atoi(&newtag->tag.tagname[1]);
					if (newtag->tag.tagname[0] == FT_SPREADSTART)
						spread_start_map.SetAt(spreadkey, newtag->tag.intvalue);
					else if (newtag->tag.tagname[0] == FT_SPREADEND)
						spread_end_map.SetAt(spreadkey, newtag->tag.intvalue);
					else if (newtag->tag.tagname[0] == FT_SPREADCOUNT)
						spread_count_map.SetAt(spreadkey, newtag->tag.intvalue);
					delete newtag;
				}
				else {
					ConvertED2KTag(newtag);
					if (newtag)
						taglist.Add(newtag);
				}
				//MORPH END - Added by SiRoB, SLUGFILLER: Spreadbars
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
	return true;
}

bool CKnownFile::LoadDateFromFile(CFile* file){
	file->Read(&date,4);
	return true;
}

bool CKnownFile::LoadFromFile(CFile* file){
	// SLUGFILLER: SafeHash - load first, verify later
	bool ret1 = LoadDateFromFile(file);
	bool ret2 = LoadHashsetFromFile(file,false);
	bool ret3 = LoadTagsFromFile(file);
	NewAvailPartsInfo();
	return ret1 && ret2 && ret3 && GetED2KPartHashCount()==GetHashCount();// Final hash-count verification, needs to be done after the tags are loaded.
	// SLUGFILLER: SafeHash
}

bool CKnownFile::WriteToFile(CFile* file){
	// date
	file->Write(&date,4); 
	// hashset
	file->Write(&m_abyFileHash,16);
	uint16 parts = hashlist.GetCount();
	file->Write(&parts,2);
	for (int i = 0; i < parts; i++)
		file->Write(hashlist[i],16);
	//tags
	//MORPH START - Modified by SiRoB, ZZ Upload System
	//const int iFixedTags = 10;
	//const int iFixedTags = 11;
	const int iFixedTags = 12;//EastShare - met control, known files expire tag[TAHO]
	//MORPH END - Modified by SiRoB, ZZ Upload System
	uint32 tagcount = iFixedTags;
	// Float meta tags are currently not written. All older eMule versions < 0.28a have 
	// a bug in the meta tag reading+writing code. To achive maximum backward 
	// compatibility for met files with older eMule versions we just don't write float 
	// tags. This is OK, because we (eMule) do not use float tags. The only float tags 
	// we may have to handle is the '# Sent' tag from the Hybrid, which is pretty 
	// useless but may be received from us via the servers.
	// 
	// The code for writing the float tags SHOULD BE ENABLED in SOME MONTHS (after most 
	// people are using the newer eMule versions which do not write broken float tags).	
	for (int j = 0; j < taglist.GetCount(); j++){
		if (taglist[j]->tag.type == 2 || taglist[j]->tag.type == 3)
			tagcount++;
	}
	//MORPH START - Added by IceCream, SLUGFILLER: Spreadbars - count spread tags
		for (POSITION pos = statistic.spreadlist.GetHeadPosition(); pos; statistic.spreadlist.GetNext(pos))
		if (statistic.spreadlist.GetValueAt(pos))
			tagcount += 3;
	//MORPH END   - Added by IceCream, SLUGFILLER: Spreadbars
	// standard tags
	file->Write(&tagcount, 4);
	
	CTag nametag(FT_FILENAME, GetFileName());
	nametag.WriteTagToFile(file);
		
	CTag sizetag(FT_FILESIZE, m_nFileSize);
	sizetag.WriteTagToFile(file);
	
	// statistic
	uint32 tran;
	tran=statistic.alltimetransferred;
	CTag attag1(FT_ATTRANSFERED, tran);
	attag1.WriteTagToFile(file);
	tran=statistic.alltimetransferred>>32;
	CTag attag4(FT_ATTRANSFEREDHI, tran);
	attag4.WriteTagToFile(file);

	CTag attag2(FT_ATREQUESTED, statistic.GetAllTimeRequests());
	attag2.WriteTagToFile(file);
	
	CTag attag3(FT_ATACCEPTED, statistic.GetAllTimeAccepts());
	attag3.WriteTagToFile(file);

	// priority N permission
	CTag priotag(FT_ULPRIORITY, IsAutoUpPriority() ? PR_AUTO : m_iUpPriority);
	priotag.WriteTagToFile(file);

	CTag permtag(FT_PERMISSIONS, m_iPermissions);
	permtag.WriteTagToFile(file);

	CTag kadLastPubKey(FT_KADLASTPUBLISHKEY, m_lastPublishTimeKadKey);
	kadLastPubKey.WriteTagToFile(file);

	CTag kadLastPubSrc(FT_KADLASTPUBLISHSRC, m_lastPublishTimeKadSrc);
	kadLastPubSrc.WriteTagToFile(file);

	//EastShare START - Added by TAHO, .met file control
	uint32 value;
	value = ( theApp.sharedfiles->GetFileByID(GetFileHash())) ? time(NULL) : statistic.GetLastUsed();
	CTag lastUsedTag(FT_LASTUSED, value);
	lastUsedTag.WriteTagToFile(file);
	//EastShare END - Added by TAHO, .met file control

	//MORPH START - Added by SiRoB, ZZ Upload System
	//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
	//CTag powersharetag(FT_POWERSHARE, (GetPowerShared())?1:0);
	CTag powersharetag(FT_POWERSHARE, GetPowerSharedMode());
	//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
	powersharetag.WriteTagToFile(file);
	//MORPH END - Added by SiRoB, ZZ Upload System

	//MORPH START - Added by IceCream, SLUGFILLER: Spreadbars
	char* namebuffer = new char[10];
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
		i_pos++;
	}
	delete[] namebuffer;
	//MORPH END   - Added by IceCream, SLUGFILLER: Spreadbars

	//other tags
	for (int j = 0; j < taglist.GetCount(); j++){
		if (taglist[j]->tag.type == 2 || taglist[j]->tag.type == 3)
			taglist[j]->WriteTagToFile(file);
	}
	return true;
}

void CKnownFile::CreateHashFromInput(FILE* file,CFile* file2, int Length, uchar* Output, uchar* in_string) { 
	// time critial
	bool PaddingStarted = false;
	uint32 Hash[4];
	Hash[0] = 0x67452301;
	Hash[1] = 0xEFCDAB89;
	Hash[2] = 0x98BADCFE;
	Hash[3] = 0x10325476;
	CFile* data = 0;
	if (in_string)
		data = new CMemFile(in_string,Length);
	uint32 Required = Length;
	uchar   X[64*128];  
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
		for (uint32 i = 0; i < len; i++) 
        { 
           MD4Transform(Hash, (uint32*)(X + i*64)); 
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
	}
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
	safe_delete(data);
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

void CAbstractFile::SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars)
{ 
	m_strFileName = pszFileName;
	if (bReplaceInvalidFileSystemChars){
		m_strFileName.Replace(_T('/'), _T('-'));
		m_strFileName.Replace(_T('>'), _T('-'));
		m_strFileName.Replace(_T('<'), _T('-'));
		m_strFileName.Replace(_T('*'), _T('-'));
		m_strFileName.Replace(_T(':'), _T('-'));
		m_strFileName.Replace(_T('?'), _T('-'));
	}
	SetFileType(GetFiletypeByName(m_strFileName));
} 
      
void CAbstractFile::SetFileType(LPCTSTR pszFileType)
{ 
	m_strFileType = pszFileType;
} 

uint32 CAbstractFile::GetIntTagValue(uint8 tagname) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		const CTag* pTag = taglist[i];
		if (pTag->tag.specialtag==tagname && pTag->tag.type==3)
			return pTag->tag.intvalue;
	}
	return NULL;
}

uint32 CAbstractFile::GetIntTagValue(LPCSTR tagname) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		const CTag* pTag = taglist[i];
		if (pTag->tag.specialtag==0 && pTag->tag.type==3 && stricmp(pTag->tag.tagname, tagname)==0)
			return pTag->tag.intvalue;
	}
	return NULL;
}

LPCSTR CAbstractFile::GetStrTagValue(uint8 tagname) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		const CTag* pTag = taglist[i];
		if (pTag->tag.specialtag==tagname && pTag->tag.type==2)
			return pTag->tag.stringvalue;			
	}
	return NULL;
}

LPCSTR CAbstractFile::GetStrTagValue(LPCSTR tagname) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		const CTag* pTag = taglist[i];
		if (pTag->tag.specialtag==0 && pTag->tag.type==2 && stricmp(pTag->tag.tagname, tagname)==0)
			return pTag->tag.stringvalue;
	}
	return NULL;
}

CTag* CAbstractFile::GetTag(uint8 tagname, uint8 tagtype) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		CTag* pTag = taglist[i];
		if (pTag->tag.specialtag==tagname && pTag->tag.type==tagtype)
			return pTag;
	}
	return NULL;
}

CTag* CAbstractFile::GetTag(LPCSTR tagname, uint8 tagtype) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		CTag* pTag = taglist[i];
		if (pTag->tag.specialtag==0 && pTag->tag.type==tagtype && stricmp(pTag->tag.tagname, tagname)==0)
			return pTag;
	}
	return NULL;
}

CTag* CAbstractFile::GetTag(uint8 tagname) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		CTag* pTag = taglist[i];
		if (pTag->tag.specialtag==tagname)
			return pTag;
	}
	return NULL;
}

CTag* CAbstractFile::GetTag(LPCSTR tagname) const
{
	for (int i = 0; i < taglist.GetSize(); i++){
		CTag* pTag = taglist[i];
		if (pTag->tag.specialtag==0 && stricmp(pTag->tag.tagname, tagname)==0)
			return pTag;
	}
	return NULL;
}

void CAbstractFile::AddTagUnique(CTag* pTag)
{
	for (int i = 0; i < taglist.GetSize(); i++){
		const CTag* pCurTag = taglist[i];
		if ( (   (pCurTag->tag.specialtag!=0 && pCurTag->tag.specialtag==pTag->tag.specialtag)
			  || (pCurTag->tag.tagname!=NULL && pTag->tag.tagname!=NULL && stricmp(pCurTag->tag.tagname, pTag->tag.tagname)==0)
			 )
			 && pCurTag->tag.type == pTag->tag.type){
			delete pCurTag;
			taglist.SetAt(i, pTag);
			return;
		}
	}
	taglist.Add(pTag);
}

Packet*	CKnownFile::CreateSrcInfoPacket(CUpDownClient* forClient){
	CTypedPtrList<CPtrList, CUpDownClient*> srclist;
	theApp.uploadqueue->FindSourcesForFileById(&srclist, forClient->GetUploadFileID()); //should we use "m_abyFileHash"?

	if(srclist.IsEmpty())
		return 0;

	CMemFile data;
	uint16 nCount = 0;

	data.Write(forClient->GetUploadFileID(), 16);
	data.Write(&nCount, 2);

	//uint32 lastRequest = forClient->GetLastSrcReqTime();
	//we are only taking 30 random sources since we can't be sure if they have parts we need
	//this is hard coded because its a temp solution until next(?) version
	srand(time(NULL));
	for(int i = 0; i < 30; i++) {
		int victim = ((rand() >> 7) % srclist.GetSize());
		POSITION pos = srclist.FindIndex(victim);
		CUpDownClient *cur_src = srclist.GetAt(pos);
		if(!cur_src->HasLowID() && cur_src != forClient) {
			nCount++;
			uint32 dwID;
			if(forClient->GetSourceExchangeVersion() > 2)
				dwID = cur_src->GetUserIDHybrid();
			else
				dwID = cur_src->GetIP();
			uint16 nPort = cur_src->GetUserPort();
			uint32 dwServerIP = cur_src->GetServerIP();
			uint16 nServerPort = cur_src->GetServerPort();
			data.Write(&dwID, 4);
			data.Write(&nPort, 2);
			data.Write(&dwServerIP, 4);
			data.Write(&nServerPort, 2);
			if (forClient->GetSourceExchangeVersion() > 1)
				data.Write(cur_src->GetUserHash(),16);
		}

		srclist.RemoveAt(pos);
		if(srclist.GetSize() == 0)
			break;
	}
	if (!nCount)
		return 0;
	data.Seek(16,0);
	data.Write(&nCount,2);

	Packet* result = new Packet(&data, OP_EMULEPROT);
	result->opcode = OP_ANSWERSOURCES;
	// 16+2+30*(4+2+4+2) = 378 bytes max.
	if ( result->size > 354 )
		result->PackPacket();
	if ( theApp.glob_prefs->GetDebugSourceExchange() )
		AddDebugLogLine( false, "Send:Source User(%s) File(%s) Count(%i)", forClient->GetUserName(), GetFileName(), nCount );
	return result;
}

//For File Comment // 
void CKnownFile::LoadComment(){ 
	char buffer[100]; 
	char* fullpath = new char[strlen(theApp.glob_prefs->GetConfigDir())+13]; 
	sprintf(fullpath,"%sfileinfo.ini",theApp.glob_prefs->GetConfigDir()); 

	buffer[0] = 0;
	for (uint16 i = 0;i != 16;i++) 
		sprintf(buffer,"%s%02X",buffer,m_abyFileHash[i]); 

	CIni ini( fullpath, buffer); 
	m_strComment = ini.GetString("Comment").Left(50); 
	m_iRate = ini.GetInt("Rate", 0);//For rate
	m_bCommentLoaded=true;
	delete[] fullpath;
}    

void CKnownFile::SetFileComment(CString strNewComment){ 
	char buffer[100]; 
	char* fullpath = new char[strlen(theApp.glob_prefs->GetConfigDir())+13]; 
	sprintf(fullpath,"%sfileinfo.ini",theApp.glob_prefs->GetConfigDir()); 
	    
	buffer[0] = 0; 
	for (uint16 i = 0;i != 16;i++) 
		sprintf(buffer,"%s%02X",buffer,m_abyFileHash[i]); 
    
	CIni ini( fullpath, buffer ); 
	ini.WriteString ("Comment", strNewComment); 
	m_strComment = strNewComment;
	delete[] fullpath;
   
	CTypedPtrList<CPtrList, CUpDownClient*> srclist;
	theApp.uploadqueue->FindSourcesForFileById(&srclist, this->GetFileHash());

	for (POSITION pos = srclist.GetHeadPosition();pos != 0;srclist.GetNext(pos)){
		CUpDownClient *cur_src = srclist.GetAt(pos);
		cur_src->SetCommentDirty();
	}
}

// For File rate 
void CKnownFile::SetFileRate(int8 iNewRate){ 
	char buffer[100]; 
	char* fullpath = new char[strlen(theApp.glob_prefs->GetConfigDir())+13]; 
	sprintf(fullpath,"%sfileinfo.ini",theApp.glob_prefs->GetConfigDir()); 
	    
	buffer[0] = 0; 
	for (uint16 i = 0;i != 16;i++) 
		sprintf(buffer,"%s%02X",buffer,m_abyFileHash[i]); 

	CIni ini( fullpath, buffer ); 
	ini.WriteInt ("Rate", iNewRate); 
	m_iRate = iNewRate; 
	delete[] fullpath;

	CTypedPtrList<CPtrList, CUpDownClient*> srclist;
	theApp.uploadqueue->FindSourcesForFileById(&srclist, this->GetFileHash());
	for (POSITION pos = srclist.GetHeadPosition();pos != 0;srclist.GetNext(pos)){
		CUpDownClient *cur_src = srclist.GetAt(pos);
		cur_src->SetCommentDirty();
	}
} 

void CKnownFile::UpdateAutoUpPriority(){
	if( !IsAutoUpPriority() )
		return;
	if ( GetQueuedCount() > 20 ){
		if( GetUpPriority() != PR_LOW ){
			SetUpPriority( PR_LOW );
			theApp.emuledlg->sharedfileswnd.sharedfilesctrl.UpdateFile(this);
		}
		return;
	}
	if ( GetQueuedCount() > 1 ){
		if( GetUpPriority() != PR_NORMAL ){
			SetUpPriority( PR_NORMAL );
			theApp.emuledlg->sharedfileswnd.sharedfilesctrl.UpdateFile(this);
		}
		return;
	}
	if( GetUpPriority() != PR_HIGH){
		SetUpPriority( PR_HIGH );
		theApp.emuledlg->sharedfileswnd.sharedfilesctrl.UpdateFile(this);
	}
}

void CKnownFile::SetUpPriority(uint8 iNewUpPriority, bool m_bsave){
	m_iUpPriority = iNewUpPriority;
	if( this->IsPartFile() && m_bsave )
		((CPartFile*)this)->SavePartFile();
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

int GetStreamFormat(const PAVISTREAM pAviStrm, long &rlStrmFmtSize, LPVOID &rpStrmFmt)
{
	ASSERT(pAviStrm);
	ASSERT(rpStrmFmt == NULL);

	// Get size of stream format data
	HRESULT hr;
	rlStrmFmtSize = 0;
	if ((hr = AVIStreamReadFormat(pAviStrm, AVIStreamStart(pAviStrm), NULL, &rlStrmFmtSize)) != AVIERR_OK)
		return FALSE;

	// Alloc stream format data
	if (rlStrmFmtSize == 0)
		return FALSE;
	rpStrmFmt = (LPVOID)malloc(rlStrmFmtSize);
	if (rpStrmFmt == NULL)
		return FALSE;

	// Read stream format data
	if ((hr = AVIStreamReadFormat(pAviStrm, AVIStreamStart(pAviStrm), rpStrmFmt, &rlStrmFmtSize)) != AVIERR_OK) {
		free(rpStrmFmt);
		rpStrmFmt = NULL;
		return FALSE;
	}

	return TRUE;
}

void CKnownFile::GetMetaDataTags()
{
	if (theApp.glob_prefs->GetExtractMetaData() == 0)
		return;

	TCHAR szExt[_MAX_EXT];
	_tsplitpath(GetFileName(), NULL, NULL, NULL, szExt);
	_tcslwr(szExt);
	if (_tcscmp(szExt, _T(".mp3"))==0 || _tcscmp(szExt, _T(".mp2"))==0 || _tcscmp(szExt, _T(".mp1"))==0 || _tcscmp(szExt, _T(".mpa"))==0)
	{
		TCHAR szFullPath[MAX_PATH];
		_tmakepath(szFullPath, NULL, GetPath(), GetFileName(), NULL);

		try{
			ID3_Tag myTag;
			myTag.Link(szFullPath, ID3TT_ALL);

			const Mp3_Headerinfo* mp3info;
			mp3info = myTag.GetMp3HeaderInfo();
			if (mp3info)
			{
				// length
				if (mp3info->time){
					CTag* pTag = new CTag(FT_MEDIA_LENGTH, (uint32)mp3info->time);
					AddTagUnique(pTag);
				}

				// here we could also create a "codec" ed2k meta tag.. though it would probable not be worth the
				// extra bytes which would have to be sent to the servers..

				// bitrate
				UINT uBitrate = mp3info->bitrate/1000;
				if (uBitrate){
					CTag* pTag = new CTag(FT_MEDIA_BITRATE, (uint32)uBitrate);
					AddTagUnique(pTag);
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
						CStringA strText(pszText);
						strText.Trim();
						if (!strText.IsEmpty()){
							CTag* pTag = new CTag(FT_MEDIA_ARTIST, strText);
						AddTagUnique(pTag);
						}
						delete[] pszText;
						break;
					}
					case ID3FID_ALBUM:{
						char* pszText = ID3_GetString(frame, ID3FN_TEXT);
						CStringA strText(pszText);
						strText.Trim();
						if (!strText.IsEmpty()){
							CTag* pTag = new CTag(FT_MEDIA_ALBUM, strText);
						AddTagUnique(pTag);
						}
						delete[] pszText;
						break;
					}
					case ID3FID_TITLE:{
						char* pszText = ID3_GetString(frame, ID3FN_TEXT);
						CStringA strText(pszText);
						strText.Trim();
						if (!strText.IsEmpty()){
							CTag* pTag = new CTag(FT_MEDIA_TITLE, strText);
						AddTagUnique(pTag);
						}
						delete[] pszText;
						break;
					}
				}
			}
			delete iter;
		}
		catch(...){
			ASSERT(0);
			AddDebugLogLine(false, _T("Unhandled exception while extracting file meta (MP3) data from \"%s\""), szFullPath);
		}
	}
	/*else if (_tcscmp(szExt, _T(".avi"))==0)
	{
		TCHAR szFullPath[MAX_PATH];
		_tmakepath(szFullPath, NULL, GetPath(), GetFileName(), NULL);

		try{
			AVISTREAMINFO VideoStrmInf = {0};
			LPVOID pVideoStrmFmt = NULL;
			long lVideoStrmFmtSize = 0;
			AVISTREAMINFO AudioStrmInf = {0};
			LPVOID pAudioStrmFmt = NULL;
			long lAudioStrmFmtSize = 0;

			PAVIFILE pAviFile;
			HRESULT hr = AVIFileOpen(&pAviFile, szFullPath, OF_READ | OF_SHARE_DENY_NONE, NULL);
			if (hr == AVIERR_OK)
			{
				int iStreamIdx = 0;
				PAVISTREAM pAviStrm;
				while (AVIFileGetStream(pAviFile, &pAviStrm, 0, iStreamIdx) == AVIERR_OK)
				{
					AVISTREAMINFO AviStrmInf;
					if (AVIStreamInfo(pAviStrm, &AviStrmInf, sizeof(AviStrmInf)) != AVIERR_OK) {
						AVIStreamRelease(pAviStrm);
						break;
					}

					if (AviStrmInf.fccType == streamtypeVIDEO){
						if (VideoStrmInf.fccType == 0){
							VideoStrmInf = AviStrmInf;
							GetStreamFormat(pAviStrm, lVideoStrmFmtSize, pVideoStrmFmt);
						}
					}
					else if (AviStrmInf.fccType == streamtypeAUDIO){
						if (AudioStrmInf.fccType == 0){
							AudioStrmInf = AviStrmInf;
							GetStreamFormat(pAviStrm, lAudioStrmFmtSize, pAudioStrmFmt);
						}
					}
					AVIStreamRelease(pAviStrm);
					iStreamIdx++;
				}
				AVIFileRelease(pAviFile);
			}

			if (VideoStrmInf.fccType == streamtypeVIDEO && pVideoStrmFmt != NULL)
			{
				// length
				double fSamplesSec = (VideoStrmInf.dwScale != 0) ? (double)VideoStrmInf.dwRate / (double)VideoStrmInf.dwScale : 0.0F;
				double fLengthSec = (fSamplesSec > 0.0) ? VideoStrmInf.dwLength / fSamplesSec : 0;
				if (fLengthSec > 0.0){
					CTag* pTag = new CTag(FT_MEDIA_LENGTH, (uint32)fLengthSec);
					AddTagUnique(pTag);
				}

				// codec
				LPBITMAPINFOHEADER pbmi = (LPBITMAPINFOHEADER)pVideoStrmFmt;
				CStringA strCodec;
				if (pbmi->biCompression == BI_RGB)
					strCodec = "rgb";
				else if (pbmi->biCompression == BI_RLE8)
					strCodec = "rle8";
				else if (pbmi->biCompression == BI_RLE4)
					strCodec = "rle4";
				else if (pbmi->biCompression == BI_BITFIELDS)
					strCodec = "bitfields";
				else{
					memcpy(strCodec.GetBuffer(4), &pbmi->biCompression, 4);
					strCodec.ReleaseBuffer(4);
					strCodec.MakeLower();
				}
				CTag* pTag = new CTag(FT_MEDIA_CODEC, strCodec);
				AddTagUnique(pTag);

				// bitrate.. audio or video??
			}

			if (pVideoStrmFmt)
				free(pVideoStrmFmt);
			if (pAudioStrmFmt)
				free(pAudioStrmFmt);
		}
		catch(...){
			AddDebugLogLine(false, _T("Unhandled exception while extracting meta data (AVI) from \"%s\""), szFullPath);
		}
	}*/
	else if (theApp.glob_prefs->GetExtractMetaData() > 1)
	{
		// starting the MediaDet object takes a noticeable amount of time.. avoid starting that object
		// for files which are not expected to contain any Audio/Video data.
		// note also: MediaDet does not work well for too short files (e.g. 16K)
		EED2KFileType eFileType = GetED2KFileTypeID(GetFileName());
		if ((eFileType == ED2KFT_AUDIO || eFileType == ED2KFT_VIDEO) && GetFileSize() >= 32768)
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
						DWORD dwAudioCodec = 0;
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
														dwVideoBitRate = pVIH->dwBitRate / 1000;
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
						}

						if (!strCodec.IsEmpty()){
							CTag* pTag = new CTag(FT_MEDIA_CODEC, strCodec);
							AddTagUnique(pTag);
						}

						if (uBitrate){
							CTag* pTag = new CTag(FT_MEDIA_BITRATE, (uint32)uBitrate);
							AddTagUnique(pTag);
						}
					}
				}
			}
			catch(...){
				AddDebugLogLine(false, _T("Unhandled exception while extracting meta data (MediaDet) from \"%s\""), szFullPath);
			}
		}
	}
}
void CKnownFile::SetPublishedED2K(bool val){
	m_PublishedED2K = val;
	theApp.emuledlg->sharedfileswnd.sharedfilesctrl.UpdateFile(this);
}

void CKnownFile::SetPublishedKadSrc(){
	m_PublishedKadSrc++;
	theApp.emuledlg->sharedfileswnd.sharedfilesctrl.UpdateFile(this);
}
void CKnownFile::SetPublishedKadKey(){
	m_PublishedKadKey++;
	theApp.emuledlg->sharedfileswnd.sharedfilesctrl.UpdateFile(this);
}

int CKnownFile::PublishKey(Kademlia::CUInt128 *nextID)
{
	if( m_lastPublishTimeKadKey > 0)
	{
		if( ((uint32)time(NULL)-m_lastPublishTimeKadKey) < KADEMLIAREPUBLISHTIME)
		{
			return false;
		}
	}
	if(wordlist.empty())
	{
		if(m_keywordcount)
		{
			m_lastPublishTimeKadKey = (uint32)time(NULL);
			m_keywordcount = 0;
			return false;
		}
		Kademlia::CSearchManager::getWordsValid(this->GetFileName(), &wordlist);
		if(wordlist.empty())
		{
			return false;
		}
	}
	CString word = wordlist.front();
	wordlist.pop_front();
	Kademlia::CMD4::hash((byte*)word.GetBuffer(0), word.GetLength(), nextID);
	m_keywordcount++;
	return m_keywordcount;
}

bool CKnownFile::PublishSrc(Kademlia::CUInt128 *nextID)
{
	if( m_lastPublishTimeKadSrc > 0)
	{
		if( ((uint32)time(NULL)-m_lastPublishTimeKadSrc) < KADEMLIAREPUBLISHTIME)
		{
			return false;
		}
	}
	m_lastPublishTimeKadSrc = (uint32)time(NULL);
	return true;
}



bool CKnownFile::IsMovie(){
	return (ED2KFT_VIDEO == GetED2KFileTypeID(GetFileName()) );
}

// function assumes that this file is shared and that any needed permission to preview exists. checks have to be done before calling! 
bool CKnownFile::GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender){
	return GrabImage(GetPath() + CString("\\") + GetFileName(), nFramesToGrab,  dStartTime, bReduceColor, nMaxWidth, pSender);
}

bool CKnownFile::GrabImage(CString strFileName,uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender){
	if (!IsMovie())
		return false;
	CFrameGrabThread* framegrabthread = (CFrameGrabThread*) AfxBeginThread(RUNTIME_CLASS(CFrameGrabThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
	framegrabthread->SetValues(this,strFileName,nFramesToGrab,  dStartTime, bReduceColor, nMaxWidth, pSender);
	framegrabthread->ResumeThread();
	return true;
}

// imgResults[i] can be NULL
void CKnownFile::GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender){
	// continue processing
	if (pSender == theApp.mmserver){
		theApp.mmserver->PreviewFinished(imgResults, nFramesGrabbed);
	}
	else if (theApp.clientlist->IsValidClient((CUpDownClient*)pSender)){
		((CUpDownClient*)pSender)->SendPreviewAnswer(this, imgResults, nFramesGrabbed);
	}
	else{
		//probably a client which got deleted while grabbing the frames for some reason
		AddDebugLogLine(false, "Couldn't find Sender of FrameGrabbing Request");
	}
	//cleanup
	for (int i = 0; i != nFramesGrabbed; i++){
		if (imgResults[i] != NULL)
			delete imgResults[i];
	}
	delete[] imgResults;

}

// #zegzav:updcliuplst
void CKnownFile::UpdateClientUploadList()
{
	// remove non-existent clients / add missing clients
	theApp.clientlist->GetClientListByFileID(&m_ClientUploadList, GetFileHash());
	m_iQueuedCount = m_ClientUploadList.GetCount();
	UpdateAutoUpPriority();
}
// SLUGFILLER: hideOS
uint16 CKnownFile::CalcPartSpread(CArray<uint32, uint32>& partspread, CUpDownClient* client){
	uint16 parts = GetED2KPartCount();
	uint16 realparts = GetPartCount();
	uint32 min;
	uint16 mincount;
	uint16 i;

	ASSERT(client != NULL);

	partspread.RemoveAll();

	if (!parts)
		return 0;

	CArray<bool, bool> partsavail;
	bool usepartsavail = false;

	for (i = 0; i < parts; i++) {
		partspread.Add(0);
		partsavail.Add(true);
	}

	if(statistic.spreadlist.IsEmpty())
		return parts;

	if (IsPartFile())
		for (i = 0; i < realparts; i++)
			if (!((CPartFile*)this)->IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){	// SLUGFILLER: SafeHash
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
		uint32 next = statistic.spreadlist.GetKeyAt(pos)/PARTSIZE;
		if (next > parts)
			next = parts;
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
			next = statistic.spreadlist.GetKeyAt(pos)/PARTSIZE;
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

	if (!theApp.glob_prefs->IsSelectiveShareEnabled())
		return parts;

	uint8 hideOS = theApp.glob_prefs->GetHideOvershares();
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
		m_PartSentCount.RemoveAll();
		min = 0;
		mincount = 0;
		for (i = 0; i < parts; i++){
			m_PartSentCount.Add(partspread[i]);
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

bool CKnownFile::HideOvershares(CFile* file, CUpDownClient* client){
	uint8 hideOS = theApp.glob_prefs->GetHideOvershares();
	if (!hideOS)
		return FALSE;
	CArray<uint32, uint32> partspread;
	uint16 parts = CalcPartSpread(partspread, client);
	if (!parts)
		return FALSE;
	uint32 max;
	max = partspread[0];
	for (uint16 i = 1; i < parts; i++)
		if (partspread[i] > max)
			max = partspread[i];
	if (max < hideOS)
		return FALSE;

	file->Write(&parts,2);
	uint16 done = 0;
	while (done != parts){
		uint8 towrite = 0;
		for (uint32 i = 0;i != 8;i++){
			if (partspread[done] < hideOS)
				towrite |= (1<<i);
			done++;
			if (done == parts)
				break;
		}
		file->Write(&towrite,1);
	}
	return TRUE;
}
// SLUGFILLER: hideOS

//MORPH - Changed by SiRoB, Avoid Sharing Nothing :( the return should be conditional
//Wistily : Share only the need START (Inspired by lovelace release feature, adapted from Slugfiller hideOS code)
bool CKnownFile::ShareOnlyTheNeed(CFile* file)
{
	uint16 parts = GetED2KPartCount();
	if (!parts || !m_bPowerShareAuto)
		return FALSE;
		
	file->Write(&parts,2);
	uint16 done = 0;
	bool ok = false; //MORPH - Added by SiRoB, Avoid Sharing Nothing :(
	while (done != parts){
		uint8 towrite = 0;
		for (uint32 i = 0;i != 8;i++){
			if (m_AvailPartFrequency[done] <= 1)
				towrite |= (1<<i);
			done++;
			if (done == parts)
				break;
		}
		ok |= (towrite!=0); //MORPH - Added by SiRoB, Avoid Sharing Nothing :(
		file->Write(&towrite,1);
	}
	//return TRUE;
	return ok; //MORPH - Added by SiRoB, Avoid Sharing Nothing :(
}
//Wistily : Share only the need STOP