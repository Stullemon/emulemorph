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
#pragma once
#include "Loggable.h"
#include "BarShader.h"
#include <list>

class CTag;
class CxImage;
namespace Kademlia{
class CUInt128;
typedef std::list<CString> WordList;
};
class CUpDownClient;
class Packet;
class CFileDataIO;
typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

class CFileStatistic
{
	friend class CKnownFile;
public:
	//MORPH START - Added by SiRoB, Reduce SpreadBar CPU consumption
	//	CFileStatistic()					{requested = transferred = accepted = alltimerequested= alltimetransferred = alltimeaccepted = 0;}
	CFileStatistic(){
		requested = 0;
		transferred = 0;
		accepted = 0;
		alltimerequested= 0;
		alltimetransferred = 0;
		alltimeaccepted = 0;
		InChangedSpreadSortValue = false;
		InChangedFullSpreadCount = false;
		InChangedSpreadBar = false;
		lastSpreadSortValue = 0;;
		lastFullSpreadCount = 0;
		lastused = time(NULL); //EastShare - Added by TAHO, .met file control
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
	uint16	GetRequests() const				{return requested;}
	uint16	GetAccepts() const				{return accepted;}
	uint64	GetTransferred() const			{return transferred;}
	uint16	GetAllTimeRequests() const		{return alltimerequested;}
	uint16	GetAllTimeAccepts() const		{return alltimeaccepted;}
	uint64	GetAllTimeTransferred() const	{return alltimetransferred;}
	
	CKnownFile* fileParent;
	uint32	GetLastUsed()				{return lastused;} // EastShare - Added by TAHO, .met fiel control
	void	SetLastUsed(uint32 inLastUsed)				{lastused = inLastUsed;} // EastShare - Added by TAHO, .met fiel control
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
	uint16 requested;
	uint16 accepted;
	uint64 transferred;
	uint32 alltimerequested;
	uint64 alltimetransferred;
	uint32 alltimeaccepted;
	uint32 lastused; //EastShare - Added by TAHO, .met file control
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
class CAbstractFile: public CObject, public CLoggable
{
	DECLARE_DYNAMIC(CAbstractFile)

public:
	CAbstractFile();
	virtual ~CAbstractFile() { }

	const CString& GetFileName() const { return m_strFileName; }
	virtual void SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars = false); // 'bReplaceInvalidFileSystemChars' is set to 'false' for backward compatibility!

	const CString& GetFileType() const { return m_strFileType; }
	virtual void SetFileType(LPCTSTR pszFileType);

	const uchar* GetFileHash() const { return m_abyFileHash; }

	uint32 GetFileSize() const { return m_nFileSize; }
	virtual void SetFileSize(uint32 nFileSize) { m_nFileSize = nFileSize; }

	uint32 GetIntTagValue(uint8 tagname) const;
	uint32 GetIntTagValue(LPCSTR tagname) const;
	bool GetIntTagValue(uint8 tagname, uint32& ruValue) const;
	LPCSTR GetStrTagValue(uint8 tagname) const;
	LPCSTR GetStrTagValue(LPCSTR tagname) const;
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
	uint8	m_iRate;
	CString m_strFileType;	// this holds the localized(!) file type, TODO: change to ed2k file type
	CArray<CTag*,CTag*> taglist;
};

class CKnownFile : public CAbstractFile
{
	DECLARE_DYNAMIC(CKnownFile)

public:
	CKnownFile();
	~CKnownFile();

	virtual void SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars = false); // 'bReplaceInvalidFileSystemChars' is set to 'false' for backward compatibility!

	const CString& GetPath() const	{return m_strDirectory;}
	void	SetPath(LPCTSTR path);

	const CString& GetFilePath() const { return m_strFilePath; }
	void SetFilePath(LPCTSTR pszFilePath);

	virtual bool	CreateFromFile(LPCTSTR directory,LPCTSTR filename); // create date, hashset and tags from a file
	virtual bool IsPartFile() const { return false; }
	virtual bool LoadFromFile(CFileDataIO* file);	//load date, hashset and tags from a .met file
	bool	WriteToFile(CFileDataIO* file);
	CTime	GetCFileDate() const { return CTime(date); }
	uint32	GetFileDate() const { return date; }
	CTime	GetCrCFileDate() const { return CTime(dateC); }
	uint32	GetCrFileDate() const { return dateC; }

	virtual void SetFileSize(uint32 nFileSize);

	// local available part hashs
	uint16	GetHashCount() const	{return hashlist.GetCount();}
	uchar*	GetPartHash(uint16 part) const;

	// SLUGFILLER: SafeHash remove - removed unnececery hash counter

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

	bool	HideOvershares(CFile* file, CUpDownClient* client);	// SLUGFILLER: hideOS

	void	AddUploadingClient(CUpDownClient* client);
	void	RemoveUploadingClient(CUpDownClient* client);
	virtual void	UpdatePartsInfo();
	virtual void	DrawShareStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool bFlat) /*const*/;

	// comment 
	CString GetFileComment() /*const*/ { if (!m_bCommentLoaded) LoadComment(); return m_strComment; }
	void	SetFileComment(CString strNewComment);

	uint8	GetFileRate() /*const*/ { if (!m_bCommentLoaded) LoadComment(); return m_iRate; }
	void	SetFileRate(uint8 iNewRate);

	bool	GetPublishedED2K() const { return m_PublishedED2K; }
	void	SetPublishedED2K( bool val );

	uint32	GetKadFileSearchID() const { return kadFileSearchID; }
	void	SetKadFileSearchID( uint32 id )	{kadFileSearchID = id;} //Don't use this unless you know what your are DOING!! (Hopefully I do.. :)

	uint32	GetPublishedKadSrc() const { return m_PublishedKadSrc; }
	void	SetPublishedKadSrc();

	const Kademlia::WordList& GetKadKeywords() const { return wordlist; }

	uint32	GetLastPublishTimeKadSrc() const { return m_lastPublishTimeKadSrc; }
	void	SetLastPublishTimeKadSrc( uint32 val ) {m_lastPublishTimeKadSrc = val;}
	int		PublishKey( Kademlia::CUInt128* nextID);
	bool	PublishSrc( Kademlia::CUInt128* nextID);

	// file sharing
	virtual	Packet*	CreateSrcInfoPacket(CUpDownClient* forClient) const;
	UINT	GetMetaDataVer() const { return m_uMetaDataVer; }
	void	UpdateMetaDataTags();
	void	RemoveMetaDataTags();

	// preview
	bool	IsMovie() const;
	virtual	bool	GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender);
	virtual void	GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender);

	uint32	date;
	uint32	dateC;
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
	bool ShareOnlyTheNeed(CFile* file);//wistily Share only the need

	//Morph - added by AndCycle, Equal Chance For Each File
	double	GetEqualChanceValue();
	CString	GetEqualChanceValueString(bool detail = true);

#ifdef _DEBUG
	// Diagnostic Support
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	//MORPH START - Added by SiRoB, Show Permissions
	// shared file view permissions (all, only friends, no one)
	int		GetPermissions(void) const	{ return m_iPermissions; };
	void	SetPermissions(int iNewPermissions);
	//MORPH END   - Added by SiRoB, Show Permissions
	//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
	void    SetPowerShared(int newValue) {m_powershared = newValue;};
	bool    GetPowerShared() const {return ((m_powershared == 1) || ((m_powershared == 2) && m_bPowerShareAuto)) && m_bPowerShareAuthorized;}
	//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by SiRoB, HIDEOS
	void	SetHideOS(int newValue) {m_iHideOS = newValue;};
	int		GetHideOS() const {return m_iHideOS;}
	void	SetSelectiveChunk(int newValue) {m_iSelectiveChunk = newValue;};
	int		GetSelectiveChunk() const {return m_iSelectiveChunk;}
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	void	SetShareOnlyTheNeed(int newValue) {m_iShareOnlyTheNeed = newValue;};
	int		GetShareOnlyTheNeed() const {return m_iShareOnlyTheNeed;}
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	int		GetPowerSharedMode() const {return m_powershared;}
	bool	GetPowerShareAuthorized() const {return m_bPowerShareAuthorized;}
	bool	GetPowerShareAuto() const {return m_bPowerShareAuto;}
	void	UpdatePowerShareLimit(bool authorizepowershare,bool autopowershare) {m_bPowerShareAuthorized = authorizepowershare;m_bPowerShareAuto = autopowershare;}
   	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH END - Added by SiRoB, ZZ Upload System 20030723-0133

	// Mighty Knife: CRC32-Tag
	bool    IsCRC32Calculated () const			{return m_sCRC32[0]!='\0';}
	CString GetLastCalculatedCRC32 () const		{return m_sCRC32;}
	// The CRC32 is not created within this object but written to this object:
	void    SetLastCalculatedCRC32 (const char* _CRC) {strcpy (m_sCRC32,_CRC);}

	// [end] Mighty Knife

protected:
	//preview
	bool	GrabImage(CString strFileName,uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender);
	bool	LoadTagsFromFile(CFileDataIO* file);
	bool	LoadDateFromFile(CFileDataIO* file);
	void	CreateHashFromFile(FILE* file, int Length, uchar* Output) const { CreateHashFromInput(file, NULL, Length, Output, NULL); }
	void	CreateHashFromFile(CFile* file, int Length, uchar* Output) const { CreateHashFromInput(NULL, file, Length, Output, NULL); }
	void	CreateHashFromString(uchar* in_string, int Length, uchar* Output) const { CreateHashFromInput(NULL, NULL, Length, Output, in_string); }
	void	LoadComment();
	uint16	CalcPartSpread(CArray<uint32, uint32>& partspread, CUpDownClient* client);	// SLUGFILLER: hideOS
	CArray<uchar*,uchar*> hashlist;
	CString	m_strDirectory;
	CString m_strFilePath;

private:
	void	CreateHashFromInput(FILE* file, CFile* file2, int Length, uchar* Output, uchar* = NULL) const;

	static CBarShader s_ShareStatusBar;
	uint16	m_iPartCount;
	uint16  m_iED2KPartCount;
	// SLUGFILLER: SafeHash remove - removed unnececery hash counter
	uint8	m_iUpPriority;
	bool	m_bAutoUpPriority;
	bool	m_bCommentLoaded;
	bool	m_PublishedED2K;
	uint32	kadFileSearchID;
	uint32	m_lastPublishTimeKadSrc;
	uint32	m_PublishedKadSrc;
	Kademlia::WordList wordlist;
	UINT	m_uMetaDataVer;
	//MORPH START - Added by SiRoB,  SharedStatusBar CPU Optimisation
	bool	InChangedSharedStatusBar;
	CDC 	m_dcSharedStatusBar;
	CBitmap m_bitmapSharedStatusBar;
	CBitmap *m_pbitmapOldSharedStatusBar;
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

	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	int		m_iShareOnlyTheNeed;
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea

	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	int		m_powershared;
	bool	m_bPowerShareAuthorized;
	bool	m_bPowerShareAuto;
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing

	// Mighty Knife: CRC32-Tag
	char    m_sCRC32 [16];
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
