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
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once
#include "BarShader.h"
#include <list>

#define	PARTSIZE			9728000

class CTag;
class CxImage;
namespace Kademlia{
class CUInt128;
	typedef std::list<CStringW> WordList;
};
class CUpDownClient;
class Packet;
class CFileDataIO;
class CAICHHashTree;
class CAICHHashSet;
class CSafeMemFile;	// SLUGFILLER: hideOS
typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

class CFileStatistic
{
	friend class CKnownFile;
	friend class CPartFile;
public:
	CFileStatistic()
	{
		requested = 0;
		transferred = 0;
		accepted = 0;
		alltimerequested= 0;
		alltimetransferred = 0;
		alltimeaccepted = 0;

		//MORPH START - Added by SiRoB, Reduce SpreadBar CPU consumption
		InChangedSpreadSortValue = false;
		InChangedFullSpreadCount = false;
		InChangedSpreadBar = false;
		lastSpreadSortValue = 0;;
		lastFullSpreadCount = 0;
		//MORPH END   - Added by SiRoB, Reduce SpreadBar CPU consumption

		//Morph Start - Added by AndCycle, Equal Chance For Each File
		shareStartTime = time(NULL);//this value init will be done in other place 
		m_bInChangedEqualChanceValue = false;
		m_dLastEqualChanceSemiValue = 0;
		//Morph End - Added by AndCycle, Equal Chance For Each File
	}
	//MORPH START - Added by SiRoB, Reduce SpreadBar CPU consumption
	~CFileStatistic()
	{
		m_bitmapSpreadBar.DeleteObject();
	}
	//MORPH END   - Added by SiRoB, Reduce SpreadBar CPU consumption
	void	MergeFileStats( CFileStatistic* toMerge );
	void	AddRequest();
	void	AddAccepted();
	//MORPH START - Added by IceCream SLUGFILLER: Spreadbars
	void	AddTransferred(uint32 start, uint32 bytes);
	void	AddBlockTransferred(uint32 start, uint32 end, uint32 count);
	void	DrawSpreadBar(CDC* dc, RECT* rect, bool bFlat) /*const*/;
	float	GetSpreadSortValue() /*const*/;
	float	GetFullSpreadCount() /*const*/;
	//MORPH END - Added by IceCream SLUGFILLER: Spreadbars
	UINT	GetRequests() const				{return requested;}
	UINT	GetAccepts() const				{return accepted;}
	uint64	GetTransferred() const			{return transferred;}
	UINT	GetAllTimeRequests() const		{return alltimerequested;}
	UINT	GetAllTimeAccepts() const		{return alltimeaccepted;}
	uint64	GetAllTimeTransferred() const	{return alltimetransferred;}
	
	CKnownFile* fileParent;
	//Morph Start - Added by AndCycle, Equal Chance For Each File
	void	SetSharedTime(uint32 sharedTime)		{ shareStartTime = time(NULL) - sharedTime; }
	uint32	GetSharedTime()							{ return time(NULL) - shareStartTime; }
	uint32	GetShareStartTime()						{ return shareStartTime; }
	double	GetEqualChanceValue();
	CString	GetEqualChanceValueString(bool detail = true);
	//Morph End - Added by AndCycle, Equal Chance For Each File
private:
	//MORPH START - Added by IceCream SLUGFILLER: Spreadbars
	CRBMap<uint32, uint32> spreadlist;
	static CBarShader s_SpreadBar;
	//MORPH - Added by SiRoB, Reduce SpreadBar CPU consumption
	bool	InChangedSpreadSortValue;
	bool	InChangedFullSpreadCount;
	bool	InChangedSpreadBar;
	CBitmap m_bitmapSpreadBar;
	int		lastSize;
	bool	lastbFlat;
	float	lastSpreadSortValue;
	float	lastFullSpreadCount;
	//MORPH - Added by SiRoB, Reduce SpreadBar CPU consumption
	//MORPH END   - Added by IceCream, SLUGFILLER: Spreadbars
	uint32 requested;
	uint32 accepted;
	uint64 transferred;
	uint32 alltimerequested;
	uint64 alltimetransferred;
	uint32 alltimeaccepted;

	//Morph Start - Added by AndCycle, Equal Chance For Each File
	uint32	shareStartTime;
	bool	m_bInChangedEqualChanceValue;
	double	m_dLastEqualChanceSemiValue;
	//Morph End - Added by AndCycle, Equal Chance For Each File
};

/*
					   CPartFile
					 /
		  CKnownFile
		/
CAbstractFile 
		\ 
		  CSearchFile
*/
class CAbstractFile: public CObject
{
	DECLARE_DYNAMIC(CAbstractFile)

public:
	CAbstractFile();
	virtual ~CAbstractFile() { }

	const CString& GetFileName() const { return m_strFileName; }
	virtual void SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars = false, bool bAutoSetFileType = true); // 'bReplaceInvalidFileSystemChars' is set to 'false' for backward compatibility!

	// returns the ED2K file type (an ASCII string)
	const CString& GetFileType() const { return m_strFileType; }
	virtual void SetFileType(LPCTSTR pszFileType);

	// returns the file type which is used to be shown in the GUI
	CString GetFileTypeDisplayStr() const;

	const uchar* GetFileHash() const { return m_abyFileHash; }
	void SetFileHash(const uchar* pucFileHash);
	bool HasNullHash() const;

	uint32 GetFileSize() const { return m_nFileSize; }
	virtual void SetFileSize(uint32 nFileSize) { m_nFileSize = nFileSize; }

	uint32 GetIntTagValue(uint8 tagname) const;
	uint32 GetIntTagValue(LPCSTR tagname) const;
	bool GetIntTagValue(uint8 tagname, uint32& ruValue) const;
	const CString& GetStrTagValue(uint8 tagname) const;
	const CString& GetStrTagValue(LPCSTR tagname) const;
	CTag* GetTag(uint8 tagname, uint8 tagtype) const;
	CTag* GetTag(LPCSTR tagname, uint8 tagtype) const;
	CTag* GetTag(uint8 tagname) const;
	CTag* GetTag(LPCSTR tagname) const;
	void AddTagUnique(CTag* pTag);
	const CArray<CTag*,CTag*>& GetTags() const { return taglist; }

#ifdef _DEBUG
	// Diagnostic Support
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CString m_strFileName;
	uchar	m_abyFileHash[16];
	uint32	m_nFileSize;
	CString m_strComment;
	uint8	m_uRating;
	CString m_strFileType;
	CArray<CTag*,CTag*> taglist;
};

class CKnownFile : public CAbstractFile
{
	DECLARE_DYNAMIC(CKnownFile)

public:
	CKnownFile();
	~CKnownFile();

	// MORPH START - Added by Commander, WebCache 1.2e
	bool ReleaseViaWebCache; //JP webcache release
	uint32 GetNumberOfClientsRequestingThisFileUsingThisWebcache(CString webcachename, uint32 maxCount); //JP webcache release
	void SetReleaseViaWebCache(bool WCRelease) {ReleaseViaWebCache=WCRelease;} //JP webcache release
	// MORPH END - Added by Commander, WebCache 1.2e

	virtual void SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars = false); // 'bReplaceInvalidFileSystemChars' is set to 'false' for backward compatibility!

	const CString& GetPath() const	{return m_strDirectory;}
	void	SetPath(LPCTSTR path);

	const CString& GetFilePath() const { return m_strFilePath; }
	void SetFilePath(LPCTSTR pszFilePath);

	// SLUGFILLER: mergeKnown, for TAHO, .met file control
	void	SetLastSeen()	{ m_dwLastSeen = time(NULL); }
	uint32	GetLastSeen()	{ return m_dwLastSeen; }
	// SLUGFILLER: mergeKnown, for TAHO, .met file control

	virtual bool CreateFromFile(LPCTSTR directory, LPCTSTR filename, LPVOID pvProgressParam); // create date, hashset and tags from a file
	virtual bool IsPartFile() const { return false; }
	virtual bool LoadFromFile(CFileDataIO* file);	//load date, hashset and tags from a .met file
	bool	WriteToFile(CFileDataIO* file);
	//MORPH START - Added by SiRoB, Import Parts [SR13]
	bool	SR13_ImportParts();
	//MORPH END   - Added by SiRoB, Import Parts [SR13]
	bool	CreateAICHHashSetOnly();

	// last file modification time in (DST corrected, if NTFS) real UTC format
	// NOTE: this value can *not* be compared with NT's version of the UTC time
	CTime	GetUtcCFileDate() const { return CTime(m_tUtcLastModified); }
	uint32	GetUtcFileDate() const { return m_tUtcLastModified; }

	virtual void SetFileSize(uint32 nFileSize);

	// local available part hashs
	uint16	GetHashCount() const	{return hashlist.GetCount();}
	uchar*	GetPartHash(uint16 part) const;
	const CArray<uchar*, uchar*>& GetHashset() const { return hashlist; }
	bool	SetHashset(const CArray<uchar*, uchar*>& aHashset);

	// nr. of part hashs according the file size wrt ED2K protocol
	UINT	GetED2KPartHashCount() const { return m_iED2KPartHashCount; }

	// nr. of 9MB parts (file data)
	__inline uint16 GetPartCount() const { return m_iPartCount; }

	// nr. of 9MB parts according the file size wrt ED2K protocol (OP_FILESTATUS)
	__inline uint16 GetED2KPartCount() const { return m_iED2KPartCount; }

	// file upload priority
	uint8	GetUpPriority(void) const { return m_iUpPriority; }
	void	SetUpPriority(uint8 iNewUpPriority, bool bSave = true);
	bool	IsAutoUpPriority(void) const { return m_bAutoUpPriority; }
	void	SetAutoUpPriority(bool NewAutoUpPriority) { m_bAutoUpPriority = NewAutoUpPriority; }
	void	UpdateAutoUpPriority();
	
	// This has lost it's meaning here.. This is the total clients we know that want this file..
	// Right now this number is used for auto priorities..
	// This may be replaced with total complete source known in the network..
	uint32	GetQueuedCount() { return m_ClientUploadList.GetCount();}

	bool	LoadHashsetFromFile(CFileDataIO* file, bool checkhash);

	bool	HideOvershares(CSafeMemFile* file, CUpDownClient* client);	// SLUGFILLER: hideOS

	void	AddUploadingClient(CUpDownClient* client);
	void	RemoveUploadingClient(CUpDownClient* client);
	virtual void	UpdatePartsInfo();
	virtual void	DrawShareStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool bFlat) /*const*/;

	// comment 
	const CString& GetFileComment() /*const*/;
	void	SetFileComment(LPCTSTR pszComment);

	uint8	GetFileRating() /*const*/;
	void	SetFileRating(uint8 uRating);

	bool	GetPublishedED2K() const { return m_PublishedED2K; }
	void	SetPublishedED2K( bool val );

	uint32	GetKadFileSearchID() const { return kadFileSearchID; }
	void	SetKadFileSearchID( uint32 id )	{kadFileSearchID = id;} //Don't use this unless you know what your are DOING!! (Hopefully I do.. :)

	uint32	GetPublishedKadSrc() const { return m_PublishedKadSrc; }
	void	SetPublishedKadSrc();

	const Kademlia::WordList& GetKadKeywords() const { return wordlist; }

	uint32	GetLastPublishTimeKadSrc() const { return m_lastPublishTimeKadSrc; }
	void	SetLastPublishTimeKadSrc( uint32 val ) {m_lastPublishTimeKadSrc = val;}

	bool	PublishSrc();

	// file sharing
	virtual	Packet*	CreateSrcInfoPacket(CUpDownClient* forClient) const;
	UINT	GetMetaDataVer() const { return m_uMetaDataVer; }
	void	UpdateMetaDataTags();
	void	RemoveMetaDataTags();

	// preview
	bool	IsMovie() const;
	bool	IsMusic() const; //MORPH - Added by IceCream, added preview also for music files
	bool	IsCDImage() const; //MORPH - Added by IceCream, for defeat 0-filler
	bool	IsDocument() const; //MORPH - Added by IceCream, for defeat 0-filler

	virtual	bool	GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender);
	virtual void	GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender);

	// aich
	CAICHHashSet*	GetAICHHashset() const							{return m_pAICHHashSet;}
	void			SetAICHHashset(CAICHHashSet* val)				{m_pAICHHashSet = val;}
	// last file modification time in (DST corrected, if NTFS) real UTC format
	// NOTE: this value can *not* be compared with NT's version of the UTC time
	uint32	m_tUtcLastModified;

	CFileStatistic statistic;
	time_t m_nCompleteSourcesTime;
	uint16 m_nCompleteSourcesCount;
	uint16 m_nCompleteSourcesCountLo;
	uint16 m_nCompleteSourcesCountHi;
	CUpDownClientPtrList m_ClientUploadList;
	CArray<uint16,uint16> m_AvailPartFrequency;
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	uint16 m_nVirtualCompleteSourcesCount;
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	
	CArray<uint16,uint16> m_PartSentCount;	// SLUGFILLER: hideOS
	bool ShareOnlyTheNeed(CSafeMemFile* file, CUpDownClient* client);//wistily Share only the need

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
	//MORPH START - Added by SiRoB, HIDEOS
	void	SetHideOS(int newValue) {m_iHideOS = newValue;}
	int		GetHideOS() const {return m_iHideOS;}
	void	SetSelectiveChunk(int newValue) {m_iSelectiveChunk = newValue;}
	int		GetSelectiveChunk() const {return m_iSelectiveChunk;}
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, Avoid misusing of hideOS
	uint8	HideOSInWork() const;
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

protected:
	//preview
	bool	GrabImage(CString strFileName,uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender);
	bool	LoadTagsFromFile(CFileDataIO* file);
	bool	LoadDateFromFile(CFileDataIO* file);
	void	CreateHash(CFile* pFile, UINT uSize, uchar* pucHash, CAICHHashTree* pShaHashOut = NULL) const;
	bool	CreateHash(FILE* fp, UINT uSize, uchar* pucHash, CAICHHashTree* pShaHashOut = NULL) const;
	bool	CreateHash(const uchar* pucData, UINT uSize, uchar* pucHash, CAICHHashTree* pShaHashOut = NULL) const;
	void	LoadComment();
	uint16	CalcPartSpread(CArray<uint32, uint32>& partspread, CUpDownClient* client);	// SLUGFILLER: hideOS
	CArray<uchar*,uchar*> hashlist;
	CString	m_strDirectory;
	CString m_strFilePath;
	CAICHHashSet*			m_pAICHHashSet; 
private:
	static CBarShader s_ShareStatusBar;
	uint16	m_iPartCount;
	uint16  m_iED2KPartCount;
	uint16	m_iED2KPartHashCount;
	uint8	m_iUpPriority;
	bool	m_bAutoUpPriority;
	bool	m_bCommentLoaded;
	bool	m_PublishedED2K;
	uint32	kadFileSearchID;
	uint32	m_lastPublishTimeKadSrc;
	uint32	m_PublishedKadSrc;
	Kademlia::WordList wordlist;
	UINT	m_uMetaDataVer;
	uint32	m_dwLastSeen;	// SLUGFILLER: mergeKnown, for TAHO, .met file control

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

// constants for MD4Transform
#define S11 3
#define S12 7
#define S13 11
#define S14 19
#define S21 3
#define S22 5
#define S23 9
#define S24 13
#define S31 3
#define S32 9
#define S33 11
#define S34 15

// basic MD4 functions
#define MD4_F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define MD4_G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define MD4_H(x, y, z) ((x) ^ (y) ^ (z))

// rotates x left n bits
// 15-April-2003 Sony: use MSVC intrinsic to save a few cycles
#ifdef _MSC_VER
#pragma intrinsic(_rotl)
#define MD4_ROTATE_LEFT(x, n) _rotl((x), (n))
#else
#define MD4_ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#endif

// partial transformations
#define MD4_FF(a, b, c, d, x, s) \
{ \
  (a) += MD4_F((b), (c), (d)) + (x); \
  (a) = MD4_ROTATE_LEFT((a), (s)); \
}

#define MD4_GG(a, b, c, d, x, s) \
{ \
  (a) += MD4_G((b), (c), (d)) + (x) + (uint32)0x5A827999; \
  (a) = MD4_ROTATE_LEFT((a), (s)); \
}

#define MD4_HH(a, b, c, d, x, s) \
{ \
  (a) += MD4_H((b), (c), (d)) + (x) + (uint32)0x6ED9EBA1; \
  (a) = MD4_ROTATE_LEFT((a), (s)); \
}

static void MD4Transform(uint32 Hash[4], uint32 x[16]);

