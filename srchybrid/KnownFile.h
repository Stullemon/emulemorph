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
#include "otherstructs.h"
#include "packets.h"
#include "types.h"
#include "BarShader.h"
#include <afxcoll.h>
#include <afxtempl.h>
#include <afxcmn.h>
#include "Kademlia/Kademlia/Kademlia.h"
#include "loggable.h"

class CxImage;

class CFileStatistic{
	friend class CKnownFile;
public:
	//MORPH START - Added by SiRoB, Reduce SpreadBar CPU consumption
	//CFileStatistic()					{requested = transferred = accepted = alltimerequested= alltimetransferred = alltimeaccepted = 0;}
	CFileStatistic(){
		requested = transferred = accepted = alltimerequested= alltimetransferred = alltimeaccepted = 0;
		InChangedSpreadSortValue = false;
		InChangedFullSpreadCount = false;
		InChangedSpreadBar = false;
		lastSpreadSortValue = 0;;
		lastFullSpreadCount = 0;
		m_pbitmapOldSpreadBar = NULL;
		lastused = time(NULL); //EastShare - Added by TAHO, .met file control
	}
	//MORPH END   - Added by SiRoB, Reduce SpreadBar CPU consumption
	~CFileStatistic();	//MORPH - Added by IceCream, SLUGFILLER: Spreadbars
	void	AddRequest();
	void	AddAccepted();
	//MORPH START - Added by IceCream SLUGFILLER: Spreadbars
	void	AddTransferred(uint32 start, uint32 bytes);
	void	AddBlockTransferred(uint32 start, uint32 end, uint32 count);
	void	DrawSpreadBar(CDC* dc, RECT* rect, bool bFlat);
	float	GetSpreadSortValue();
	float	GetFullSpreadCount();
	//MORPH END - Added by IceCream SLUGFILLER: Spreadbars
	void	Merge(CFileStatistic *other);	// SLUGFILLER: mergeKnown
	uint16	GetRequests()				{return requested;}
	uint16	GetAccepts()				{return accepted;}
	uint64	GetTransferred()			{return transferred;}
	uint16	GetAllTimeRequests()		{return alltimerequested;}
	uint16	GetAllTimeAccepts()			{return alltimeaccepted;}
	uint64	GetAllTimeTransferred()		{return alltimetransferred;}
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
	CDC 	m_dcSpreadBar;
	CBitmap m_bitmapSpreadBar;
	CBitmap *m_pbitmapOldSpreadBar;
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
class CAbstractFile: public CLoggable
{
public:
	CAbstractFile();
	virtual ~CAbstractFile() { }

	const CString& GetFileName() const { return m_strFileName; }
	void SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars = false); // 'bReplaceInvalidFileSystemChars' is set to 'false' for backward compatibility!

	const CString& GetFileType() const { return m_strFileType; }
	void SetFileType(LPCTSTR pszFileType);

	const uchar* GetFileHash() const { return m_abyFileHash; }

	uint32 GetFileSize() const { return m_nFileSize; }
	void SetFileSize(uint32 nFileSize) { m_nFileSize = nFileSize; }

	uint32 GetIntTagValue(uint8 tagname) const;
	uint32 GetIntTagValue(LPCSTR tagname) const;
	LPCSTR GetStrTagValue(uint8 tagname) const;
	LPCSTR GetStrTagValue(LPCSTR tagname) const;
	CTag* GetTag(uint8 tagname, uint8 tagtype) const;
	CTag* GetTag(LPCSTR tagname, uint8 tagtype) const;
	CTag* GetTag(uint8 tagname) const;
	CTag* GetTag(LPCSTR tagname) const;
	void AddTagUnique(CTag* pTag);
	const CArray<CTag*,CTag*>& GetTags() const { return taglist; }

protected:
	CString m_strFileName;
	uchar	m_abyFileHash[16];
	uint32	m_nFileSize;
	CString m_strComment;
	int8	m_iRate;
	CString m_strFileType;	// this holds the localized(!) file type, TODO: change to ed2k file type
	CArray<CTag*,CTag*> taglist;
};

class CUpDownClient;

class CKnownFile : public CAbstractFile
{
public:
	CKnownFile();
	~CKnownFile();

	virtual bool	CreateFromFile(LPCTSTR directory,LPCTSTR filename); // create date, hashset and tags from a file
	
	const CString& GetPath() const	{return m_strDirectory;}
	void	SetPath(LPCTSTR path);

	const CString& GetFilePath() const { return m_strFilePath; }
	void SetFilePath(LPCTSTR pszFilePath);

	virtual	bool	IsPartFile()	{return false;}
	virtual bool	LoadFromFile(CFile* file);	//load date, hashset and tags from a .met file
	bool	WriteToFile(CFile* file);	
	CTime	GetCFileDate()			{return CTime(date);}
	uint32	GetFileDate()			{return date;}
	CTime	GetCrCFileDate()		{return CTime(dateC);}
	uint32	GetCrFileDate()			{return dateC;}

	void SetFileSize(uint32 nFileSize);

	// local available part hashs
	uint16	GetHashCount() const	{return hashlist.GetCount();}
	uchar*	GetPartHash(uint16 part) const;

	// nr. of part hashs according the file size wrt ED2K protocol
	UINT	GetED2KPartHashCount() const { return m_iED2KPartHashCount; }

	// nr. of 9MB parts (file data)
	__inline uint16 GetPartCount() const { return m_iPartCount; }

	// nr. of 9MB parts according the file size wrt ED2K protocol (OP_FILESTATUS)
	__inline uint16 GetED2KPartCount() const { return m_iED2KPartCount; }

	uint32	date;
	uint32	dateC;

	// file upload priority
	uint8	GetUpPriority(void)		{return m_iUpPriority;};
	void	SetUpPriority(uint8 iNewUpPriority, bool m_bsave = true);
	bool	IsAutoUpPriority(void)		{return m_bAutoUpPriority;};
	void	SetAutoUpPriority(bool NewAutoUpPriority) {m_bAutoUpPriority = NewAutoUpPriority;};
	void	UpdateAutoUpPriority();
	void	AddQueuedCount()			{m_iQueuedCount++; UpdateAutoUpPriority();}
	void	SubQueuedCount()			{if( m_iQueuedCount != 0 ) m_iQueuedCount--; UpdateAutoUpPriority();}
	uint32	GetQueuedCount()			{return m_iQueuedCount; /*call func after 'return'!? what's the ident here?*/ /*UpdateAutoUpPriority();*/}
	// shared file view permissions (all, only friends, no one)
	uint8	GetPermissions(void)	{ return m_iPermissions; };
	void	SetPermissions(uint8 iNewPermissions) {m_iPermissions = iNewPermissions;};

	bool	LoadHashsetFromFile(CFile* file, bool checkhash);

	bool	HideOvershares(CFile* file, CUpDownClient* client);	// SLUGFILLER: hideOS

	void	AddUploadingClient(CUpDownClient* client);
	void	RemoveUploadingClient(CUpDownClient* client);
	void	NewAvailPartsInfo();
	void	DrawShareStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat);

	CFileStatistic statistic;
	// comment 
	CString	GetFileComment()		{if (!m_bCommentLoaded) LoadComment(); return m_strComment;} 
	void	SetFileComment(CString strNewComment);
	void	SetFileRate(int8 iNewRate); 
	int8	GetFileRate()			{if (!m_bCommentLoaded) LoadComment(); return m_iRate;}
	void	SetPublishedED2K( bool val );
	bool	GetPublishedED2K()	{return m_PublishedED2K;}
	uint32	GetKadFileSearchID()	{return kadFileSearchID;}
	void	SetKadFileSearchID( uint32 id )	{kadFileSearchID = id;} //Don't use this unless you know what your are DOING!! (Hopefully I do.. :)
	void	SetPublishedKadSrc();
	uint32	GetPublishedKadSrc()	{return m_PublishedKadSrc;}
	void	SetPublishedKadKey();
	uint32	GetPublishedKadKey()	{return m_PublishedKadKey;}
	uint32	GetKadKeywordCount() {return m_keywordcount;}
	uint32	GetLastPublishTimeKadSrc() {return m_lastPublishTimeKadSrc;}
	void	SetLastPublishTimeKadSrc( uint32 val ) {m_lastPublishTimeKadSrc = val;}
	uint32	GetLastPublishTimeKadKey() {return m_lastPublishTimeKadKey;}
	void	SetLastPublishTimeKadKey( uint32 val ) {m_lastPublishTimeKadKey = val;}
	int		PublishKey( Kademlia::CUInt128* nextID);
	bool	PublishSrc( Kademlia::CUInt128* nextID);

	// file sharing
	virtual	Packet*	CreateSrcInfoPacket(CUpDownClient* forClient);
	void GetMetaDataTags();

	void	UpdateClientUploadList();	// #zegzav:updcliuplst

	// preview
	bool	IsMovie();
	virtual	bool	GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender);
	virtual void	GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender);
	//MORPH START - Added by SiRoB, ZZ Upload System 20030723-0133
	//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
	void    SetPowerShared(int newValue) {m_powershared = newValue;};
	bool    GetPowerShared() {return ((m_powershared == 1) || ((m_powershared == 2) && m_bPowerShareAuto)) && m_bPowerShareAuthorized;}
	//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	int		GetPowerSharedMode() {return m_powershared;}
	bool	GetPowerShareAuthorized() {return m_bPowerShareAuthorized;}
	bool	GetPowerShareAuto() {return m_bPowerShareAuto;}
	void	UpdatePowerShareLimit(bool authorizepowershare,bool autopowershare) {m_bPowerShareAuthorized = authorizepowershare;m_bPowerShareAuto = autopowershare;}
   	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH END - Added by SiRoB, ZZ Upload System 20030723-0133
protected:
	//preview
	bool	GrabImage(CString strFileName,uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender);

	bool	LoadTagsFromFile(CFile* file);
	bool	LoadDateFromFile(CFile* file);
	void	CreateHashFromFile(FILE* file, int Length, uchar* Output)	{CreateHashFromInput(file,0,Length,Output,0);}
	void	CreateHashFromFile(CFile* file, int Length, uchar* Output)	{CreateHashFromInput(0,file,Length,Output,0);}
	void	CreateHashFromString(uchar* in_string, int Length, uchar* Output)	{CreateHashFromInput(0,0,Length,Output,in_string);}
	void	LoadComment();//comment
	CArray<uchar*,uchar*> hashlist;
	CString	m_strDirectory;
	CString m_strFilePath;
	//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
	uint16	CalcPartSpread(CArray<uint32, uint32>& partspread, CUpDownClient* client);	// SLUGFILLER: hideOS
	//MORPH END   - Added by SiRoB, SLUGFILLER: hideOS

private:
	void	CreateHashFromInput(FILE* file,CFile* file2, int Length, uchar* Output, uchar* = 0);
	bool	m_bCommentLoaded;
	uint16	m_iPartCount;
	uint16  m_iED2KPartCount;
	uint16	m_iED2KPartHashCount;
	uint8	m_iUpPriority;
	uint8	m_iPermissions;
	bool	m_bAutoUpPriority;
	uint32	m_iQueuedCount;
	static	CBarShader s_ShareStatusBar;
	bool	m_PublishedED2K;
	uint32	kadFileSearchID;
	uint32	m_lastPublishTimeKadSrc;
	uint32	m_lastPublishTimeKadKey;
	uint32	m_PublishedKadSrc;
	uint32	m_PublishedKadKey;
	uint32	m_keywordcount;
	Kademlia::WordList wordlist;
	//MORPH START - Added by SiRoB,  SharedStatusBar CPU Optimisation
	bool	InChangedSharedStatusBar;
	CDC 	m_dcSharedStatusBar;
	CBitmap m_bitmapSharedStatusBar;
	CBitmap *m_pbitmapOldSharedStatusBar;
	int	lastSize;
	bool	lastonlygreyrect;
	bool	lastbFlat;
	//MORPH END - Added by SiRoB,  SharedStatusBar CPU Optimisation
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	int		m_powershared;
	bool	m_bPowerShareAuthorized;
	bool	m_bPowerShareAuto;
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
public:
	time_t m_nCompleteSourcesTime;
	uint16 m_nCompleteSourcesCount;
	uint16 m_nCompleteSourcesCountLo;
	uint16 m_nCompleteSourcesCountHi;
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	uint16 m_nVirtualCompleteSourcesCountMin;
	uint16 m_nVirtualCompleteSourcesCountMax;
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	CTypedPtrList<CPtrList, CUpDownClient*> m_ClientUploadList;
	CArray<uint16,uint16> m_AvailPartFrequency;
	CArray<uint16,uint16> m_PartSentCount;	// SLUGFILLER: hideOS
	bool ShareOnlyTheNeed(CFile* file);//wistily Share only the need
};

// permission values for shared files
#define PERM_ALL		0
#define PERM_FRIENDS	1
#define PERM_NOONE		2

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

