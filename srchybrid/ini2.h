// Ini.h: Schnittstelle für die Klasse CIni.

// Autor: Michael Schikora
// Mail:  schiko@schikos.de
//
// If you found this code useful,
// please let me know
//
// How to use:
//
//
//void CMyClass::UpdateFromIni(BOOL bFromIni)
//{
//   CIni ini(m_strFileName,m_strSection);
//   ini.SER_GET(bFromIni,m_nValueXY); 
//   ini.SER_GET(bFromIni,m_strValue);
//   ini.SER_ARR(bFromIni,m_arValue,MAX_AR); 
//   ini.SER_ARR(bFromIni,m_ar3D,3);
//   //ore with default values 
//   ini.SER_GETD(bFromIni,m_nValueXY,5); 
//   ini.SER_GETD(bFromIni,m_strValue,"Hello");
//   ini.SER_ARRD(bFromIni,m_arValue,MAX_AR,10); 
//   ini.SER_ARRD(bFromIni,m_ar3D,3,5); 
//}
#if !defined(AFX_INI_H__EEBAF800_182A_11D3_B51F_00104B4A13B4__INCLUDED_)
#define AFX_INI_H__EEBAF800_182A_11D3_B51F_00104B4A13B4__INCLUDED_

#pragma once

#define SER_GET(bGet,value) SerGet(bGet,value,#value)
#define SER_ARR(bGet,value,n) SerGet(bGet,value,n,#value)
#define SER_GETD(bGet,value,default) SerGet(bGet,value,#value,NULL,default)
#define SER_ARRD(bGet,value,n,default) SerGet(bGet,value,n,#value,default)

class CIni  
{
public:

#ifdef __NEVER_DEFINED__
   // MAKRO: SerGet(bGet,value,#value)
   int SER_GET(BOOL bGet,int value);
   // MAKRO: SerGet(bGet,value,n,#value)
   int SER_ARR(bGet,int* value,int n);
#endif
   // If the IniFilename contains no path,
   // the module-directory will be add to the FileName,
   // to avoid storing in the windows-directory
   // bModulPath=TRUE: ModulDir, bModulPath=FALSE: CurrentDir
	static void AddModulPath(CString& strFileName, BOOL bModulPath = TRUE);
	static CString GetDefaultSection();
	static CString GetDefaultIniFile(BOOL bModulPath = TRUE);

	CIni( BOOL bModulPath = TRUE);
	CIni(CIni const& Ini, BOOL bModulPath = TRUE);
	CIni(CString const& strFileName, BOOL bModulPath = TRUE);
	CIni(CString const& strFileName, CString const& strSection, BOOL bModulPath = TRUE);
	virtual ~CIni();

	void SetFileName(const CString& strFileName);
	void SetSection(const CString& strSection);
	const CString& GetFileName() const;
	const CString& GetSection() const;

	CString		GetString(LPCTSTR lpszEntry,	LPCTSTR		lpszDefault = NULL,				LPCTSTR lpszSection = NULL);
	double		GetDouble(LPCTSTR lpszEntry,	double		fDefault = 0.0,					LPCTSTR lpszSection = NULL);
	float		GetFloat(LPCTSTR lpszEntry,		float		fDefault = 0.0F,				LPCTSTR lpszSection = NULL);
	int			GetInt(LPCTSTR lpszEntry,		int			nDefault = 0,					LPCTSTR lpszSection = NULL);
	ULONGLONG	GetUInt64(LPCTSTR lpszEntry,	ULONGLONG	nDefault = 0,					LPCTSTR lpszSection = NULL);
	WORD		GetWORD(LPCTSTR lpszEntry,		WORD		nDefault = 0,					LPCTSTR lpszSection = NULL);
	BOOL		GetBool(LPCTSTR lpszEntry,		BOOL		bDefault = FALSE,				LPCTSTR lpszSection = NULL);
	CPoint		GetPoint(LPCTSTR lpszEntry,		CPoint		ptDefault = CPoint(0,0),		LPCTSTR lpszSection = NULL);
	CRect		GetRect(LPCTSTR lpszEntry,		CRect		rectDefault = CRect(0,0,0,0),	LPCTSTR lpszSection = NULL);
	COLORREF	GetColRef(LPCTSTR lpszEntry,	COLORREF	crDefault = RGB(128,128,128),	LPCTSTR lpszSection = NULL);
	BOOL		GetBinary(LPCTSTR lpszEntry,	BYTE** ppData, UINT* pBytes,				LPCTSTR lpszSection = NULL);

	void		WriteString(LPCTSTR strEntry,	LPCTSTR		s,								LPCTSTR lpszSection = NULL);
	void		WriteDouble(LPCTSTR lpszEntry,	double		f,								LPCTSTR lpszSection = NULL);
	void		WriteFloat(LPCTSTR lpszEntry,	float		f,								LPCTSTR lpszSection = NULL);
	void		WriteInt(LPCTSTR lpszEntry,		int			n,								LPCTSTR lpszSection = NULL);
	void		WriteUInt64(LPCTSTR lpszEntry,	ULONGLONG	n,								LPCTSTR lpszSection = NULL);
	void		WriteWORD(LPCTSTR lpszEntry,	WORD		n,								LPCTSTR lpszSection = NULL);
	void		WriteBool(LPCTSTR lpszEntry,	BOOL		b,								LPCTSTR lpszSection = NULL);
	void		WritePoint(LPCTSTR lpszEntry,	CPoint		pt,								LPCTSTR lpszSection = NULL);
	void		WriteRect(LPCTSTR lpszEntry,	CRect		rect,							LPCTSTR lpszSection = NULL);
	void		WriteColRef(LPCTSTR lpszEntry,	COLORREF	cr,								LPCTSTR lpszSection = NULL);
	BOOL		WriteBinary(LPCTSTR lpszEntry,	LPBYTE pData, UINT nBytes,					LPCTSTR lpszSection = NULL);

	void		SerGetString(	BOOL bGet, CString&		s,	LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	LPCTSTR strDefault = NULL);
	void		SerGetDouble(	BOOL bGet, double&		f,	LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	double fDefault = 0.0);
	void		SerGetFloat(	BOOL bGet, float&		f,	LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	float fDefault = 0.0);
	void		SerGetInt(		BOOL bGet, int&			n,	LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	int nDefault = 0);
	void		SerGetDWORD(	BOOL bGet, DWORD&		n,	LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	DWORD nDefault = 0);
	void		SerGetBool(		BOOL bGet, BOOL&		b,	LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	BOOL bDefault = FALSE);
	void		SerGetPoint(	BOOL bGet, CPoint&		pt,	LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	CPoint ptDefault = CPoint(0,0));
	void		SerGetRect(		BOOL bGet, CRect&		rc,	LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	CRect rectDefault = CRect(0,0,0,0));
	void		SerGetColRef(	BOOL bGet, COLORREF&	cr,	LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	COLORREF crDefault = RGB(128,128,128));

	void		SerGet(	BOOL bGet, CString&	 s,	 LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	LPCTSTR lpszDefault = NULL);
	void		SerGet(	BOOL bGet, double&	 f,	 LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	double fDefault = 0.0);
	void		SerGet(	BOOL bGet, float&	 f,	 LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	float fDefault = 0.0F);
	void		SerGet(	BOOL bGet, int&		 n,	 LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	int nDefault = 0);
	void		SerGet(	BOOL bGet, short&	 n,	 LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	int nDefault = 0);
	void		SerGet(	BOOL bGet, DWORD&	 n,	 LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	DWORD nDefault = 0);
	void		SerGet(	BOOL bGet, WORD&	 n,	 LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	DWORD nDefault = 0);
//	void		SerGet(	BOOL bGet, BOOL&	 b,	 LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	BOOL bDefault = FALSE);
	void		SerGet(	BOOL bGet, CPoint&	 pt, LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	CPoint ptDefault = CPoint(0,0));
	void		SerGet(	BOOL bGet, CRect&	 rc, LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	CRect rectDefault = CRect(0,0,0,0));
//	void		SerGet(	BOOL bGet, COLORREF& cr, LPCTSTR lpszEntry,	LPCTSTR lpszSection = NULL,	COLORREF crDefault = RGB(128,128,128));
   
//ARRAYs
	void		SerGet(	BOOL bGet, CString*	s,	int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection = NULL, LPCTSTR lpszDefault = NULL);
	void		SerGet(	BOOL bGet, double*	f,	int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection = NULL, double fDefault = 0.0);
	void		SerGet(	BOOL bGet, float*	f,	int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection = NULL, float fDefault = 0.0F);
	void		SerGet(	BOOL bGet, BYTE*	n,	int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection = NULL, BYTE nDefault = 0);
	void		SerGet(	BOOL bGet, int*		n,	int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection = NULL, int nDefault = 0);
	void		SerGet(	BOOL bGet, short*	n,	int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection = NULL, int nDefault = 0);
	void		SerGet(	BOOL bGet, DWORD*	n,	int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection = NULL, DWORD nDefault = 0);
	void		SerGet(	BOOL bGet, WORD*	n,	int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection = NULL, DWORD nDefault = 0);
	void		SerGet(	BOOL bGet, CPoint*	pt,	int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection = NULL, CPoint ptDefault = CPoint(0,0));
	void		SerGet(	BOOL bGet, CRect*	rc,	int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection = NULL, CRect rectDefault = CRect(0,0,0,0));

	int			Parse(const CString&, int nOffset, CString &strOut);
	void		DeleteKey(LPCTSTR pszKey);
   //MAKRO :
   //SERGET(bGet,value) SerGet(bGet,value,#value)

private:
	void Init(LPCTSTR lpszIniFile, LPCTSTR lpszSection = NULL);
	LPTSTR GetLPCSTR(LPCTSTR lpszEntry, LPCTSTR lpszSection, LPCTSTR lpszDefault);

	BOOL  m_bModulPath;  //TRUE: Filenames without path take the Modulepath
                        //FALSE: Filenames without path take the CurrentDirectory

#define MAX_INI_BUFFER 256
	TCHAR	m_chBuffer[MAX_INI_BUFFER];
	CString m_strFileName;
	CString m_strSection;
//////////////////////////////////////////////////////////////////////
// statische Methoden
//////////////////////////////////////////////////////////////////////
public:
	static CString	Read( LPCTSTR strFileName, LPCTSTR strSection, LPCTSTR strEntry, LPCTSTR strDefault);
	static void		Write(LPCTSTR strFileName, LPCTSTR strSection, LPCTSTR strEntry, LPCTSTR strValue);
};

#endif // !defined(AFX_INI_H__EEBAF800_182A_11D3_B51F_00104B4A13B4__INCLUDED_)
