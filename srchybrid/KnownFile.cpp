// parts of this file are based on work from pan One (http://home-3.tiscali.nl/~meost/pms/)
//this file is part of eMule
//Copyright (C)2002-2007 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "FrameGrabThread.h"
#include "CxImage/xImage.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "PartFile.h"
#include "Packets.h"
#include "Kademlia/Kademlia/SearchManager.h"
#include "Kademlia/Kademlia/Entry.h"
#include "SafeFile.h"
#include "shahashset.h"
#include "Log.h"
#include "MD4.h"
#include "Collection.h"
#include "emuledlg.h"
#include "SharedFilesWnd.h"
#include "MediaInfo.h"
#include "UploadQueue.h"

// id3lib
#pragma warning(disable:4100) // unreferenced formal parameter
#include <id3/tag.h>
#include <id3/misc_support.h>
#pragma warning(default:4100) // unreferenced formal parameter
extern wchar_t *ID3_GetStringW(const ID3_Frame *frame, ID3_FieldID fldName);

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	META_DATA_VER	1

IMPLEMENT_DYNAMIC(CKnownFile, CAbstractFile)

CKnownFile::CKnownFile()
{
	m_iPartCount = 0;
	m_iED2KPartCount = 0;
	m_iED2KPartHashCount = 0;
	m_tUtcLastModified = (UINT)-1;
	if(thePrefs.GetNewAutoUp()){
		m_iUpPriority = PR_HIGH;
		m_bAutoUpPriority = true;
	}
	else{
		m_iUpPriority = PR_NORMAL;
		m_bAutoUpPriority = false;
	}
	statistic.fileParent = this;
	(void)m_strComment;
	m_PublishedED2K = false;
	kadFileSearchID = 0;
	SetLastPublishTimeKadSrc(0,0);
	m_nCompleteSourcesTime = time(NULL);
	m_nCompleteSourcesCount = 1;
	m_nCompleteSourcesCountLo = 1;
	m_nCompleteSourcesCountHi = 1;
	m_uMetaDataVer = 0;
	m_lastPublishTimeKadSrc = 0;
	m_lastPublishTimeKadNotes = 0;
	m_lastBuddyIP = 0;
	m_pAICHHashSet = new CAICHHashSet(this);
	m_pCollection = NULL;
	m_verifiedFileType=FILETYPE_UNKNOWN;

	ReleaseViaWebCache = false; //JP webcache release // MORPH - Added by Commander, WebCache 1.2e
	//MORPH START - Added by SiRoB, Show Permission
	m_iPermissions = -1;
	//MORPH END   - Added by SiRoB, Show Permission
	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	m_iSpreadbarSetStatus = -1;
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	//MORPH START - Added by SiRoB, HIDEOS
	m_iHideOS = -1;
	m_iSelectiveChunk = -1;
	m_bHideOSAuthorized = true; //MORPH - Added by SiRoB, Avoid misusing of hideOS
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	m_iShareOnlyTheNeed = -1;
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	//MORPH START - Added by SiRoB, Avoid misusing of powershare
	m_bpowershared = false;
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
	//MORPH END   - Added by SiRoB, Reduce SharedStatusBAr CPU consumption
	// MORPH START leuk_he mergeKnown, for TAHO, .met file control
    m_dwLastSeen=time(NULL); // intiliaze if a official is read in to prevent all known files lost.
	// MORPH END leuk_he  mergeKnown, for TAHO, .met file control

	// Mighty Knife: CRC32-Tag
	m_sCRC32 [0] = '\0';
	// [end] Mighty Knife
}

CKnownFile::~CKnownFile()
{
	for (int i = 0; i < hashlist.GetSize(); i++)
		delete[] hashlist[i];
	delete m_pAICHHashSet;
	delete m_pCollection;
	//MORPH START - Added by SiRoB, Reduce SharedStatusBar CPU consumption
	m_bitmapSharedStatusBar.DeleteObject();
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
	(void)m_iPartCount;
	(void)m_iED2KPartCount;
	(void)m_iED2KPartHashCount;
	ASSERT( m_iUpPriority == PR_VERYLOW || m_iUpPriority == PR_LOW || m_iUpPriority == PR_NORMAL || m_iUpPriority == PR_HIGH || m_iUpPriority == PR_VERYHIGH );
	CHECK_BOOL(m_bAutoUpPriority);
	(void)s_ShareStatusBar;
	CHECK_BOOL(m_PublishedED2K);
	(void)kadFileSearchID;
	(void)m_lastPublishTimeKadSrc;
	(void)m_lastPublishTimeKadNotes;
	(void)m_lastBuddyIP;
	(void)wordlist;
}

void CKnownFile::Dump(CDumpContext& dc) const
{
	CAbstractFile::Dump(dc);
}
#endif

CBarShader CKnownFile::s_ShareStatusBar(16);
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

		s_ShareStatusBar.SetFileSize(GetFileSize());
		s_ShareStatusBar.SetHeight(iHeight);
		s_ShareStatusBar.SetWidth(iWidth);

		if(m_ClientUploadList.GetSize() > 0 || m_nCompleteSourcesCountHi > 1) {
			// We have info about chunk frequency in the net, so we will color the chunks we have after perceived availability.
	    	const COLORREF crMissing = RGB(255, 0, 0);
		    s_ShareStatusBar.Fill(crMissing);

			if (!onlygreyrect) {
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

            uint32 tempCompleteSources = m_nCompleteSourcesCountLo;
            if(tempCompleteSources > 0) {
                tempCompleteSources--;
            }

		    for (UINT i = 0; i < GetPartCount(); i++){
					uint32 frequency = tempCompleteSources;
                	if(!m_AvailPartFrequency.IsEmpty()) {
                	    frequency = max(m_AvailPartFrequency[i], tempCompleteSources);
	                }

					if(frequency > 0 ){
				 		COLORREF color = RGB(0, (22*(frequency-1) >= 210)? 0:210-(22*(frequency-1)), 255);
				    s_ShareStatusBar.FillRange(PARTSIZE*(uint64)(i),PARTSIZE*(uint64)(i+1),color);
					}
				}
			}
        } else {
        // We have no info about chunk frequency in the net, so just color the chunk we have as black.
        COLORREF crNooneAsked;
		if(bFlat) { 
		    crNooneAsked = RGB(0, 0, 0);
		} else { 
		    crNooneAsked = RGB(104, 104, 104);
	    }
		s_ShareStatusBar.Fill(crNooneAsked);
		}
		s_ShareStatusBar.Draw(&cdcStatus, 0, 0, bFlat); 
	}
	else
		hOldBitmap = cdcStatus.SelectObject(m_bitmapSharedStatusBar);
	dc->BitBlt(rect->left, rect->top, iWidth, iHeight, &cdcStatus, 0, 0, SRCCOPY);
	cdcStatus.SelectObject(hOldBitmap);
}

// SLUGFILLER: heapsortCompletesrc
static void HeapSort(CArray<uint16,uint16> &count, UINT first, UINT last){
	UINT r;
	for ( r = first; !(r & (UINT)INT_MIN) && (r<<1) < last; ){
		UINT r2 = (r<<1)+1;
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

void CKnownFile::UpdateFileRatingCommentAvail(bool bForceUpdate)
{
	bool bOldHasComment = m_bHasComment;
	UINT uOldUserRatings = m_uUserRating;

	m_bHasComment = false;
	UINT uRatings = 0;
	UINT uUserRatings = 0;

	for(POSITION pos = m_kadNotes.GetHeadPosition(); pos != NULL; )
	{
		Kademlia::CEntry* entry = m_kadNotes.GetNext(pos);
		if (!m_bHasComment && !entry->GetStrTagValue(TAG_DESCRIPTION).IsEmpty())
			m_bHasComment = true;
		UINT rating = (UINT)entry->GetIntTagValue(TAG_FILERATING);
		if(rating!=0)
		{
			uRatings++;
			uUserRatings += rating;
		}
	}

	if(uRatings)
		m_uUserRating = (uint32)ROUND((float)uUserRatings / uRatings);
	else
		m_uUserRating = 0;

	if (bOldHasComment != m_bHasComment || uOldUserRatings != m_uUserRating || bForceUpdate)
		theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
}

void CKnownFile::UpdatePartsInfo()
{
	// Cache part count
	UINT partcount = GetPartCount();
	bool flag = (time(NULL) - m_nCompleteSourcesTime > 0); 
	
	// Reset part counters
	if ((UINT)m_AvailPartFrequency.GetSize() < partcount)
		m_AvailPartFrequency.SetSize(partcount);
	for (UINT i = 0; i < partcount; i++)
		m_AvailPartFrequency[i] = 0;

	CArray<uint16, uint16> count;
	if (flag)
		count.SetSize(0, m_ClientUploadList.GetSize());
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	UINT iCompleteSourcesCountInfoReceived = 0;
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	for (POSITION pos = m_ClientUploadList.GetHeadPosition(); pos != 0; )
	{
		const CUpDownClient* cur_src = m_ClientUploadList.GetNext(pos);
		const uint8* srcstatus = cur_src->GetUpPartStatus();
		//This could be a partfile that just completed.. Many of these clients will not have this information.
		if(srcstatus)
		{
			for (UINT i = 0; i < partcount; i++)
			{
				if (srcstatus[i]&SC_AVAILABLE)
					++m_AvailPartFrequency[i];
			}
			if ( flag )
				count.Add(cur_src->GetUpCompleteSourcesCount());
			//MORPH START - Added by SiRoB, Avoid misusing of powersharing
			if (cur_src->GetUpCompleteSourcesCount()>0)
				++iCompleteSourcesCountInfoReceived;
			//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
		}
		//MORPH START - Added by SiRoB, ShareOnlyTheNeed hide Uploaded and uploading part
		if (cur_src->IsDownloading()){
			uint8* m_abyUpPartUploadingAndUploaded = new uint8[partcount];
			cur_src->GetUploadingAndUploadedPart(m_abyUpPartUploadingAndUploaded, partcount);
			for (UINT i = 0; i < partcount; i++)
				if (m_AvailPartFrequency[i] == 0 && m_abyUpPartUploadingAndUploaded[i]>0)
					m_AvailPartFrequency[i] = 1;
			delete[] m_abyUpPartUploadingAndUploaded;
		}
		//MORPH END    - Added by SiRoB, ShareOnlyTheNeed hide Uploaded and uploading part
	}

	if (flag)
	{
		m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;

		if( partcount > 0)
			m_nCompleteSourcesCount = m_AvailPartFrequency[0];
		for (UINT i = 1; i < partcount; i++)
		{
			if( m_nCompleteSourcesCount > m_AvailPartFrequency[i])
				m_nCompleteSourcesCount = m_AvailPartFrequency[i];
		}
	
		count.Add(m_nCompleteSourcesCount+1); // plus 1 since we have the file complete too
	
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
					m_nCompleteSourcesCountLo = count.GetAt(i);
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
	m_nVirtualCompleteSourcesCount = (UINT)-1;
	for (UINT i = 0; i < partcount; i++){
		if(m_AvailPartFrequency[i] < m_nVirtualCompleteSourcesCount)
			m_nVirtualCompleteSourcesCount = m_AvailPartFrequency[i];
	}
	UpdatePowerShareLimit(m_nVirtualCompleteSourcesCount<=1 || m_nCompleteSourcesCountHi<=GetPartCount(), m_nCompleteSourcesCountHi==1 && m_nVirtualCompleteSourcesCount==0 && iCompleteSourcesCountInfoReceived>1,m_nCompleteSourcesCountHi>((GetPowerShareLimit()>=0)?GetPowerShareLimit():thePrefs.GetPowerShareLimit()));
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by SiRoB, Avoid misusing of HideOS
	m_bHideOSAuthorized = true;
	//MORPH END   - Added by SiRoB, Avoid misusing of HideOS
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
	CKnownFile* pFile = NULL;

	// If this is called within the sharedfiles object during startup,
	// we cannot reference it yet..

	if(theApp.sharedfiles)
		pFile = theApp.sharedfiles->GetFileByID(GetFileHash());

	if (pFile && pFile == this)
		theApp.sharedfiles->RemoveKeywords(this);

	CAbstractFile::SetFileName(pszFileName, bReplaceInvalidFileSystemChars);

	wordlist.clear();
	if(m_pCollection)
	{
		CString sKeyWords;
		sKeyWords.Format(_T("%s %s"), m_pCollection->GetCollectionAuthorKeyString(), GetFileName());
		Kademlia::CSearchManager::GetWords(sKeyWords, &wordlist);
	}
	else
		Kademlia::CSearchManager::GetWords(GetFileName(), &wordlist);

	if (pFile && pFile == this)
		theApp.sharedfiles->AddKeywords(this);
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
	if (!_tmakepathlimit(strFilePath.GetBuffer(MAX_PATH), NULL, in_directory, in_filename, NULL)){
		LogError(GetResString(IDS_ERR_FILEOPEN), in_filename, _T(""));
		return false;
	}
	strFilePath.ReleaseBuffer();
	SetFilePath(strFilePath);
	FILE* file = _tfsopen(strFilePath, _T("rbS"), _SH_DENYNO); // can not use _SH_DENYWR because we may access a completing part file
	if (!file){
		LogError(GetResString(IDS_ERR_FILEOPEN) + _T(" - %s"), strFilePath, _T(""), _tcserror(errno));
		return false;
	}

	// set filesize
	if (_filelengthi64(file->_file) > MAX_EMULE_FILE_SIZE){
		fclose(file);
		return false; // not supported by network
	}
	SetFileSize((uint64)_filelengthi64(file->_file));

	// we are reading the file data later in 8K blocks, adjust the internal file stream buffer accordingly
	setvbuf(file, NULL, _IOFBF, 1024*8*2);

	m_AvailPartFrequency.SetSize(GetPartCount());
	for (UINT i = 0; i < GetPartCount();i++)
		m_AvailPartFrequency[i] = 0;
	
	// create hashset
	uint64 togo = m_nFileSize;
	UINT hashcount;
	for (hashcount = 0; togo >= PARTSIZE; )
	{
		CAICHHashTree* pBlockAICHHashTree = m_pAICHHashSet->m_pHashTree.FindHash((uint64)hashcount*PARTSIZE, PARTSIZE);
		ASSERT( pBlockAICHHashTree != NULL );

		uchar* newhash = new uchar[16];
		if (!CreateHash(file, PARTSIZE, newhash, pBlockAICHHashTree)) {
			LogError(_T("Failed to hash file \"%s\" - %s"), strFilePath, _tcserror(errno));
			fclose(file);
			delete[] newhash;
			return false;
		}

		if (theApp.emuledlg==NULL || !theApp.emuledlg->IsRunning()){ // in case of shutdown while still hashing
			fclose(file);
			delete[] newhash;
			return false;
		}

		hashlist.Add(newhash);
		togo -= PARTSIZE;
		hashcount++;

		if (pvProgressParam && theApp.emuledlg && theApp.emuledlg->IsRunning()){
			ASSERT( ((CKnownFile*)pvProgressParam)->IsKindOf(RUNTIME_CLASS(CKnownFile)) );
			ASSERT( ((CKnownFile*)pvProgressParam)->GetFileSize() == GetFileSize() );
			UINT uProgress = (UINT)(uint64)(((uint64)(GetFileSize() - togo) * 100) / GetFileSize());
			ASSERT( uProgress <= 100 );
			VERIFY( PostMessage(theApp.emuledlg->GetSafeHwnd(), TM_FILEOPPROGRESS, uProgress, (LPARAM)pvProgressParam) );
		}
	}

	CAICHHashTree* pBlockAICHHashTree;
	if (togo == 0)
		pBlockAICHHashTree = NULL; // sha hashtree doesnt takes hash of 0-sized data
	else{
		pBlockAICHHashTree = m_pAICHHashSet->m_pHashTree.FindHash((uint64)hashcount*PARTSIZE, togo);
		ASSERT( pBlockAICHHashTree != NULL );
	}
	
	uchar* lasthash = new uchar[16];
	md4clr(lasthash);
	if (!CreateHash(file, togo, lasthash, pBlockAICHHashTree)) {
		LogError(_T("Failed to hash file \"%s\" - %s"), strFilePath, _tcserror(errno));
		fclose(file);
		delete[] lasthash;
		return false;
	}
	
	m_pAICHHashSet->ReCalculateHash(false);
	if (m_pAICHHashSet->VerifyHashTree(true))
	{
		m_pAICHHashSet->SetStatus(AICH_HASHSETCOMPLETE);
		if (!m_pAICHHashSet->SaveHashSet())
			LogError(LOG_STATUSBAR, GetResString(IDS_SAVEACFAILED));
	}
	else{
		// now something went pretty wrong
// WebCache ////////////////////////////////////////////////////////////////////////////////////
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
		DebugLogError(LOG_STATUSBAR, _T("Failed to calculate AICH Hashset from file %s"), GetFileName());
	}

	hashlist.Add(lasthash);		// SLUGFILLER: SafeHash - better handling of single-part files
	if (!hashcount){
		md4cpy(m_abyFileHash, lasthash);
		// SLUGFILLER: SafeHash remove - removed delete
	} 
	else {
		// SLUGFILLER: SafeHash remove - moved up
		uchar* buffer = new uchar[hashlist.GetCount()*16];
		for (int i = 0; i < hashlist.GetCount(); i++)
			md4cpy(buffer+(i*16), hashlist[i]);
		CreateHash(buffer, hashlist.GetCount()*16, m_abyFileHash);
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
	struct _stat fileinfo;
	if (_fstat(file->_file, &fileinfo) == 0){
		m_tUtcLastModified = fileinfo.st_mtime;
		AdjustNTFSDaylightFileTime(m_tUtcLastModified, strFilePath);
	}
	//Morph Start - Added by Stulle, Equal Chance For Each File
	statistic.SetSessionShareTime();
	//Morph End - Added by Stulle, Equal Chance For Each File

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
		AddLogLine(false, GetResString(IDS_HASHING_COMPLETED), hashfilename);
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
		LogError(GetResString(IDS_ERR_FILEOPEN) + _T(" - %s"), GetFilePath(), _T(""), _tcserror(errno));
		return false;
	}
	// we are reading the file data later in 8K blocks, adjust the internal file stream buffer accordingly
	setvbuf(file, NULL, _IOFBF, 1024*8*2);

	// create aichhashset
	uint64 togo = m_nFileSize;
	UINT hashcount;
	for (hashcount = 0; togo >= PARTSIZE; )
	{
		CAICHHashTree* pBlockAICHHashTree = m_pAICHHashSet->m_pHashTree.FindHash((uint64)hashcount*PARTSIZE, PARTSIZE);
		ASSERT( pBlockAICHHashTree != NULL );
		if (!CreateHash(file, PARTSIZE, NULL, pBlockAICHHashTree)) {
			LogError(_T("Failed to hash file \"%s\" - %s"), GetFilePath(), _tcserror(errno));
			fclose(file);
			return false;
		}

		if (theApp.emuledlg==NULL || !theApp.emuledlg->IsRunning()){ // in case of shutdown while still hashing
			fclose(file);
			return false;
		}

		togo -= PARTSIZE;
		hashcount++;
	}

	if (togo != 0)
	{
		CAICHHashTree* pBlockAICHHashTree = m_pAICHHashSet->m_pHashTree.FindHash((uint64)hashcount*PARTSIZE, togo);
		ASSERT( pBlockAICHHashTree != NULL );
		if (!CreateHash(file, togo, NULL, pBlockAICHHashTree)) {
			LogError(_T("Failed to hash file \"%s\" - %s"), GetFilePath(), _tcserror(errno));
			fclose(file);
			return false;
		}
	}

	m_pAICHHashSet->ReCalculateHash(false);
	if (m_pAICHHashSet->VerifyHashTree(true))
	{
		m_pAICHHashSet->SetStatus(AICH_HASHSETCOMPLETE);
		if (!m_pAICHHashSet->SaveHashSet())
			LogError(LOG_STATUSBAR, GetResString(IDS_SAVEACFAILED));
	}
	else{
		// now something went pretty wrong
// WebCache ////////////////////////////////////////////////////////////////////////////////////
		if(thePrefs.GetLogICHEvents()) //JP log ICH events
		DebugLogError(LOG_STATUSBAR, _T("Failed to calculate AICH Hashset from file %s"), GetFileName());
	}

	fclose(file);
	file = NULL;

	return true;	
}

void CKnownFile::SetFileSize(EMFileSize nFileSize)
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

	if (nFileSize == (uint64)0){
		ASSERT(0);
		m_iPartCount = 0;
		m_iED2KPartCount = 0;
		m_iED2KPartHashCount = 0;
		return;
	}

	// nr. of data parts
	ASSERT( (uint64)(((uint64)nFileSize + (PARTSIZE - 1)) / PARTSIZE) <= (UINT)USHRT_MAX );
	m_iPartCount = (uint16)(((uint64)nFileSize + (PARTSIZE - 1)) / PARTSIZE);

	// nr. of parts to be used with OP_FILESTATUS
	m_iED2KPartCount = (uint16)((uint64)nFileSize / PARTSIZE + 1);

	// nr. of parts to be used with OP_HASHSETANSWER
	m_iED2KPartHashCount = (uint16)((uint64)nFileSize / PARTSIZE);
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
		if (parts != GetED2KPartCount()){	// SLUGFILLER: SafeHash - use GetED2KPartCount
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
		CreateHash(buffer, hashlist.GetCount()*16, checkid);
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
	CreateHash(buffer, hashlist.GetCount()*16, aucHashsetHash);
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
	statistic.SetSessionShareTime();//Morph - Added by Stulle, Equal Chance For Each File
	//MORPH START - Added by SiRoB, SLUGFILLER: Spreadbars
	// SLUGFILLER: Spreadbars
	CMap<UINT,UINT,uint64,uint64> spread_start_map;
	CMap<UINT,UINT,uint64,uint64> spread_end_map;
	CMap<UINT,UINT,uint64,uint64> spread_count_map;
	// SLUGFILLER: Spreadbars
	//MORPH END - Added by SiRoB, SLUGFILLER: Spreadbars
	UINT tagcount = file->ReadUInt32();
	for (UINT j = 0; j < tagcount; j++){
		CTag* newtag = new CTag(file, false);
		switch (newtag->GetNameID()){
			case FT_FILENAME:{
				ASSERT( newtag->IsStr() );
				if (newtag->IsStr()){
					if (GetFileName().IsEmpty())
						SetFileName(newtag->GetStr());
				}
				delete newtag;
				break;
			}
			case FT_FILESIZE:{
				ASSERT( newtag->IsInt64(true) );
				if (newtag->IsInt64(true))
				{
					SetFileSize(newtag->GetInt64());
					m_AvailPartFrequency.SetSize(GetPartCount());
					for (UINT i = 0; i < GetPartCount();i++)
						m_AvailPartFrequency[i] = 0;
				}
				delete newtag;
				break;
			}
			case FT_ATTRANSFERRED:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					statistic.alltimetransferred = newtag->GetInt();
				delete newtag;
				break;
			}
			case FT_ATTRANSFERREDHI:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					statistic.alltimetransferred = ((uint64)newtag->GetInt() << 32) | (UINT)statistic.alltimetransferred;
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
					m_iUpPriority = (uint8)newtag->GetInt();
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
					SetLastPublishTimeKadSrc( newtag->GetInt(), 0 );
				if(GetLastPublishTimeKadSrc() > (uint32)time(NULL)+KADEMLIAREPUBLISHTIMES)
				{
					//There may be a posibility of an older client that saved a random number here.. This will check for that..
					SetLastPublishTimeKadSrc(0,0);
				}
				delete newtag;
				break;
			}
			case FT_KADLASTPUBLISHNOTES:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					SetLastPublishTimeKadNotes( newtag->GetInt() );
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
				if (DecodeBase32(newtag->GetStr(),hash) == (UINT)CAICHHash::GetHashSize())
					m_pAICHHashSet->SetMasterHash(hash, AICH_HASHSETCOMPLETE);
				else
					ASSERT( false );
				delete newtag;
				break;
			}
			// SLUGFILLER: mergeKnown, for TAHO, .met file control
			case FT_LASTSEENCOMPLETE:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					m_dwLastSeen = newtag->GetInt();
				delete newtag;
				break;
			}
			// SLUGFILLER: mergeKnown, for TAHO, .met file control
			default:
				//MORPH START - Changed by SiRoB, SLUGFILLER: Spreadbars
				// Mighty Knife: Take care of corrupted tags !!!
				if(!newtag->GetNameID() && newtag->IsInt64(true) && newtag->GetName()){
				// [end] Mighty Knife
					UINT spreadkey = atoi(&newtag->GetName()[1]);
					if (newtag->GetName()[0] == FT_SPREADSTART)
						spread_start_map.SetAt(spreadkey, newtag->GetInt64());
					else if (newtag->GetName()[0] == FT_SPREADEND)
						spread_end_map.SetAt(spreadkey, newtag->GetInt64());
					else if (newtag->GetName()[0] == FT_SPREADCOUNT)
						spread_count_map.SetAt(spreadkey, newtag->GetInt64());
					//MORPH START - Added by SiRoB, ZZ Upload System				
					else if(CmpED2KTagName(newtag->GetName(), FT_POWERSHARE) == 0)
						//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
						//SetPowerShared(newtag->GetInt() == 1);
						SetPowerShared((newtag->GetInt()<=3)?newtag->GetInt():-1);
						//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
					//MORPH END   - Added by SiRoB, ZZ Upload System
					//MORPH START - Added by SiRoB, POWERSHARE Limit
					else if(CmpED2KTagName(newtag->GetName(), FT_POWERSHARE_LIMIT) == 0)
						SetPowerShareLimit((newtag->GetInt()<=200)?newtag->GetInt():-1);
					//MORPH END   - Added by SiRoB, POWERSHARE Limit
					//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
					else if(CmpED2KTagName(newtag->GetName(), FT_SPREADBAR) == 0)
						SetSpreadbarSetStatus(newtag->GetInt());
					//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
					//MORPH START - Added by SiRoB, HIDEOS
					else if(CmpED2KTagName(newtag->GetName(), FT_HIDEOS) == 0)
						SetHideOS((newtag->GetInt()<=10)?newtag->GetInt():-1);
					else if(CmpED2KTagName(newtag->GetName(), FT_SELECTIVE_CHUNK) == 0)
						SetSelectiveChunk(newtag->GetInt()<=1?newtag->GetInt():-1);
					//MORPH END   - Added by SiRoB, HIDEOS
				//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
					else if(CmpED2KTagName(newtag->GetName(), FT_SHAREONLYTHENEED) == 0)
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
		UINT spreadkey;
		uint64 spread_start;
		uint64 spread_end;
		uint64 spread_count;
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

	return m_nFileSize!=(uint64)0;		// SLUGFILLER: SafeHash - Must have a filesize tag
}

bool CKnownFile::LoadDateFromFile(CFileDataIO* file){
	m_tUtcLastModified = file->ReadUInt32();
	//Morph Start - Added by Stulle, Equal Chance For Each File
	statistic.SetSessionShareTime();
	//Morph End - Added by Stulle, Equal Chance For Each File
	return true;
}

bool CKnownFile::LoadFromFile(CFileDataIO* file){
	// SLUGFILLER: SafeHash - load first, verify later
	bool ret1 = LoadDateFromFile(file);
	bool ret2 = LoadHashsetFromFile(file,false);
	bool ret3 = LoadTagsFromFile(file);
	UpdatePartsInfo();
	if (GetED2KPartCount() <= 1) {	// ignore loaded hash for 1-chunk files
		for (int i = 0; i < hashlist.GetSize(); i++)
			delete[] hashlist[i];
		hashlist.RemoveAll();
		uchar* cur_hash = new uchar[16];
		md4cpy(cur_hash, m_abyFileHash);
		hashlist.Add(cur_hash);
		ret2 = true;
	} else if (GetED2KPartCount()!=GetHashCount())
		ret2 = false;	// Final hash-count verification, needs to be done after the tags are loaded.
	return ret1 && ret2 && ret3;
	// SLUGFILLER: SafeHash
}

bool CKnownFile::WriteToFile(CFileDataIO* file)
{
	// date
	file->WriteUInt32(m_tUtcLastModified);

	// hashset
	file->WriteHash16(m_abyFileHash);
	UINT parts = hashlist.GetCount();
	file->WriteUInt16((uint16)parts);
	for (UINT i = 0; i < parts; i++)
		file->WriteHash16(hashlist[i]);

	uint32 uTagCount = 0;
	ULONG uTagCountFilePos = (ULONG)file->GetPosition();
	file->WriteUInt32(uTagCount);

	if (WriteOptED2KUTF8Tag(file, GetFileName(), FT_FILENAME))
		uTagCount++;
	CTag nametag(FT_FILENAME, GetFileName());
	nametag.WriteTagToFile(file);
	uTagCount++;
	
	CTag sizetag(FT_FILESIZE, m_nFileSize, IsLargeFile());
	sizetag.WriteTagToFile(file);
	uTagCount++;
	
	// statistic
	if (statistic.alltimetransferred){
		CTag attag1(FT_ATTRANSFERRED, (uint32)statistic.alltimetransferred);
		attag1.WriteTagToFile(file);
		uTagCount++;
		
		CTag attag4(FT_ATTRANSFERREDHI, (uint32)(statistic.alltimetransferred >> 32));
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

	if (m_lastPublishTimeKadNotes){
		CTag kadLastPubNotes(FT_KADLASTPUBLISHNOTES, m_lastPublishTimeKadNotes);
		kadLastPubNotes.WriteTagToFile(file);
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

	// SLUGFILLER: mergeKnown, for TAHO, .met file control
	CTag lsctag(FT_LASTSEENCOMPLETE, m_dwLastSeen);
	lsctag.WriteTagToFile(file);
	uTagCount++;
	// SLUGFILLER: mergeKnown, for TAHO, .met file control

	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	if (GetSpreadbarSetStatus()>=0) {
		CTag spreadBar(FT_SPREADBAR, GetSpreadbarSetStatus());
		spreadBar.WriteTagToFile(file);
        uTagCount++;
	}
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file

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
	if(GetSpreadbarSetStatus() > 0 || (GetSpreadbarSetStatus() == -1 ? thePrefs.GetSpreadbarSetStatus() > 0 : false)){//MORPH	- Added by AndCycle, SLUGFILLER: Spreadbars - per file

		char namebuffer[10];
		char* number = &namebuffer[1];
		UINT i_pos = 0;
		if (IsLargeFile()) {
			uint64 hideOS = GetHideOS()>=0?GetHideOS():thePrefs.GetHideOvershares();
			for (POSITION pos = statistic.spreadlist.GetHeadPosition(); pos; ){
				uint64 count = statistic.spreadlist.GetValueAt(pos);
				if (!count) {
					statistic.spreadlist.GetNext(pos);
					continue;
				}
				uint64 start = statistic.spreadlist.GetKeyAt(pos);
				statistic.spreadlist.GetNext(pos);
				ASSERT(pos != NULL);	// Last value should always be 0
				uint64 end = statistic.spreadlist.GetKeyAt(pos);
				//MORPH - Smooth sample
				if (end - start < EMBLOCKSIZE && count > hideOS)
					continue;
				//MORPH - Smooth sample
				itoa(i_pos,number,10);
				namebuffer[0] = FT_SPREADSTART;
				CTag(namebuffer,start,true).WriteTagToFile(file);
				namebuffer[0] = FT_SPREADEND;
				CTag(namebuffer,end,true).WriteTagToFile(file);
				namebuffer[0] = FT_SPREADCOUNT;
				CTag(namebuffer,count,true).WriteTagToFile(file);
				uTagCount+=3;
				i_pos++;
			}
		} else {
			uint32 hideOS = GetHideOS()>=0?GetHideOS():thePrefs.GetHideOvershares();
			for (POSITION pos = statistic.spreadlist.GetHeadPosition(); pos; ){
				uint32 count = (uint32)statistic.spreadlist.GetValueAt(pos);
				if (!count) {
					statistic.spreadlist.GetNext(pos);
					continue;
				}
				uint32 start = (uint32)statistic.spreadlist.GetKeyAt(pos);
				statistic.spreadlist.GetNext(pos);
				ASSERT(pos != NULL);	// Last value should always be 0
				uint32 end = (uint32)statistic.spreadlist.GetKeyAt(pos);
				//MORPH - Smooth sample
				if (end - start < EMBLOCKSIZE && count > hideOS)
					continue;
				//MORPH - Smooth sample
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
		}

	}//MORPH	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
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

void CKnownFile::CreateHash(CFile* pFile, uint64 Length, uchar* pMd4HashOut, CAICHHashTree* pShaHashOut) const
{
	ASSERT( pFile != NULL );
	ASSERT( pMd4HashOut != NULL || pShaHashOut != NULL );
	CSingleLock sLock1(&(theApp.hashing_mut), TRUE);	// SLUGFILLER: SafeHash - only one chunk-hash at a time

	uint64  Required = Length;
	uchar   X[64*128];
	uint64	posCurrentEMBlock = 0;
	uint64	nIACHPos = 0;
	CAICHHashAlgo* pHashAlg = m_pAICHHashSet->GetNewHashAlgo();
	CMD4 md4;

	while (Required >= 64){
        uint32 len; 
        if ((Required / 64) > sizeof(X)/(64 * sizeof(X[0]))) 
             len = sizeof(X)/(64 * sizeof(X[0])); 
		else
			len = (uint32)Required / 64;
		pFile->Read(X, len*64);

		// SHA hash needs 180KB blocks
		if (pShaHashOut != NULL){
			if (nIACHPos + len*64 >= EMBLOCKSIZE){
				uint32 nToComplete = (uint32)(EMBLOCKSIZE - nIACHPos);
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

		if (pMd4HashOut != NULL){
			md4.Add(X, len*64);
		}
		Required -= len*64;
	}

	Required = Length % 64;
	if (Required != 0){
		pFile->Read(X, (uint32)Required);

		if (pShaHashOut != NULL){
			if (nIACHPos + Required >= EMBLOCKSIZE){
				uint32 nToComplete = (uint32)(EMBLOCKSIZE - nIACHPos);
				pHashAlg->Add(X, nToComplete);
				ASSERT( nIACHPos + nToComplete == EMBLOCKSIZE );
				pShaHashOut->SetBlockHash(EMBLOCKSIZE, posCurrentEMBlock, pHashAlg);
				posCurrentEMBlock += EMBLOCKSIZE;
				pHashAlg->Reset();
				pHashAlg->Add(X+nToComplete, (uint32)(Required - nToComplete));
				nIACHPos = Required - nToComplete;
			}
			else{
				pHashAlg->Add(X, (uint32)Required);
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

	if (pMd4HashOut != NULL){
		md4.Add(X, (uint32)Required);
		md4.Finish();
		md4cpy(pMd4HashOut, md4.GetHash());
	}

	delete pHashAlg;
}

bool CKnownFile::CreateHash(FILE* fp, uint64 uSize, uchar* pucHash, CAICHHashTree* pShaHashOut) const
{
	bool bResult = false;
	CStdioFile file(fp);
	try
	{
		CreateHash(&file, uSize, pucHash, pShaHashOut);
		bResult = true;
	}
	catch(CFileException* ex)
	{
		ex->Delete();
	}
	return bResult;
}

bool CKnownFile::CreateHash(const uchar* pucData, uint32 uSize, uchar* pucHash, CAICHHashTree* pShaHashOut) const
{
	bool bResult = false;
	CMemFile file(const_cast<uchar*>(pucData), uSize);
	try
	{
		CreateHash(&file, uSize, pucHash, pShaHashOut);
		bResult = true;
	}
	catch(CFileException* ex)
	{
		ex->Delete();
	}
	return bResult;
}

uchar* CKnownFile::GetPartHash(UINT part) const
{
	if (part >= (UINT)hashlist.GetCount())
		return NULL;
	return hashlist[part];
}

Packet*	CKnownFile::CreateSrcInfoPacket(const CUpDownClient* forClient, uint8 byRequestedVersion, uint16 nRequestedOptions) const
{
	if (m_ClientUploadList.IsEmpty())
		return NULL;

	if (md4cmp(forClient->GetUploadFileID(), GetFileHash())!=0) {
		// should never happen
		DEBUG_ONLY( DebugLogError(_T("*** %hs - client (%s) upload file \"%s\" does not match file \"%s\""), __FUNCTION__, forClient->DbgGetClientInfo(), DbgGetFileInfo(forClient->GetUploadFileID()), GetFileName()) );
		ASSERT(0);
		return NULL;
	}

	// check whether client has either no download status at all or a download status which is valid for this file
	if (   !(forClient->GetUpPartCount()==0 && forClient->GetUpPartStatus()==NULL)
		&& !(forClient->GetUpPartCount()==GetPartCount() && forClient->GetUpPartStatus()!=NULL)) {
		// should never happen
		DEBUG_ONLY( DebugLogError(_T("*** %hs - part count (%u) of client (%s) does not match part count (%u) of file \"%s\""), __FUNCTION__, forClient->GetUpPartCount(), forClient->DbgGetClientInfo(), GetPartCount(), GetFileName()) );
		ASSERT(0);
		return NULL;
	}

	CSafeMemFile data(1024);
	
	uint8 byUsedVersion;
	bool bIsSX2Packet;
	if (forClient->SupportsSourceExchange2() && byRequestedVersion > 0){
		// the client uses SourceExchange2 and requested the highest version he knows
		// and we send the highest version we know, but of course not higher than his request
		byUsedVersion = min(byRequestedVersion, (uint8)SOURCEEXCHANGE2_VERSION);
		bIsSX2Packet = true;
		data.WriteUInt8(byUsedVersion);

		// we don't support any special SX2 options yet, reserved for later use
		if (nRequestedOptions != 0)
			DebugLogWarning(_T("Client requested unknown options for SourceExchange2: %u (%s)"), nRequestedOptions, forClient->DbgGetClientInfo());
	}
	else{
		byUsedVersion = forClient->GetSourceExchange1Version();
		bIsSX2Packet = false;
		if (forClient->SupportsSourceExchange2())
			DebugLogWarning(_T("Client which announced to support SX2 sent SX1 packet instead (%s)"), forClient->DbgGetClientInfo());
	}

	uint16 nCount = 0;
	data.WriteHash16(forClient->GetUploadFileID());
	data.WriteUInt16(nCount);
	uint32 cDbgNoSrc = 0;
	for (POSITION pos = m_ClientUploadList.GetHeadPosition(); pos != 0; )
	{
		const CUpDownClient *cur_src = m_ClientUploadList.GetNext(pos);
		if (cur_src->HasLowID() || cur_src == forClient || !(cur_src->GetUploadState() == US_UPLOADING || cur_src->GetUploadState() == US_ONUPLOADQUEUE))
			continue;
		if (!cur_src->IsEd2kClient())
			continue;

		bool bNeeded = false;
		const uint8* rcvstatus = forClient->GetUpPartStatus();
		if (rcvstatus)
		{
			ASSERT( forClient->GetUpPartCount() == GetPartCount() );
			const uint8* srcstatus = cur_src->GetUpPartStatus();
			// MORPH - Modified by Commander, WebCache 1.2e
			if( srcstatus && (!forClient->SupportsWebCache() && !cur_src->SupportsWebCache())) // Superlexx - IFP - if both clients do support webcache, then they have IFP and might find empty sources useful; send that source even if they both are not behind same proxy to improve found webcache-enabled source number on those clients
			{
				ASSERT( cur_src->GetUpPartCount() == GetPartCount() );
				if (cur_src->GetUpPartCount() == forClient->GetUpPartCount())
				{
					for (UINT x = 0; x < GetPartCount(); x++)
					{
						//MORPH - Changed by SiRoB, See chunk that we hide
						/*
						if (srcstatus[x]== && !rcvstatus[x])
						*/
						if (srcstatus[x]&SC_AVAILABLE && !(rcvstatus[x]&SC_AVAILABLE))
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
					//if (thePrefs.GetVerbose())
						DEBUG_ONLY( DebugLogError(_T("*** %hs - found source (%s) with wrong part count (%u) attached to file \"%s\" (partcount=%u)"), __FUNCTION__, cur_src->DbgGetClientInfo(), cur_src->GetUpPartCount(), GetFileName(), GetPartCount()));
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
			ASSERT( forClient->GetUpPartCount() == 0 );
			TRACE(_T("%hs, requesting client has no chunk status - %s"), __FUNCTION__, forClient->DbgGetClientInfo());
			// remote client does not support upload chunk status, search sources which have at least one complete part
			// we could even sort the list of sources by available chunks to return as much sources as possible which
			// have the most available chunks. but this could be a noticeable performance problem.
			const uint8* srcstatus = cur_src->GetUpPartStatus();
			if (srcstatus)
			{
				ASSERT( cur_src->GetUpPartCount() == GetPartCount() );
				for (UINT x = 0; x < GetPartCount(); x++ )
				{
					//MORPH - Changed by SiRoB, See chunk that we hide
					/*
					if (srcstatus[x])
					*/
					if (srcstatus[x]&SC_AVAILABLE)
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

		if (bNeeded)
		{
			nCount++;
			uint32 dwID;
			if (byUsedVersion >= 3)
				dwID = cur_src->GetUserIDHybrid();
			else
				dwID = cur_src->GetIP();
		    data.WriteUInt32(dwID);
		    data.WriteUInt16(cur_src->GetUserPort());
		    data.WriteUInt32(cur_src->GetServerIP());
		    data.WriteUInt16(cur_src->GetServerPort());
			if (byUsedVersion >= 2)
			    data.WriteHash16(cur_src->GetUserHash());
			if (byUsedVersion >= 4){
				// CryptSettings - SourceExchange V4
				// 5 Reserved (!)
				// 1 CryptLayer Required
				// 1 CryptLayer Requested
				// 1 CryptLayer Supported
				const uint8 uSupportsCryptLayer	= cur_src->SupportsCryptLayer() ? 1 : 0;
				const uint8 uRequestsCryptLayer	= cur_src->RequestsCryptLayer() ? 1 : 0;
				const uint8 uRequiresCryptLayer	= cur_src->RequiresCryptLayer() ? 1 : 0;
				const uint8 byCryptOptions = (uRequiresCryptLayer << 2) | (uRequestsCryptLayer << 1) | (uSupportsCryptLayer << 0);
				data.WriteUInt8(byCryptOptions);
			}
			if (nCount > 500)
				break;
		}
	}
	TRACE(_T("%hs: Out of %u clients, %u had no valid chunk status\n"), __FUNCTION__, m_ClientUploadList.GetCount(), cDbgNoSrc);
	if (!nCount)
		return 0;
	data.Seek(bIsSX2Packet ? 17 : 16, SEEK_SET);
	data.WriteUInt16((uint16)nCount);

	Packet* result = new Packet(&data, OP_EMULEPROT);
	result->opcode = bIsSX2Packet ? OP_ANSWERSOURCES2 : OP_ANSWERSOURCES;
	// (1+)16+2+501*(4+2+4+2+16+1) = 14547 (14548) bytes max.
	if ( result->size > 354 )
		result->PackPacket();
	if (thePrefs.GetDebugSourceExchange())
		AddDebugLogLine(false, _T("SXSend: Client source response SX2=%s, Version=%u; Count=%u, %s, File=\"%s\""), bIsSX2Packet ? _T("Yes") : _T("No"), byUsedVersion, nCount, forClient->DbgGetClientInfo(), GetFileName());
	return result;
}

void CKnownFile::SetFileComment(LPCTSTR pszComment)
{
	if (m_strComment.Compare(pszComment) != 0)
	{
		SetLastPublishTimeKadNotes(0);
		CIni ini(thePrefs.GetFileCommentsFilePath(), md4str(GetFileHash()));
		ini.WriteStringUTF8(_T("Comment"), pszComment);
		m_strComment = pszComment;

		for (POSITION pos = m_ClientUploadList.GetHeadPosition();pos != 0;)
			m_ClientUploadList.GetNext(pos)->SetCommentDirty();
	}
}

void CKnownFile::SetFileRating(UINT uRating)
{
	if (m_uRating != uRating)
	{
		SetLastPublishTimeKadNotes(0);
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
			//MORPH - Changed by SiRoB, No need to force saving part.met optimization
			/*
			SetUpPriority( PR_LOW);
			*/
			SetUpPriority( PR_LOW , false);
			theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
		}
		return;
	}
	if ( GetQueuedCount() > 1 ){
		if( GetUpPriority() != PR_NORMAL ){
			//MORPH - Changed by SiRoB, No need to force saving part.met optimization
			/*
			SetUpPriority( PR_NORMAL );
			*/
			SetUpPriority( PR_NORMAL , false);
			theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
		}
		return;
	}
	if( GetUpPriority() != PR_HIGH){
		//MORPH - Changed by SiRoB, No need to force saving part.met optimization
		/*
		SetUpPriority( PR_HIGH );
		*/
		SetUpPriority( PR_HIGH , false);
		theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
	}
}

void CKnownFile::SetUpPriority(uint8 iNewUpPriority, bool bSave)
{
	m_iUpPriority = iNewUpPriority;
	ASSERT( m_iUpPriority == PR_VERYLOW || m_iUpPriority == PR_LOW || m_iUpPriority == PR_NORMAL || m_iUpPriority == PR_HIGH || m_iUpPriority == PR_VERYHIGH );

	if( IsPartFile() && bSave )
		((CPartFile*)this)->SavePartFile();

    //MORPH START - Added by SiRoB, Power Share
	if(GetPowerShared()) {
        theApp.uploadqueue->ReSortUploadSlots(true);
	}
	//MORPH END   - Added by SiRoB, Power Share
}

//MOPRH - Added by SiRoB, Keep Permission flag
void CKnownFile::SetPermissions(int iNewPermissions)
{
	// Mighty Knife: Community visible filelist
	ASSERT( m_iPermissions == -1 || m_iPermissions == PERM_ALL || m_iPermissions == PERM_FRIENDS || m_iPermissions == PERM_NOONE || m_iPermissions == PERM_COMMUNITY);
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

CStringA GetED2KAudioCodec(WORD wFormatTag)
{
	if (wFormatTag == 0x0055)
		return "mp3";
	else if (wFormatTag == 0x0130)
		return "sipr";
	else if (wFormatTag == 0x2000)
		return "ac3";
	else if (wFormatTag == 0x2004)
		return "cook";
	return "";
}

CStringA GetED2KVideoCodec(DWORD biCompression)
{
	if (biCompression == BI_RGB)
		return "rgb";
	else if (biCompression == BI_RLE8)
		return "rle8";
	else if (biCompression == BI_RLE4)
		return "rle4";
	else if (biCompression == BI_BITFIELDS)
		return "bitfields";

	LPCSTR pszCompression = (LPCSTR)&biCompression;
	for (int i = 0; i < 4; i++)
	{
		if (   !isalnum((unsigned char)pszCompression[i])
			&& pszCompression[i] != '.' 
			&& pszCompression[i] != '_' 
			&& pszCompression[i] != ' ')
			return "";
	}

	CStringA strCodec;
	memcpy(strCodec.GetBuffer(4), &biCompression, 4);
	strCodec.ReleaseBuffer(4);
	strCodec.Trim();
	if (strCodec.GetLength() < 2)
		return "";
	strCodec.MakeLower();
	return strCodec;
}

SMediaInfo *GetRIFFMediaInfo(LPCTSTR pszFullPath)
{
	bool bIsAVI;
	SMediaInfo *mi = new SMediaInfo;
	if (!GetRIFFHeaders(pszFullPath, mi, bIsAVI)) {
		delete mi;
		return NULL;
	}
	return mi;
}

SMediaInfo *GetRMMediaInfo(LPCTSTR pszFullPath)
{
	bool bIsRM;
	SMediaInfo *mi = new SMediaInfo;
	if (!GetRMHeaders(pszFullPath, mi, bIsRM)) {
		delete mi;
		return NULL;
	}
	return mi;
}

// Max. string length which is used for string meta tags like TAG_MEDIA_TITLE, TAG_MEDIA_ARTIST, ...
#define	MAX_METADATA_STR_LEN	80

void TruncateED2KMetaData(CString& rstrData)
{
	rstrData.Trim();
	if (rstrData.GetLength() > MAX_METADATA_STR_LEN)
	{
		rstrData.Truncate(MAX_METADATA_STR_LEN);
		rstrData.Trim();
	}
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
		if (_tmakepathlimit(szFullPath, NULL, GetPath(), GetFileName(), NULL)){
		try{
				// ID3LIB BUG: If there are ID3v2 _and_ ID3v1 tags available, id3lib
				// destroys (actually corrupts) the Unicode strings from ID3v2 tags due to
				// converting Unicode to ASCII and then convertion back from ASCII to Unicode.
				// To prevent this, we force the reading of ID3v2 tags only, in case there are 
				// also ID3v1 tags available.
				ID3_Tag myTag;
				CStringA strFilePathA(szFullPath);
				size_t id3Size = myTag.Link(strFilePathA, ID3TT_ID3V2);
				if (id3Size == 0) {
					myTag.Clear();
					myTag.Link(strFilePathA, ID3TT_ID3V1);
				}

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
					UINT uBitrate = (mp3info->vbr_bitrate ? mp3info->vbr_bitrate : mp3info->bitrate) / 1000;
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
							wchar_t* pszText = ID3_GetStringW(frame, ID3FN_TEXT);
							CString strText(pszText);
							TruncateED2KMetaData(strText);
							if (!strText.IsEmpty()){
								CTag* pTag = new CTag(FT_MEDIA_ARTIST, strText);
								AddTagUnique(pTag);
								m_uMetaDataVer = META_DATA_VER;
							}
							delete[] pszText;
							break;
						}
						case ID3FID_ALBUM:{
								wchar_t* pszText = ID3_GetStringW(frame, ID3FN_TEXT);
							CString strText(pszText);
							TruncateED2KMetaData(strText);
							if (!strText.IsEmpty()){
								CTag* pTag = new CTag(FT_MEDIA_ALBUM, strText);
								AddTagUnique(pTag);
								m_uMetaDataVer = META_DATA_VER;
							}
							delete[] pszText;
							break;
						}
						case ID3FID_TITLE:{
							wchar_t* pszText = ID3_GetStringW(frame, ID3FN_TEXT);
							CString strText(pszText);
							TruncateED2KMetaData(strText);
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
	}
	else
	{
		TCHAR szFullPath[MAX_PATH];

		if (_tmakepathlimit(szFullPath, NULL, GetPath(), GetFileName(), NULL))
		{

		SMediaInfo* mi = NULL;
		try
		{
				mi = GetRIFFMediaInfo(szFullPath);
				if (mi == NULL)
					mi = GetRMMediaInfo(szFullPath);
				if (mi)
			{
					mi->InitFileLength();
					UINT uLengthSec = (UINT)mi->fFileLengthSec;

				CStringA strCodec;
				uint32 uBitrate = 0;
					if (mi->iVideoStreams) {
						strCodec = GetED2KVideoCodec(mi->video.bmiHeader.biCompression);
					uBitrate = (mi->video.dwBitRate + 500) / 1000;
				}
					else if (mi->iAudioStreams) {
						strCodec = GetED2KAudioCodec(mi->audio.wFormatTag);
					uBitrate = (DWORD)(((mi->audio.nAvgBytesPerSec * 8.0) + 500.0) / 1000.0);
				}

				if (uLengthSec) {
					CTag* pTag = new CTag(FT_MEDIA_LENGTH, (uint32)uLengthSec);
					AddTagUnique(pTag);
					m_uMetaDataVer = META_DATA_VER;
				}

				if (!strCodec.IsEmpty()) {
					CTag* pTag = new CTag(FT_MEDIA_CODEC, CString(strCodec));
					AddTagUnique(pTag);
					m_uMetaDataVer = META_DATA_VER;
				}

				if (uBitrate) {
					CTag* pTag = new CTag(FT_MEDIA_BITRATE, (uint32)uBitrate);
					AddTagUnique(pTag);
					m_uMetaDataVer = META_DATA_VER;
				}

					TruncateED2KMetaData(mi->strTitle);
					if (!mi->strTitle.IsEmpty()){
						CTag* pTag = new CTag(FT_MEDIA_TITLE, mi->strTitle);
						AddTagUnique(pTag);
						m_uMetaDataVer = META_DATA_VER;
					}

					TruncateED2KMetaData(mi->strAuthor);
					if (!mi->strAuthor.IsEmpty()){
						CTag* pTag = new CTag(FT_MEDIA_ARTIST, mi->strAuthor);
						AddTagUnique(pTag);
						m_uMetaDataVer = META_DATA_VER;
					}

					delete mi;
					return;
				}
			}
			catch(...){
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false, _T("Unhandled exception while extracting file meta (AVI) data from \"%s\""), szFullPath);
				ASSERT(0);
			}
		delete mi;
		}

#if _MSC_VER<1400
		if (thePrefs.GetExtractMetaData() >= 2)
		{
			// starting the MediaDet object takes a noticeable amount of time.. avoid starting that object
			// for files which are not expected to contain any Audio/Video data.
			// note also: MediaDet does not work well for too short files (e.g. 16K)
			EED2KFileType eFileType = GetED2KFileTypeID(GetFileName());
			if ((eFileType == ED2KFT_AUDIO || eFileType == ED2KFT_VIDEO) && GetFileSize() >= (uint64)32768)
			{
				// Avoid processing of some file types which are known to crash due to bugged DirectShow filters.
				TCHAR szExt[_MAX_EXT];
				_tsplitpath(GetFileName(), NULL, NULL, NULL, szExt);
				_tcslwr(szExt);
				if (_tcscmp(szExt, _T(".ogm"))!=0 && _tcscmp(szExt, _T(".ogg"))!=0 && _tcscmp(szExt, _T(".mkv"))!=0)
				{
					TCHAR szFullPath[MAX_PATH];
					if (_tmakepathlimit(szFullPath, NULL, GetPath(), GetFileName(), NULL))
					{
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
									WORD wAudioCodec = 0;
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
																	dwAudioBitRate = (DWORD)(((wfx->nAvgBytesPerSec * 8.0) + 500.0) / 1000.0);
																	wAudioCodec = wfx->wFormatTag;
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

									UINT uLengthSec = 0;
									CStringA strCodec;
									uint32 uBitrate = 0;
									if (fVideoStreamLengthSec > 0.0){
										uLengthSec = (UINT)fVideoStreamLengthSec;
										strCodec = GetED2KVideoCodec(dwVideoCodec);
										uBitrate = dwVideoBitRate;
									}
									else if (fAudioStreamLengthSec > 0.0){
										uLengthSec = (UINT)fAudioStreamLengthSec;
										strCodec = GetED2KAudioCodec(wAudioCodec);
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
#endif
	}
}

void CKnownFile::SetPublishedED2K(bool val){
	m_PublishedED2K = val;
	theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
}

bool CKnownFile::PublishNotes()
{
	if(m_lastPublishTimeKadNotes > (uint32)time(NULL))
	{
		return false;
	}
	if(GetFileComment() != "")
	{
		m_lastPublishTimeKadNotes = (uint32)time(NULL)+KADEMLIAREPUBLISHTIMEN;
		return true;
	}
	if(GetFileRating() != 0)
	{
		m_lastPublishTimeKadNotes = (uint32)time(NULL)+KADEMLIAREPUBLISHTIMEN;
		return true;
	}

	return false;
}

bool CKnownFile::PublishSrc()
{
	uint32 lastBuddyIP = 0;

	if( theApp.IsFirewalled() )
	{
		CUpDownClient* buddy = theApp.clientlist->GetBuddy();
		if( buddy )
		{
			lastBuddyIP = theApp.clientlist->GetBuddy()->GetIP();
			if( lastBuddyIP != m_lastBuddyIP )
			{
				SetLastPublishTimeKadSrc( (uint32)time(NULL)+KADEMLIAREPUBLISHTIMES, lastBuddyIP );
				return true;
			}
		}
		else
			return false;
	}

	if(m_lastPublishTimeKadSrc > (uint32)time(NULL))
		return false;

	SetLastPublishTimeKadSrc((uint32)time(NULL)+KADEMLIAREPUBLISHTIMES,lastBuddyIP);
	return true;
}

bool CKnownFile::IsMovie() const
{
	return (ED2KFT_VIDEO == GetED2KFileTypeID(GetFileName()) );
}

//MORPH START - Added by IceCream, music preview
bool CKnownFile::IsMusic() const
{
	return (ED2KFT_AUDIO == GetED2KFileTypeID(GetFileName()) );
}
//MORPH END   - Added by IceCream, music preview

// function assumes that this file is shared and that any needed permission to preview exists. checks have to be done before calling! 
bool CKnownFile::GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender)
{
	return GrabImage(GetPath() + CString(_T("\\")) + GetFileName(), nFramesToGrab,  dStartTime, bReduceColor, nMaxWidth, pSender);
}

bool CKnownFile::GrabImage(CString strFileName,uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender)
{
	if (!IsMovie())
		return false;
	CFrameGrabThread* framegrabthread = (CFrameGrabThread*) AfxBeginThread(RUNTIME_CLASS(CFrameGrabThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
	framegrabthread->SetValues(this, strFileName, nFramesToGrab, dStartTime, bReduceColor, nMaxWidth, pSender);
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
	for (int i = 0; i != nFramesGrabbed; i++)
		delete imgResults[i];
	delete[] imgResults;
}

CString CKnownFile::GetInfoSummary() const
{
	CString strFolder = GetPath();
	PathRemoveBackslash(strFolder.GetBuffer());
	strFolder.ReleaseBuffer();

	CString strAccepts, strRequests, strTransferred;
    strRequests.Format(_T("%u (%u)"), statistic.GetRequests(), statistic.GetAllTimeRequests());
	strAccepts.Format(_T("%u (%u)"), statistic.GetAccepts(), statistic.GetAllTimeAccepts());
	strTransferred.Format(_T("%s (%s)"), CastItoXBytes(statistic.GetTransferred(), false, false), CastItoXBytes(statistic.GetAllTimeTransferred(), false, false));
	CString strType = GetFileTypeDisplayStr();
	if (strType.IsEmpty())
		strType = _T("-");

	CString info;
	info.Format(_T("%s\n")
		+ GetResString(IDS_FD_HASH) + _T(" %s\n")
		+ GetResString(IDS_FD_SIZE) + _T(" %s\n<br_head>\n")
		+ GetResString(IDS_TYPE) + _T(": %s\n")
		+ GetResString(IDS_FOLDER) + _T(": %s\n\n")
		+ GetResString(IDS_PRIORITY) + _T(": %s\n")
		+ GetResString(IDS_SF_REQUESTS) + _T(": %s\n")
		+ GetResString(IDS_SF_ACCEPTS) + _T(": %s\n")
		+ GetResString(IDS_SF_TRANSFERRED) + _T(": %s"),
		GetFileName(),
		md4str(GetFileHash()),
		CastItoXBytes(GetFileSize(), false, false),
		strType,
		strFolder,
		GetUpPriorityDisplayString(),
		strRequests,
		strAccepts,
		strTransferred);
	return info;
}

CString CKnownFile::GetUpPriorityDisplayString() const {
	switch (GetUpPriority()) {
		case PR_VERYLOW :
			return GetResString(IDS_PRIOVERYLOW);
		case PR_LOW :
			if (IsAutoUpPriority())
				return GetResString(IDS_PRIOAUTOLOW);
			else
				return GetResString(IDS_PRIOLOW);
		case PR_NORMAL :
			if (IsAutoUpPriority())
				return GetResString(IDS_PRIOAUTONORMAL);
			else
				return GetResString(IDS_PRIONORMAL);
		case PR_HIGH :
			if (IsAutoUpPriority())
				return GetResString(IDS_PRIOAUTOHIGH);
			else
				return GetResString(IDS_PRIOHIGH);
		case PR_VERYHIGH :
			return GetResString(IDS_PRIORELEASE);
		default:
			return _T("");
	}
}

//MORPH START - Added by SiRoB, Power Share
void CKnownFile::SetPowerShared(int newValue) {
    int oldValue = m_powershared;
    m_powershared = newValue;
	if (IsPartFile())
		((CPartFile*)this)->UpdatePartsInfo();
	else
		UpdatePartsInfo();
	if(theApp.uploadqueue && oldValue != newValue)
	{
		theApp.uploadqueue->ReSortUploadSlots(true);
	}
}

void CKnownFile::UpdatePowerShareLimit(bool authorizepowershare,bool autopowershare, bool limitedpowershare)
{
	m_bPowerShareAuthorized = authorizepowershare;
	m_bPowerShareAuto = autopowershare;
	m_bPowerShareLimited = limitedpowershare;
	int temppowershared = (m_powershared>=0)?m_powershared:thePrefs.GetPowerShareMode();
	bool oldPowershare = m_bpowershared;
	m_bpowershared = ((temppowershared&1) || ((temppowershared == 2) && m_bPowerShareAuto)) && m_bPowerShareAuthorized && !((temppowershared == 3) && m_bPowerShareLimited);
	if (theApp.uploadqueue && oldPowershare != m_bpowershared)
		theApp.uploadqueue->ReSortUploadSlots(true);
}
bool CKnownFile::GetPowerShared() const
{
	return m_bpowershared;
}
//MORPH END   - Added by SiRoB, Power Share

//MORPH START - Revisited , we only get static Client PartCount now
// SLUGFILLER: hideOS
void CKnownFile::CalcPartSpread(CArray<uint64>& partspread, CUpDownClient* client)
{
	UINT parts = GetPartCount();
	uint64 min = (uint64)-1;
	UINT mincount = 1;
	UINT mincount2 = 1;
	UINT i;

	ASSERT(client != NULL);

	partspread.RemoveAll();

	CArray<bool, bool> partsavail;
	bool usepartsavail = false;
	
	partspread.SetSize(parts);
	partsavail.SetSize(parts);
	for (i = 0; i < parts; i++) {
		partspread[i] = 0;
		partsavail[i] = true;
	}

	//MORPH - Removed by SiRoB, Share Only The Need
	/*
	if(statistic.spreadlist.IsEmpty())
		return;
	*/
	if (IsPartFile()) {
		uint32 somethingtoshare = 0;
		for (i = 0; i < parts; i++)
			if (!((CPartFile*)this)->IsPartShareable(i)){	// SLUGFILLER: SafeHash
				partsavail[i] = false;
				usepartsavail = true;
			} else
				++somethingtoshare;
		if (somethingtoshare<=2)
			return;
	}
	if (client->m_abyUpPartStatus) {
		uint32 somethingtoshare = 0;
		for (i = 0; i < parts; i++)
			if (client->IsUpPartAvailable(i)) {
				partsavail[i] = false;
				usepartsavail = true;
			} else if (partsavail[i])
				++somethingtoshare;
		if (somethingtoshare<=2)
			return;
	}
	
	//MORPH START - Added by SiRoB, Share Only The Need
	UINT hideOS = HideOSInWork();
	if(hideOS && !statistic.spreadlist.IsEmpty())
	{
	//MORPH END   - Added by SiRoB, Share Only The Need
		POSITION pos = statistic.spreadlist.GetHeadPosition();
		uint16 last = 0;
		uint64 count = statistic.spreadlist.GetValueAt(pos);
		min = count;
		statistic.spreadlist.GetNext(pos);
		while (pos && last < parts){
			uint64 next = statistic.spreadlist.GetKeyAt(pos);
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
		for (i = 0; i < parts; i++)
			if (partspread[i] >= hideOS && client->m_abyUpPartStatus)
				client->m_abyUpPartStatus[i] |= SC_HIDDENBYHIDEOS;
	//MORPH START - Added by SiRoB, Share Only The Need
	}
	UINT SOTN = ((GetShareOnlyTheNeed()>=0)?GetShareOnlyTheNeed():thePrefs.GetShareOnlyTheNeed()); 
	if (SOTN) {
		if (IsPartFile()) {
			CPartFile* pfile = (CPartFile*)this;
			if (pfile->m_SrcpartFrequency.IsEmpty() == false) {
				for (i = 0; i < parts; i++)
					if (partsavail[i] && pfile->m_SrcpartFrequency[i]>partspread[i]) {
						partspread[i] = pfile->m_SrcpartFrequency[i];
						if (client->m_abyUpPartStatus)
							client->m_abyUpPartStatus[i] |= SC_HIDDENBYSOTN;
					}
			}
		} else {
			if (m_AvailPartFrequency.IsEmpty() == false) {
				for (i = 0; i < parts; i++)
					if (partsavail[i] && m_AvailPartFrequency[i]>partspread[i]) {
						partspread[i] = m_AvailPartFrequency[i];
						if (client->m_abyUpPartStatus)
							client->m_abyUpPartStatus[i] |= SC_HIDDENBYSOTN;
					}
			}
		}
	}

	//MORPH END   - Added by SiRoB, Share Only The Need
	if (usepartsavail && !statistic.spreadlist.IsEmpty() || SOTN) {		// Special case, ignore unshareables for min calculation
		//MORPH START - Changed by SiRoB, force always to show 2 chunks
		uint64 min2 = min = (uint64)-1;
		for (i = 0; i < parts; i++)
			if (partsavail[i]){
				if (min2>partspread[i])
					min2 = partspread[i];
				else if (min2==partspread[i])
					min = min2;
			}
		//MORPH END   - Changed by SiRoB, force always to show 2 chunks
	}

	for (i = 0; i < parts; i++) {
		if (partspread[i] > min)
			partspread[i] -= min;
		else {
			partspread[i] = 0;
			if (client->m_abyUpPartStatus)
				client->m_abyUpPartStatus[i] &= SC_AVAILABLE;
		}
	}

	if (!hideOS || ((GetSelectiveChunk()>=0)?!GetSelectiveChunk():!thePrefs.IsSelectiveShareEnabled()))
		return;

	if (client->m_nSelectedChunk > 0 && (int)client->m_nSelectedChunk <= partspread.GetSize() && partsavail[client->m_nSelectedChunk-1]) {
		for (i = 0; i < client->m_nSelectedChunk-1; i++) {
			partspread[i] = hideOS;
			//MORPH START - Added by SiRoB, See Chunk that we hide
			if (client->m_abyUpPartStatus)
				client->m_abyUpPartStatus[i] |= SC_HIDDENBYHIDEOS;
			//MORPH END   - Added by SiRoB, See Chunk that we hide
		}
		for (i = client->m_nSelectedChunk; i < parts; i++) {
			partspread[i] = hideOS;
			//MORPH START - Added by SiRoB, See Chunk that we hide
			if (client->m_abyUpPartStatus)
				client->m_abyUpPartStatus[i] |= SC_HIDDENBYHIDEOS;
			//MORPH END   - Added by SiRoB, See Chunk that we hide
		}
		return;
	}
	else
		client->m_nSelectedChunk = 0;

	bool resetSentCount = false;
	
	//MORPH START - Changed by SiRoB, -HotFix-
	/*
	if (m_PartSentCount.GetSize() != partspread.GetSize())
	*/
	if (m_PartSentCount.GetSize() != partspread.GetSize() || m_PartSentCount.GetSize() == 0)
	//MORPH END   - Changed by SiRoB, -HotFix-
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
		mincount2 = 0;
		m_PartSentCount.SetSize(parts);
		for (i = 0; i < parts; i++){
			m_PartSentCount[i] = partspread[i];
			if (partsavail[i] && !partspread[i])
				mincount++;
		}
		if (!mincount)
		  return; // We're a no-needed source already
	}

	mincount = (rand() % mincount) + 1;
	mincount2 = mincount;
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
	if (mincount)
		return;
	m_PartSentCount[i]++;
	client->m_nSelectedChunk = i+1;
	mincount = i;
	for (i = 0; i < parts; i++) {
		if (!partsavail[i])
			continue;
		if (m_PartSentCount[i] > min)
			continue;
		ASSERT(m_PartSentCount[i] == min);
		mincount2--;
		if (!mincount2)
			break;
	}
	if (mincount2)
		return;
	m_PartSentCount[i]++;
	mincount2 = i;
	/*
	for (i = 0; i < mincount; i++)
		partspread[i] = hideOS;
	for (i = mincount+1; i < parts; i++)
		partspread[i] = hideOS;
	*/
	for (i = 0; i < parts; i++)
	{	
		if ( i != mincount && i != mincount2) {
			partspread[i] = hideOS;
			//MORPH START - Added by SiRoB, See Chunk that we hide
			if (client->m_abyUpPartStatus)
				client->m_abyUpPartStatus[i] |= SC_HIDDENBYHIDEOS;
			//MORPH END   - Added by SiRoB, See Chunk that we hide
		}
	}
	return;
};
//MORPH END   - Revisited , we only get static Client PartCount now

//MORPH START - Revised, static client m_npartcount
bool CKnownFile::HideOvershares(CSafeMemFile* file, CUpDownClient* client){
	CArray<uint64> partspread;
	UINT parts = GetPartCount();
	if (client->m_abyUpPartStatus == NULL) {
		client->SetPartCount(parts);
		client->m_abyUpPartStatus = new uint8[parts];
		memset(client->m_abyUpPartStatus,0,parts);
	}
	CalcPartSpread(partspread, client);

	uint64 max = partspread[0];
	for (UINT i = 1; i < parts; i++)
		if (partspread[i] > max)
			max = partspread[i];
	UINT hideOS = HideOSInWork();
	UINT SOTN = ((GetShareOnlyTheNeed()>=0)?GetShareOnlyTheNeed():thePrefs.GetShareOnlyTheNeed());
	if (!hideOS && SOTN)
		hideOS = 1;
	if (max < hideOS || !hideOS)
		return FALSE;
	UINT nED2kPartCount = GetED2KPartCount();
	file->WriteUInt16((uint16)nED2kPartCount);
	UINT done = 0;
	while (done != nED2kPartCount){
		uint8 towrite = 0;
		for (UINT i = 0;i < 8;i++){
			if (done < parts && partspread[done] < hideOS) {
				towrite |= (1<<i);
				//MORPH START - Added by SiRoB, See Chunk that we hide
				client->m_abyUpPartStatus[done] &= SC_AVAILABLE;
				//MORPH END   - Added by SiRoB, See Chunk that we hide
			}
			done++;
			if (done == nED2kPartCount)
				break;
		}
		file->WriteUInt8(towrite);
	}
	return TRUE;
}
// SLUGFILLER: hideOS

//MORPH START - Added by SiRoB, Avoid misusing of HideOS
UINT	CKnownFile::HideOSInWork() const
{
	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	if(GetSpreadbarSetStatus() == 0 || (GetSpreadbarSetStatus() == -1 && thePrefs.GetSpreadbarSetStatus() == 0))
		return 0;
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	if (m_bHideOSAuthorized==true)
		return  (m_iHideOS>=0)?m_iHideOS:thePrefs.GetHideOvershares();
	else
		return 0;
}
//MORPH END   - Added by SiRoB, Avoid misusing of HideOS
// MORPH START - Added by Commander, WebCache 1.2e
// WebCache ////////////////////////////////////////////////////////////////////////////////////
//JP webcache release START
uint32 CKnownFile::GetNumberOfClientsRequestingThisFileUsingThisWebcache(CString webcachename, uint32 maxCount)
{
	if (maxCount == 0)
		maxCount = 0xFFFFFFFF; //be careful with using 0 (unlimited) when calling this function cause it is O(n^2) and gets called for every webcache enabled client
uint32 returncounter = 0;
CList<uint32,uint32&> IP_List; //JP only count unique IPs
POSITION pos = m_ClientUploadList.GetHeadPosition();
while (pos != NULL)
{
	CUpDownClient* cur_client = m_ClientUploadList.GetNext(pos);
	if (cur_client->GetWebCacheName() == webcachename && !cur_client->HasLowID() && cur_client->GetUploadState() != US_BANNED) //MORPH - Changed by SiRoB, Code Optimization
	{
		//search for IP in IP_List
		bool found = false;
		POSITION pos2 = IP_List.GetHeadPosition();
			uint32 cur_IP;
		while (pos2 != NULL)
		{
			cur_IP = IP_List.GetNext(pos2);
			if (cur_IP == cur_client->GetIP())
			{
				found = true;
			}
			//leave while look if IP was found
			if (found) break;
		}

		//if not found add IP to list
		if (!found)
		{
			uint32 user_IP = cur_client->GetIP();
			IP_List.AddTail(user_IP);
				returncounter++;
		}
	}
	//Don't let this list get longer than 10 so we don't waste CPU-cycles
	if (returncounter >= maxCount) break; 
}
IP_List.RemoveAll();
return returncounter;
}
//JP webcache release END
// MORPH END - Added by Commander, WebCache 1.2e

//MORPH START - Added by SiRoB, copy feedback feature
CString CKnownFile::GetFeedback(bool isUS)
{
	CString feed;
	if (isUS)
	{
		feed.AppendFormat(_T("File name: %s\r\n"),GetFileName());
		feed.AppendFormat(_T("File type: %s\r\n"),GetFileType());
		feed.AppendFormat(_T("Size: %s\r\n"), CastItoXBytes(GetFileSize(),false,false,3,true));
		feed.AppendFormat(_T("Downloaded: %s\r\n"), (IsPartFile()==false)?GetResString(IDS_COMPLETE):CastItoXBytes(((CPartFile*)this)->GetCompletedSize(),false,false,3));
		feed.AppendFormat(_T("Transferred: %s (%s)\r\n"), CastItoXBytes(statistic.GetTransferred(),false,false,3,true), CastItoXBytes(statistic.GetAllTimeTransferred(),false,false,3,true)); 
		feed.AppendFormat(_T("Requested: %i (%i)\r\n"), statistic.GetRequests(), statistic.GetAllTimeRequests()); 
		feed.AppendFormat(_T("Accepted Requests: %i (%i)\r\n"), statistic.GetAccepts(),statistic.GetAllTimeAccepts()); 
		if(IsPartFile()){
			feed.AppendFormat(_T("Total sources: %i \r\n"),((CPartFile*)this)->GetSourceCount());
			feed.AppendFormat(_T("Available sources : %i \r\n"),((CPartFile*)this)->GetAvailableSrcCount());
			feed.AppendFormat(_T("No Need Part sources: %i \r\n"),((CPartFile*)this)->GetSrcStatisticsValue(DS_NONEEDEDPARTS));
		}
		feed.AppendFormat(_T("Complete sources: %i (%i)\r\n"),m_nCompleteSourcesCount, m_nVirtualCompleteSourcesCount);
	}
	else
	{
		feed.AppendFormat(GetResString(IDS_FEEDBACK_FILENAME), GetFileName());
		feed.Append(_T(" \r\n"));
		feed.AppendFormat(GetResString(IDS_FEEDBACK_FILETYPE), GetFileType());
		feed.Append(_T(" \r\n"));
		feed.AppendFormat(GetResString(IDS_FEEDBACK_FILESIZE), CastItoXBytes(GetFileSize(),false,false,3));
		feed.Append(_T(" \r\n"));
		feed.AppendFormat(GetResString(IDS_FEEDBACK_DOWNLOADED), (IsPartFile()==false)?GetResString(IDS_COMPLETE):CastItoXBytes(((CPartFile*)this)->GetCompletedSize(),false,false,3));
		feed.Append(_T(" \r\n"));
		feed.AppendFormat(GetResString(IDS_FEEDBACK_TRANSFERRED), CastItoXBytes(statistic.GetTransferred(),false,false,3),CastItoXBytes(statistic.GetAllTimeTransferred(),false,false,3));
		feed.Append(_T(" \r\n"));
		feed.AppendFormat(GetResString(IDS_FEEDBACK_REQUESTED), statistic.GetRequests(), statistic.GetAllTimeRequests());
		feed.Append(_T(" \r\n"));
		feed.AppendFormat(GetResString(IDS_FEEDBACK_ACCEPTED), statistic.GetAccepts() , statistic.GetAllTimeAccepts());
		feed.Append(_T(" \r\n"));
		if(IsPartFile()){
			feed.AppendFormat(GetResString(IDS_FEEDBACK_TOTAL), ((CPartFile*)this)->GetSourceCount());
			feed.Append(_T(" \r\n"));
			feed.AppendFormat(GetResString(IDS_FEEDBACK_AVAILABLE), ((CPartFile*)this)->GetAvailableSrcCount());
			feed.Append(_T(" \r\n"));
			feed.AppendFormat(GetResString(IDS_FEEDBACK_NONEEDPART), ((CPartFile*)this)->GetSrcStatisticsValue(DS_NONEEDEDPARTS));
			feed.Append(_T(" \r\n"));
		}
		feed.AppendFormat(GetResString(IDS_FEEDBACK_COMPLETE), m_nCompleteSourcesCount, m_nVirtualCompleteSourcesCount);
		feed.Append(_T(" \r\n"));
	}
	return feed;
}
//MORPH END   - Added by SiRoB, copy feedback feature
