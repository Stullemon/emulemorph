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

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


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
   static void AddModulPath(CString& strFileName,BOOL bModulPath = TRUE);
   static CString GetDefaultSection();
   static CString GetDefaultIniFile(BOOL bModulPath = TRUE);

	CIni( BOOL bModulPath = TRUE);
	CIni(CIni const& Ini, BOOL bModulPath = TRUE);
	CIni(CString const& strFileName, BOOL bModulPath = TRUE);
	CIni(CString const& strFileName, CString const& strSection, BOOL bModulPath = TRUE);
	virtual ~CIni();

	void SetFileName(CString const& strFileName);
	void SetSection(CString const& strSection);
	CString const& GetFileName() const;
	CString const& GetSection() const;
private:
	void Init(LPCSTR strIniFile, LPCSTR strSection = NULL);
public:
	CString		GetString(CString strEntry,	LPCSTR strDefault=NULL,					LPCSTR strSection = NULL);
	double		GetDouble(CString strEntry,	double fDefault = 0.0,					LPCSTR strSection = NULL);
	float		GetFloat(CString strEntry,	float fDefault = 0.0,					LPCSTR strSection = NULL);
	int			GetInt(CString strEntry,	int nDefault = 0,						LPCSTR strSection = NULL);
	WORD		GetWORD(CString strEntry,	WORD nDefault = 0,						LPCSTR strSection = NULL);
	BOOL		GetBool(CString strEntry,	BOOL bDefault = FALSE,					LPCSTR strSection = NULL);
	CPoint		GetPoint(CString strEntry,	CPoint ptDefault = CPoint(0,0),			LPCSTR strSection = NULL);
	CRect		GetRect(CString strEntry,	CRect rectDefault = CRect(0,0,0,0),		LPCSTR strSection = NULL);
	COLORREF	GetColRef(CString strEntry,	COLORREF crDefault = RGB(128,128,128),	LPCSTR strSection = NULL);
	BOOL		GetBinary(LPCTSTR lpszEntry, BYTE** ppData, UINT* pBytes,			LPCTSTR strSection = NULL);

	void		WriteString(CString strEntry,CString	str,		LPCSTR strSection = NULL);
	void		WriteDouble(CString strEntry,double		f,			LPCSTR strSection = NULL);
	void		WriteFloat(CString strEntry,float		f,			LPCSTR strSection = NULL);
	void		WriteInt(CString strEntry,int			n,			LPCSTR strSection = NULL);
	void		WriteWORD(CString strEntry,WORD		n,			LPCSTR strSection = NULL);
	void		WriteBool(CString strEntry,BOOL			b,			LPCSTR strSection = NULL);
	void		WritePoint(CString strEntry,CPoint		pt,			LPCSTR strSection = NULL);
	void		WriteRect(CString strEntry,CRect		rect,		LPCSTR strSection = NULL);
	void		WriteColRef(CString strEntry,COLORREF	cr,			LPCSTR strSection = NULL);
	BOOL		WriteBinary(LPCTSTR lpszEntry, LPBYTE pData, UINT nBytes, LPCTSTR strSection = NULL);

	void		SerGetString(	BOOL bGet,CString	& str,	CString strEntry,	LPCSTR strSection = NULL,	LPCSTR strDefault=NULL);
	void		SerGetDouble(	BOOL bGet,double	& f,	CString strEntry,	LPCSTR strSection = NULL,	double fDefault = 0.0);
	void		SerGetFloat(	BOOL bGet,float		& f,	CString strEntry,	LPCSTR strSection = NULL,	float fDefault = 0.0);
	void		SerGetInt(		BOOL bGet,int		& n,	CString strEntry,	LPCSTR strSection = NULL,	int nDefault = 0);
	void		SerGetDWORD(	BOOL bGet,DWORD		& n,	CString strEntry,	LPCSTR strSection = NULL,	DWORD nDefault = 0);
	void		SerGetBool(		BOOL bGet,BOOL		& b,	CString strEntry,	LPCSTR strSection = NULL,	BOOL bDefault = FALSE);
	void		SerGetPoint(	BOOL bGet,CPoint	& pt,	CString strEntry,	LPCSTR strSection = NULL,	CPoint ptDefault = CPoint(0,0));
	void		SerGetRect(		BOOL bGet,CRect		& rect,	CString strEntry,	LPCSTR strSection = NULL,	CRect rectDefault = CRect(0,0,0,0));
	void		SerGetColRef(	BOOL bGet,COLORREF	& cr,	CString strEntry,	LPCSTR strSection = NULL,	COLORREF crDefault = RGB(128,128,128));

	void		SerGet(	BOOL bGet,CString	& str,	CString strEntry,	LPCSTR strSection = NULL,	LPCSTR strDefault=NULL);
	void		SerGet(	BOOL bGet,double	& f,	CString strEntry,	LPCSTR strSection = NULL,	double fDefault = 0.0);
	void		SerGet(	BOOL bGet,float		& f,	CString strEntry,	LPCSTR strSection = NULL,	float fDefault = 0.0);
	void		SerGet(	BOOL bGet,int		& n,	CString strEntry,	LPCSTR strSection = NULL,	int nDefault = 0);
	void		SerGet(	BOOL bGet,short		& n,	CString strEntry,	LPCSTR strSection = NULL,	int nDefault = 0);
	void		SerGet(	BOOL bGet,DWORD		& n,	CString strEntry,	LPCSTR strSection = NULL,	DWORD nDefault = 0);
	void		SerGet(	BOOL bGet,WORD		& n,	CString strEntry,	LPCSTR strSection = NULL,	DWORD nDefault = 0);
//	void		SerGet(	BOOL bGet,BOOL		& b,	CString strEntry,	LPCSTR strSection = NULL,	BOOL bDefault = FALSE);
	void		SerGet(	BOOL bGet,CPoint	& pt,	CString strEntry,	LPCSTR strSection = NULL,	CPoint ptDefault = CPoint(0,0));
	void		SerGet(	BOOL bGet,CRect		& rect,	CString strEntry,	LPCSTR strSection = NULL,	CRect rectDefault = CRect(0,0,0,0));
//	void		SerGet(	BOOL bGet,COLORREF	& cr,	CString strEntry,	LPCSTR strSection = NULL,	COLORREF crDefault = RGB(128,128,128));
   
//ARRAYs
	void		SerGet(	BOOL bGet,CString	* str,	int nCount, CString strEntry, LPCSTR strSection = NULL, LPCSTR strDefault=NULL);
	void		SerGet(	BOOL bGet,double	* f,	int nCount, CString strEntry, LPCSTR strSection = NULL, double fDefault = 0.0);
	void		SerGet(	BOOL bGet,float		* f,	int nCount, CString strEntry, LPCSTR strSection = NULL, float fDefault = 0.0);
	void		SerGet(	BOOL bGet,unsigned char	* n,int nCount, CString strEntry, LPCSTR strSection = NULL, unsigned char nDefault = 0);
	void		SerGet(	BOOL bGet,int		* n,	int nCount, CString strEntry, LPCSTR strSection = NULL, int nDefault = 0);
	void		SerGet(	BOOL bGet,short		* n,	int nCount, CString strEntry, LPCSTR strSection = NULL, int nDefault = 0);
	void		SerGet(	BOOL bGet,DWORD		* n,	int nCount, CString strEntry, LPCSTR strSection = NULL, DWORD nDefault = 0);
	void		SerGet(	BOOL bGet,WORD		* n,	int nCount, CString strEntry, LPCSTR strSection = NULL, DWORD nDefault = 0);
	void		SerGet(	BOOL bGet,CPoint	* pt,	int nCount, CString strEntry, LPCSTR strSection = NULL, CPoint ptDefault = CPoint(0,0));
	void		SerGet(	BOOL bGet,CRect		* rect,	int nCount, CString strEntry, LPCSTR strSection = NULL, CRect rectDefault = CRect(0,0,0,0));

	int			Parse(CString &strIn, int nOffset, CString &strOut);
   //MAKRO :
   //SERGET(bGet,value) SerGet(bGet,value,#value)

private:
	char* GetLPCSTR(CString strEntry,LPCSTR strSection,LPCSTR strDefault);
   BOOL  m_bModulPath;  //TRUE: Filenames without path take the Modulepath
                        //FALSE: Filenames without path take the CurrentDirectory

#define MAX_INI_BUFFER 256
	char	m_chBuffer[MAX_INI_BUFFER];
	CString m_strFileName;
	CString m_strSection;
//////////////////////////////////////////////////////////////////////
// statische Methoden
//////////////////////////////////////////////////////////////////////
public:
	static CString	Read( CString const& strFileName, CString const& strSection, CString const& strEntry, CString const& strDefault);
	static void		Write(CString const& strFileName, CString const& strSection, CString const& strEntry, CString const& strValue);
};

#endif // !defined(AFX_INI_H__EEBAF800_182A_11D3_B51F_00104B4A13B4__INCLUDED_)
