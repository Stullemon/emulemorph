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
#include "knownfile.h"
#include "types.h"
#include "opcodes.h"
#include "BarShader.h"
#include "kademlia/utils/UInt128.h"
// khaos::kmod+ Save/Load Sources
#include "SourceSaver.h" //<<-- enkeyDEV(Ottavio84) -New SLS-
// khaos::kmod-

#define	PS_READY			0
#define	PS_EMPTY			1
#define PS_WAITINGFORHASH	2
#define PS_HASHING			3
#define PS_ERROR			4
#define PS_INSUFFICIENT		5	// SLUGFILLER: checkDiskspace
#define	PS_UNKNOWN			6
#define PS_PAUSED			7
#define PS_COMPLETING		8
#define PS_COMPLETE			9

#define PR_VERYLOW			4 // I Had to change this because it didn't save negative number correctly.. Had to modify the sort function for this change..
#define PR_LOW				0 //*
#define SRV_PR_LOW			2
#define PR_NORMAL			1 // Don't change this - needed for edonkey clients and server!
#define SRV_PR_NORMAL		0
#define	PR_HIGH				2 //*
#define SRV_PR_HIGH			1
#define PR_VERYHIGH			3
#define PR_AUTO				5 //UAP Hunter

//#define BUFFER_SIZE_LIMIT	500000 // Max bytes before forcing a flush
#define BUFFER_TIME_LIMIT	60000   // Max milliseconds before forcing a flush

#define	PARTMET_BAK_EXT	_T(".bak")
#define	PARTMET_TMP_EXT	_T(".backup")

#define STATES_COUNT		13

class CSearchFile;
class CUpDownClient;
class CxImage;

struct PartFileBufferedData
{
	BYTE *data;						// Barry - This is the data to be written
	uint32 start;					// Barry - This is the start offset of the data
	uint32 end;						// Barry - This is the end offset of the data
	Requested_Block_Struct *block;	// Barry - This is the requested block that this data relates to
};

class CPartFile : public CKnownFile {
public:
	CPartFile();
	CPartFile(CSearchFile* searchresult);  //used when downloading a new file
	CPartFile(CString edonkeylink);
	CPartFile(class CED2KFileLink* fileLink);
	void InitializeFromLink(CED2KFileLink* fileLink);
	virtual ~CPartFile();
	
	bool	CreateFromFile(LPCTSTR directory,LPCTSTR filename)	{return false;}// not supported in this class
	bool	LoadFromFile(FILE* file)						{return false;}
	bool	IsPartFile()									{return !(status == PS_COMPLETE);}
	//MORPH START - Added by Yun.SF3, ZZ Upload System
	uint32	Process(uint32 reducedownload,uint8 m_icounter, uint32 friendReduceddownload);
	//MORPH END - Added by Yun.SF3, ZZ Upload System
		bool	LoadPartFile(LPCTSTR in_directory, LPCTSTR filename); //filename = *.part.met
	bool	SavePartFile();
	void	PartFileHashFinished(CKnownFile* result);
	bool	HashSinglePart(uint16 partnumber); // true = ok , false = corrupted	
	uint64	GetRealFileSize();

	void	AddGap(uint32 start, uint32 end);
	void	FillGap(uint32 start, uint32 end);
	void	DrawStatusBar(CDC* dc, RECT* rect, bool bFlat);
	void	DrawShareStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat);
	bool	IsComplete(uint32 start, uint32 end);
	bool	IsPureGap(uint32 start, uint32 end);
	bool	IsCorruptedPart(uint16 partnumber);
	void	UpdateCompletedInfos();

	bool	GetNextRequestedBlock(CUpDownClient* sender,Requested_Block_Struct** newblocks,uint16* count);
	void	WritePartStatus(CFile* file, CUpDownClient* client = NULL);
	void	WriteCompleteSourcesCount(CFile* file);
	void	AddSources(CMemFile* sources,uint32 serverip, uint16 serverport);
	static bool	CanAddSource(uint32 userid, uint16 port, uint32 serverip, uint16 serverport, uint8* pdebug_lowiddropped = NULL);
	uint8	GetStatus(bool ignorepause = false);
	void	NewSrcPartsInfo();
	void	SetDownPriority(uint8 iNewDownPriority);
	bool	IsAutoDownPriority(void)		{return m_bAutoDownPriority;};
	void	SetAutoDownPriority(bool NewAutoDownPriority) {m_bAutoDownPriority = NewAutoDownPriority;};
	void	UpdateAutoDownPriority();
	const CString& GetPartMetFileName() const				{return m_partmetfilename;}
	uint32	GetTransfered()									{return transfered;}
	uint8	GetDownPriority()								{return m_iDownPriority;}
	const CString& GetFullName() const						{return m_fullname;}
	uint16	GetSourceCount();
	// khaos::kmod+ Source Counts Are Cached
	uint16	GetAvailableSrcCount()							{return m_anStates[0]+m_anStates[1];}	
	// khaos::kmod-
	uint16	GetSrcA4AFCount()								{return m_iSrcA4AF;}
	uint16  GetSrcStatisticsValue(uint16 value); // value = EDownloadState
	uint16	GetTransferingSrcCount();			// == GetSrcStatisticsValue(DS_DOWNLOADING)
	//MORPH START - Added by SiRoB, A4AF counter
	void	IncreaseSourceCountA4AF()						{m_iSrcA4AF++;}
	void	DecreaseSourceCountA4AF()						{m_iSrcA4AF--;}
	void	CleanA4AFSource(CPartFile* toremove);
	//MORPH END    - Added by SiRoB, A4AF counter
	
	// khaos::categorymod+
	bool	IsPaused()										{return GetStatus()==PS_PAUSED;}
	// khaos::categorymod-
	uint32	GetDatarate()									{return datarate;}
	float	GetPercentCompleted()							{return percentcompleted;}
	//MORPH START - Modifified by SiRoB
	uint16  GetNotCurrentSourcesCount()						{return m_anStates[2]+m_anStates[3]+m_anStates[4]+m_anStates[5]+m_anStates[6]+m_anStates[7]+m_anStates[8]+m_anStates[9]+m_anStates[10]+m_anStates[11]+m_anStates[12];} // m_iSourceCount - m_iSrcTransferring - m_iSrcOnQueue;}
	int		GetValidSourcesCount()							{return m_anStates[1]+m_anStates[0]+m_anStates[2]+m_anStates[12] ;}// DS_ONQUEUE + DS_DOWNLOADING + DS_CONNECTED + DS_REMOTEQUEUEFULL;}
	//MORPH END   - Modifified by SiRoB

	uint32	GetNeededSpace(); // SLUGFILLER: checkDiskspace
	bool	IsMovie();
	bool	IsArchive(bool onlyPreviewable=false); // Barry - Also want to preview archives
	bool	IsMusic(); //MORPH - Added by IceCream, added preview also for music files
	bool	IsCDImage(); //MORPH - Added by IceCream, for defeat 0-filler
	bool	IsDocument(); //MORPH - Added by IceCream, for defeat 0-filler
	CString CPartFile::getPartfileStatus(); //<<--9/21/02
	sint32	CPartFile::getTimeRemaining(); //<<--9/21/02
	CTime	lastseencomplete;
	int		getPartfileStatusRang();
	// CString	GetDownloadFileInfo(); moved to WebServer.h

	// Barry - Added as replacement for BlockReceived to buffer data before writing to disk
	uint32	WriteToBuffer(uint32 transize, BYTE *data, uint32 start, uint32 end, Requested_Block_Struct *block);
	void	FlushBuffer(void);
	// Barry - This will invert the gap list, up to caller to delete gaps when done
	// 'Gaps' returned are really the filled areas, and guaranteed to be in order
	void	GetFilledList(CTypedPtrList<CPtrList, Gap_Struct*> *filled);

	// Barry - Is archive recovery in progress
	volatile bool m_bRecoveringArchive;

	// Barry - Added to prevent list containing deleted blocks on shutdown
	void	RemoveAllRequestedBlocks(void);

	void	RemoveBlockFromList(uint32 start,uint32 end);
	void	RemoveAllSources(bool bTryToSwap);
	void	DeleteFile();
	// khaos::kmod+ New param used for completing files
	void	StopFile(bool setVars = true);
	// khaos::kmod-
	void	PauseFile();
	void	ResumeFile();
	// SLUGFILLER: checkDiskspace
	void	PauseFileInsufficient();
	void	ResumeFileInsufficient();
	// SLUGFILLER: checkDiskspace

	virtual	Packet* CreateSrcInfoPacket(CUpDownClient* forClient);
	void	AddClientSources(CMemFile* sources, uint8 sourceexchangeversion);

	void	PreviewFile();
	bool	PreviewAvailable();
	uint16	GetAvailablePartCount()			{return availablePartsCount;}
	void	UpdateAvailablePartsCount();

	uint32	GetLastAnsweredTime()			{ return m_ClientSrcAnswered; }
	void	SetLastAnsweredTime()			{ m_ClientSrcAnswered = ::GetTickCount(); }
	void	SetLastAnsweredTimeTimeout()	{ m_ClientSrcAnswered = 2 * CONNECTION_LATENCY +
											                        ::GetTickCount() - SOURCECLIENTREASK; }
	// -khaos--+++>
	uint32	GetLostDueToCorruption()		{return m_iLostDueToCorruption+m_iSesCorruptionBytes;}
	uint32	GetGainDueToCompression()		{return m_iGainDueToCompression+m_iSesCompressionBytes;}
	uint32	GetSesCorruptionBytes()			{return m_iSesCorruptionBytes;}
	uint32	GetSesCompressionBytes()		{return m_iSesCompressionBytes;}
	// <-----khaos-
	uint32	TotalPacketsSavedDueToICH()		{return m_iTotalPacketsSavedDueToICH;}
	bool	HasComment()					{return hasComment;}
	bool	HasRating()						{return hasRating;}
	bool	HasBadRating()					{return hasBadRating;}
	bool	IsStopped()						{return stopped;}
	void	SetHasComment(bool in)			{hasComment=in;}
	void	SetHasRating(bool in)			{hasRating=in;}
	void	UpdateFileRatingCommentAvail();
	void	AddDownloadingSource(CUpDownClient* client);
	void	RemoveDownloadingSource(CUpDownClient* client);
	void	SetStatus(uint8 in);

	//MORPH - Removed by SiRoB, Due to Khaos A4AF 
	/*void	SetA4AFAuto(bool in)			{m_is_A4AF_auto = in;} // [sivka / Tarod]
	bool	IsA4AFAuto()					{return m_is_A4AF_auto;} // [sivka / Tarod]*/
	CString GetProgressString(uint16 size);
	CString GetInfoSummary(CPartFile* partfile);

	int		GetCommonFilePenalty();
	void	UpdateDisplayedInfo(boolean force=false);
	uint8	GetCategory();
	void	SetCategory(uint8 cat);
	void	GetSizeToTransferAndNeededSpace(uint32& pui32SizeToTransfer, uint32& pui32NeededSpace);
	
	uint8*	MMCreatePartStatus();
	//preview
	bool	GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth,void* pSender);
	void	GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed,void* pSender);

	uint16	src_stats[3];
	CFile	m_hpartfile;	//permanent opened handle to avoid write conflicts
	volatile bool m_bPreviewing;
	CMutex 	m_FileCompleteMutex; // Lord KiRon - Mutex for file completion
	//MORPH - Removed by SiRoB, Due to Khaos A4AF
	/*CTypedPtrList<CPtrList, CUpDownClient*> A4AFsrclist; //<<-- enkeyDEV(Ottavio84) -A4AF-*/
	// khaos::categorymod+
	void	SetCatResumeOrder(uint16 order)	{ m_catResumeOrder = order; SavePartFile(); }
	uint16	GetCatResumeOrder()				{ return m_catResumeOrder; }
	uint16	GetFileGroup()					{ return m_catFileGroup; }
	void	SetFileGroup(uint16 group)		{ m_catFileGroup = group; SavePartFile(); }
	// khaos::categorymod-
	// khaos::accuratetimerem+
	void	SetActivatedTick()				{ m_dwActivatedTick = GetTickCount(); }
	DWORD	GetActivatedTick()				{ return m_dwActivatedTick; }
	sint32	GetTimeRemainingAvg();
	// khaos::accuratetimerem-
	// khaos::kmod+ Advanced A4AF: Brute Force Features
	bool	ForceAllA4AF()					{ return m_bForceAllA4AF; }
	bool	ForceA4AFOff()					{ return m_bForceA4AFOff; }
	void	SetForceAllA4AF(bool in)		{ m_bForceAllA4AF = in; }
	void	SetForceA4AFOff(bool in)		{ m_bForceA4AFOff = in; }
	// khaos::kmod-
	int  CPartFile::GetRating(); //MORPH - Added by IceCream, eMule Plus rating icons

protected:
	bool	GetNextEmptyBlockInPart(uint16 partnumber,Requested_Block_Struct* result);
	bool	IsAlreadyRequested(uint32 start, uint32 end);
	void	CompleteFile(bool hashingdone);
	void	CreatePartFile();
	void	Init();
	// khaos::kmod+ Save/Load Sources
	CSourceSaver m_sourcesaver; //<<-- enkeyDEV(Ottavio84) -New SLS-
	// khaos::kmod-
private:
	uint16	count;
	// -khaos--+++>
	uint16	m_iSrcA4AF;
	// <-----khaos-
	uint16	m_anStates[STATES_COUNT];
	uint32  completedsize;
	// -khaos--+++>
	uint32	m_iLostDueToCorruption;
	uint32	m_iGainDueToCompression;
	uint32	m_iSesCompressionBytes;
	uint32	m_iSesCorruptionBytes;
	// <-----khaos-
	uint32  m_iTotalPacketsSavedDueToICH; 
	uint32	datarate;
	CString	m_fullname;
	CString	m_partmetfilename;
	uint32	transfered;
	bool	paused;
	bool	stopped;
	bool	insufficient; // SLUGFILLER: checkDiskspace
	uint8	m_iDownPriority;
	bool	m_bAutoDownPriority;
	uint8	status;
	bool	newdate;	// indicates if there was a writeaccess to the .part file
	uint32	lastpurgetime;
	uint32	m_LastNoNeededCheck;
	CTypedPtrList<CPtrList, Gap_Struct*> gaplist;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> requestedblocks_list;
	CArray<uint16,uint16> m_SrcpartFrequency;
	float	percentcompleted;
	CList<uint16,uint16>	corrupted_list;
	uint16	availablePartsCount;
	uint32	m_ClientSrcAnswered;
	bool	m_bPercentUpdated;
	static	CBarShader s_LoadBar; 
	static	CBarShader s_ChunkBar; 
	bool	hasRating;
	bool	hasBadRating;
	bool	hasComment;
	bool	updatemystatus;
	BOOL 	PerformFileComplete(); // Lord KiRon
	static UINT CompleteThreadProc(LPVOID pvParams); // Lord KiRon - Used as separate thread to complete file
	DWORD	m_lastRefreshedDLDisplay;
	DWORD   m_lastdatetimecheck;
	CTime	m_lastdatecheckvalue;
	CTypedPtrList<CPtrList, CUpDownClient*> m_downloadingSourceList;
	void	CharFillRange(CString* buffer,uint32 start, uint32 end, char color);

	// Barry - Buffered data to be written
	CTypedPtrList<CPtrList, PartFileBufferedData*> m_BufferedData_list;
	uint32 m_nTotalBufferData;
	uint32 m_nLastBufferFlushTime;
	uint8	m_category;
	//MORPH - Removed by SiRoB, Due to Khaos A4AF
	/*bool	m_is_A4AF_auto;*/

	// khaos::categorymod+
	uint16	m_catResumeOrder;
	uint16	m_catFileGroup;
	// khaos::categorymod-
	// khaos::accuratetimerem+
	uint32	m_nSecondsActive;
	uint32	m_nInitialBytes;
	DWORD	m_dwActivatedTick;
	// khaos::accuratetimerem-
	// khaos::kmod+ Advanced A4AF: Force A4AF
	bool	m_bForceAllA4AF;
	bool	m_bForceA4AFOff;
	// khaos::kmod-
	//MORPH START - Added by SiRoB, ZZ Upload System 20030723-0133
	uint32 m_random_update_wait;
	uint32 m_NumberOfClientsWithPartStatus;
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030723-0133
	//MORPH START - Added by IceCream, Complete source feature v0.07a zegzav
	uint16	m_nLastCompleteSrcCount;	// #zegzav:completesrc (add)
	bool	m_bUpdateCompleteSrcCount;	// #zegzav:completesrc (add)
	//MORPH END   - Added by IceCream, Complete source feature v0.07a zegzav
	//MORPH START - Added by SiRoB,  SharedStatusBar CPU Optimisation
	bool	InChangedSharedStatusBar;
	CDC 	m_dcSharedStatusBar;
	CBitmap m_bitmapSharedStatusBar;
	CBitmap *m_pbitmapOldSharedStatusBar;
	int		lastSize;
	bool	lastonlygreyrect;
	bool	lastbFlat;
	//MORPH END - Added by SiRoB,  SharedStatusBar CPU Optimisation
public:
	uint32	lastsearchtime;
	uint32	lastsearchtimeKad;	
	bool	m_bLocalSrcReqQueued;
	CTypedPtrList<CPtrList, CUpDownClient*> srclists[SOURCESSLOTS];
	bool	srcarevisible; // used for downloadlistctrl
	bool	hashsetneeded;
	uint32  GetCompletedSize()   {return completedsize;}
};
