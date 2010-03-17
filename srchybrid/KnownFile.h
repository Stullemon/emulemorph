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
#pragma once
#include "BarShader.h"
#include "StatisticFile.h"
#include "ShareableFile.h"
#include <list>

class CxImage;
class CUpDownClient;
class Packet;
class CFileDataIO;
class CAICHHashTree;
class CAICHRecoveryHashSet;
class CCollection;
class CAICHHashAlgo;
class CSafeMemFile;	// SLUGFILLER: hideOS

typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

class CKnownFile : public CShareableFile
{
	DECLARE_DYNAMIC(CKnownFile)
	friend class CImportPartsFileThread; //MORPH - Added by SiRoB, ImportParts

public:
	CKnownFile();
	virtual ~CKnownFile();

	virtual void SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars = false, bool bRemoveControlChars = false); // 'bReplaceInvalidFileSystemChars' is set to 'false' for backward compatibility!
	
	bool	CreateFromFile(LPCTSTR directory, LPCTSTR filename, LPVOID pvProgressParam); // create date, hashset and tags from a file
	bool	LoadFromFile(CFileDataIO* file);	//load date, hashset and tags from a .met file
	bool	WriteToFile(CFileDataIO* file);
	//MORPH START - Added by SiRoB, Import Parts [SR13]
	bool	SR13_ImportParts();
	//MORPH END   - Added by SiRoB, Import Parts [SR13]
	bool	CreateAICHHashSetOnly();

	// last file modification time in (DST corrected, if NTFS) real UTC format
	// NOTE: this value can *not* be compared with NT's version of the UTC time
	CTime	GetUtcCFileDate() const										{ return CTime(m_tUtcLastModified); }
	time_t	GetUtcFileDate() const										{ return m_tUtcLastModified; }

	// Did we not see this file for a long time so that some information should be purged?
	bool	ShouldPartiallyPurgeFile() const;
	bool	ShouldCompletlyPurgeFile() const; //EastShare - Added by TAHO, .met control
	void	SetLastSeen()												{ m_timeLastSeen = time(NULL); }

	virtual void	SetFileSize(EMFileSize nFileSize);

	// nr. of 9MB parts (file data)
	__inline uint16 GetPartCount() const								{ return m_iPartCount; }

	// nr. of 9MB parts according the file size wrt ED2K protocol (OP_FILESTATUS)
	__inline uint16 GetED2KPartCount() const { return m_iED2KPartCount; }

	// file upload priority
	uint8	GetUpPriority(void) const									{ return m_iUpPriority; }
	void	SetUpPriority(uint8 iNewUpPriority, bool bSave = true);
	bool	IsAutoUpPriority(void) const								{ return m_bAutoUpPriority; }
	void	SetAutoUpPriority(bool NewAutoUpPriority)					{ m_bAutoUpPriority = NewAutoUpPriority; }
	void	UpdateAutoUpPriority();

	// This has lost it's meaning here.. This is the total clients we know that want this file..
	// Right now this number is used for auto priorities..
	// This may be replaced with total complete source known in the network..
	uint32	GetQueuedCount() { return m_ClientUploadList.GetCount();}

	bool	HideOvershares(CSafeMemFile* file, CUpDownClient* client);	// SLUGFILLER: hideOS

	void	AddUploadingClient(CUpDownClient* client);
	void	RemoveUploadingClient(CUpDownClient* client);
	virtual void	UpdatePartsInfo();
	virtual	void	DrawShareStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool bFlat) /*const*/;

	// comment
	void	SetFileComment(LPCTSTR pszComment);

	void	SetFileRating(UINT uRating);

	bool	GetPublishedED2K() const { return m_PublishedED2K; }
	void	SetPublishedED2K(bool val);

	uint32	GetKadFileSearchID() const { return kadFileSearchID; }
	void	SetKadFileSearchID(uint32 id) { kadFileSearchID = id; } //Don't use this unless you know what your are DOING!! (Hopefully I do.. :)

	const Kademlia::WordList& GetKadKeywords() const { return wordlist; }

	uint32	GetLastPublishTimeKadSrc() const { return m_lastPublishTimeKadSrc; }
	void	SetLastPublishTimeKadSrc(uint32 time, uint32 buddyip) { m_lastPublishTimeKadSrc = time; m_lastBuddyIP = buddyip;}
	uint32	GetLastPublishBuddy() const { return m_lastBuddyIP; }
	void	SetLastPublishTimeKadNotes(uint32 time) {m_lastPublishTimeKadNotes = time;}
	uint32	GetLastPublishTimeKadNotes() const { return m_lastPublishTimeKadNotes; }

	bool	PublishSrc();
	bool	PublishNotes();

	// file sharing
	virtual Packet* CreateSrcInfoPacket(const CUpDownClient* forClient, uint8 byRequestedVersion, uint16 nRequestedOptions) const;
	UINT	GetMetaDataVer() const { return m_uMetaDataVer; }
	void	UpdateMetaDataTags();
	void	RemoveMetaDataTags(UINT uTagType = 0);
	void	RemoveBrokenUnicodeMetaDataTags();

	// preview
	bool	IsMovie() const;
	bool	IsMusic() const; //MORPH - Added by IceCream, added preview also for music files
	virtual bool GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender);
	virtual void GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender);

	// Display / Info / Strings
	virtual CString	GetInfoSummary() const;
	CString			GetUpPriorityDisplayString() const;

	//aich
	void	SetAICHRecoverHashSetAvailable(bool bVal)			{ m_bAICHRecoverHashSetAvailable = bVal; }
	bool	IsAICHRecoverHashSetAvailable() const				{ return m_bAICHRecoverHashSetAvailable; }						

	static bool	CreateHash(const uchar* pucData, uint32 uSize, uchar* pucHash, CAICHHashTree* pShaHashOut = NULL);



	// last file modification time in (DST corrected, if NTFS) real UTC format
	// NOTE: this value can *not* be compared with NT's version of the UTC time
	time_t	m_tUtcLastModified; // vs2005 ?

	CStatisticFile statistic;
	time_t m_nCompleteSourcesTime;
	uint16 m_nCompleteSourcesCount;
	uint16 m_nCompleteSourcesCountLo;
	uint16 m_nCompleteSourcesCountHi;
	CUpDownClientPtrList m_ClientUploadList;
	CArray<uint16, uint16> m_AvailPartFrequency;
	CCollection* m_pCollection;
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	UINT m_nVirtualCompleteSourcesCount;
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing

	CArray<uint64> m_PartSentCount;	// SLUGFILLER: hideOS

#ifdef _DEBUG
	// Diagnostic Support
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	//MORPH START - Added by SiRoB, Show Permissions
	// shared file view permissions (all, only friends, no one)
	int		GetPermissions(void) const	{ return m_iPermissions; }
	void	SetPermissions(int iNewPermissions);
	//MORPH END   - Added by SiRoB, Show Permissions
	//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
	void    SetPowerShared(int newValue);
	bool    GetPowerShared() const;
	//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	void	SetSpreadbarSetStatus(int newValue) {m_iSpreadbarSetStatus = newValue;}
	int		GetSpreadbarSetStatus() const {return m_iSpreadbarSetStatus;}
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	//MORPH START - Added by SiRoB, HIDEOS
	void	SetHideOS(int newValue) {m_iHideOS = newValue;}
	int		GetHideOS() const {return m_iHideOS;}
	void	SetSelectiveChunk(int newValue) {m_iSelectiveChunk = newValue;}
	int		GetSelectiveChunk() const {return m_iSelectiveChunk;}
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, Avoid misusing of hideOS
	void	SetHideOSAuthorized(bool newValue) {m_bHideOSAuthorized = newValue;}
	UINT	HideOSInWork() const;
	//MORPH END   - Added by SiRoB, Avoid misusing of hideOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	void	SetShareOnlyTheNeed(int newValue) {m_iShareOnlyTheNeed = newValue;}
	int		GetShareOnlyTheNeed() const {return m_iShareOnlyTheNeed;}
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	int		GetPowerSharedMode() const {return m_powershared;}
	bool	GetPowerShareAuthorized() const {return m_bPowerShareAuthorized;}
	bool	GetPowerShareAuto() const {return m_bPowerShareAuto;}
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	void	SetPowerShareLimit(int newValue) {m_iPowerShareLimit = newValue;}
	int		GetPowerShareLimit() const {return m_iPowerShareLimit;}
	bool	GetPowerShareLimited() const {return m_bPowerShareLimited;}
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	void	UpdatePowerShareLimit(bool authorizepowershare,bool autopowershare, bool limitedpowershare);
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing

	// Mighty Knife: CRC32-Tag
	bool    IsCRC32Calculated () const			{return m_sCRC32[0]!='\0';}
	CString GetLastCalculatedCRC32 () const		{return m_sCRC32;}
	// The CRC32 is not created within this object but written to this object:
	void    SetLastCalculatedCRC32 (const LPCTSTR _CRC) {_tcscpy (m_sCRC32,_CRC);}

	// [end] Mighty Knife
	//
	//MORPH START - Added by SiRoB, copy feedback feature
	CString GetFeedback(bool isUS = false);
	//MORPH END   - Added by SiRoB, copy feedback feature
	
protected:
	//preview
	bool	GrabImage(CString strFileName, uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender);
	bool	LoadTagsFromFile(CFileDataIO* file);
	bool	LoadDateFromFile(CFileDataIO* file);
	static void	CreateHash(CFile* pFile, uint64 uSize, uchar* pucHash, CAICHHashTree* pShaHashOut = NULL);
	static bool	CreateHash(FILE* fp, uint64 uSize, uchar* pucHash, CAICHHashTree* pShaHashOut = NULL);
	virtual void	UpdateFileRatingCommentAvail(bool bForceUpdate = false);
	//MORPH START - Revisited , we only get static Client PartCount now
	void	CalcPartSpread(CArray<uint64>& partspread, CUpDownClient* client);	// SLUGFILLER: hideOS
	//MORPH END   - Revisited , we only get static Client PartCount now

private:
	static CBarShader s_ShareStatusBar;
	uint16	m_iPartCount;
	uint16	m_iED2KPartCount;
	uint8	m_iUpPriority;
	bool	m_bAutoUpPriority;
	bool	m_PublishedED2K;
	uint32	kadFileSearchID;
	uint32	m_lastPublishTimeKadSrc;
	uint32	m_lastPublishTimeKadNotes;
	uint32	m_lastBuddyIP;
	Kademlia::WordList wordlist;
	UINT	m_uMetaDataVer;
	time_t	m_timeLastSeen; // we only "see" files when they are in a shared directory
	bool	m_bAICHRecoverHashSetAvailable;

	//MORPH START - Added by SiRoB,  SharedStatusBar CPU Optimisation
	bool	InChangedSharedStatusBar;
	CBitmap m_bitmapSharedStatusBar;
	int	lastSize;
	bool	lastonlygreyrect;
	bool	lastbFlat;
	//MORPH END - Added by SiRoB,  SharedStatusBar CPU Optimisation

	//MORPH START - Added by SiRoB, Show Permission
	int		m_iPermissions;
	//MORPH END   - Added by SiRoB, Show Permission

	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	int		m_iSpreadbarSetStatus;
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	//MORPH START - Added by SiRoB, HIDEOS
	int		m_iHideOS;
	int		m_iSelectiveChunk;
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, Avoid misusing of hideOS
	bool	m_bHideOSAuthorized;
	//MORPH END   - Added by SiRoB, Avoid misusing of hideOS
	
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	int		m_iShareOnlyTheNeed;
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	int		m_powershared;
	bool	m_bPowerShareAuthorized;
	bool	m_bPowerShareAuto;
	bool	m_bpowershared;
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	int		m_iPowerShareLimit;
	bool	m_bPowerShareLimited;
	//MORPH END   - Added by SiRoB, POWERSHARE Limit

	// Mighty Knife: CRC32-Tag
	TCHAR    m_sCRC32 [16];
	// [end] Mighty Knife
};

// permission values for shared files
#define PERM_ALL		0
#define PERM_FRIENDS	1
#define PERM_NOONE		2
// MightyKnife: Community visible files
#define PERM_COMMUNITY  3
// [end] Mighty Knife
