// Ini.cpp: Implementierung der Klasse CIni.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Ini2.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// If the IniFilename contains no path,
// the module-directory will be add to the FileName,
// to avoid storing in the windows-directory
/*static*/ void CIni::AddModulPath(CString& strFileName,BOOL bModulPath /*= TRUE*/)
{
   char drive[_MAX_DRIVE];
   char dir[_MAX_DIR];
   char fname[_MAX_FNAME];
   char ext[_MAX_EXT];

   _splitpath( strFileName, drive, dir, fname, ext );
   if( ! drive[0]  )
   {
      //PathCanonicalize(..) doesn't work with for all Plattforms !
      CString strModule;
      if( bModulPath )
      {
         GetModuleFileName(NULL,strModule.GetBuffer(MAX_INI_BUFFER),MAX_INI_BUFFER);
         strModule.ReleaseBuffer();
      }
      else
      {
         GetCurrentDirectory(MAX_INI_BUFFER,strModule.GetBuffer(MAX_INI_BUFFER));
         strModule.ReleaseBuffer();
         // fix by "cpp@world-online.no"
         strModule.TrimRight('\\');
         strModule.TrimRight('/');
         strModule += "\\";
      }
      strModule.ReleaseBuffer();
      _splitpath( strModule, drive, dir, fname, ext );
      strModule = drive;
      strModule+= dir;
      strModule+= strFileName;
      strFileName = strModule;
   }
}
/*static*/ CString CIni::GetDefaultSection()
{
   return AfxGetAppName();
}
/*static*/ CString CIni::GetDefaultIniFile(BOOL bModulPath /*= TRUE*/)
{
   char drive[_MAX_DRIVE];
   char dir[_MAX_DIR];
   char fname[_MAX_FNAME];
   char ext[_MAX_EXT];
   CString strTemp;
   CString strApplName;
   GetModuleFileName(NULL,strTemp.GetBuffer(MAX_INI_BUFFER),MAX_INI_BUFFER);
   strTemp.ReleaseBuffer();
   _splitpath( strTemp, drive, dir, fname, ext );
   strTemp = fname; //"ApplName"
   strTemp += ".ini";  //"ApplName.ini"
   if( bModulPath )
   {
      strApplName  = drive;
      strApplName += dir;
      strApplName += strTemp;
   }
   else
   {
      GetCurrentDirectory(MAX_INI_BUFFER,strApplName.GetBuffer(MAX_INI_BUFFER));
      strApplName.ReleaseBuffer();
      strApplName.TrimRight('\\');
      strApplName.TrimRight('/');
      strApplName += "\\";
      strApplName += strTemp;
   }
   return strApplName;
}
//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////
// Creates/Use file : "Drive:\ApplPath\ApplName.ini"
CIni::CIni(BOOL bModulPath /*= TRUE*/):
   m_bModulPath(bModulPath)
{
   m_strFileName = GetDefaultIniFile(m_bModulPath);
   m_strSection  = GetDefaultSection();
}
CIni::CIni(CIni const& Ini, BOOL bModulPath /*= TRUE*/):
	m_strFileName(Ini.m_strFileName),
	m_strSection(Ini.m_strSection),
   m_bModulPath(Ini.m_bModulPath)
{
   if(m_strFileName.IsEmpty())
      m_strFileName = GetDefaultIniFile(m_bModulPath);
   AddModulPath(m_strFileName,m_bModulPath);
   if(m_strSection.IsEmpty())
      m_strSection = GetDefaultSection();
}
CIni::CIni(CString const& strFileName, BOOL bModulPath /*= TRUE*/):
	m_strFileName(strFileName),
   m_bModulPath(bModulPath)
{
   if(m_strFileName.IsEmpty())
      m_strFileName = GetDefaultIniFile(m_bModulPath);
   AddModulPath(m_strFileName,bModulPath);
   m_strSection = GetDefaultSection();
}
CIni::CIni(CString const& strFileName, CString const& strSection, BOOL bModulPath /*= TRUE*/):
	m_strFileName(strFileName),
	m_strSection(strSection),
   m_bModulPath(bModulPath)
{
   if(m_strFileName.IsEmpty())
      m_strFileName = GetDefaultIniFile(m_bModulPath);
   AddModulPath(m_strFileName,bModulPath);
   if(m_strSection.IsEmpty())
      m_strSection = GetDefaultSection();
}

CIni::~CIni()
{
}
//////////////////////////////////////////////////////////////////////
// Zugriff auf Quelle/Ziel von IO-Operationen
//////////////////////////////////////////////////////////////////////
void CIni::SetFileName(CString const& strFileName)
{
	m_strFileName = strFileName;
   AddModulPath(m_strFileName);
}
void CIni::SetSection(CString const& strSection)
{
	m_strSection = strSection;
}

CString const& CIni::GetFileName() const
{
	return m_strFileName;
}
CString const& CIni::GetSection() const
{
	return m_strSection;
}
//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////


void CIni::Init( LPCSTR strFileName, LPCSTR strSection/* = NULL*/)
{
	if(strSection != NULL)
		m_strSection = strSection;
	if(strFileName != NULL)
		m_strFileName = strFileName;
}
CString CIni::GetString(CString strEntry,LPCSTR strDefault/*=NULL*/,LPCSTR strSection/* = NULL*/)
{
	if(strDefault == NULL)
		return CString(GetLPCSTR(strEntry,strSection,""));
	else
		return CString(GetLPCSTR(strEntry,strSection,strDefault));
}
double CIni::GetDouble(CString strEntry,double fDefault/* = 0.0*/,LPCSTR strSection/* = NULL*/)
{
	CString strDefault;
	strDefault.Format("%g",fDefault);
	GetLPCSTR(strEntry,strSection,strDefault);
	return atof(m_chBuffer);
}
float CIni::GetFloat(CString strEntry,float fDefault/* = 0.0*/, LPCSTR strSection/* = NULL*/)
{
	CString strDefault;
	strDefault.Format("%g",fDefault);
	GetLPCSTR(strEntry,strSection,strDefault);
	return (float)atof(m_chBuffer);
}
int CIni::GetInt(CString strEntry,int nDefault/* = 0*/,LPCSTR strSection/* = NULL*/)
{
	CString strDefault;
	strDefault.Format("%d",nDefault);
	GetLPCSTR(strEntry,strSection,strDefault);
	return atoi(m_chBuffer);
}
WORD CIni::GetWORD(CString strEntry,WORD nDefault/* = 0*/,LPCSTR strSection/* = NULL*/)
{
	CString strDefault;
	strDefault.Format("%u",nDefault);
	GetLPCSTR(strEntry,strSection,strDefault);
	return (WORD)atoi(m_chBuffer);
}
BOOL CIni::GetBool(CString strEntry,BOOL bDefault/* = FALSE*/,LPCSTR strSection/* = NULL*/)
{
	CString strDefault;
	strDefault.Format("%d",bDefault);
	GetLPCSTR(strEntry,strSection,strDefault);
	return ( atoi(m_chBuffer) != 0 );
}
CPoint CIni::GetPoint(CString strEntry,	CPoint ptDefault, LPCSTR strSection)
{
	CPoint ptReturn=ptDefault;

	CString strDefault;
	strDefault.Format("(%d,%d)",ptDefault.x, ptDefault.y);

	CString strPoint = GetString(strEntry,strDefault);
	sscanf(strPoint,"(%d,%d)", &ptReturn.x, &ptReturn.y);

	return ptReturn;
}
CRect CIni::GetRect(CString strEntry, CRect rectDefault, LPCSTR strSection)
{
	CRect rectReturn=rectDefault;

	CString strDefault;
	//old version :strDefault.Format("(%d,%d,%d,%d)",rectDefault.top,rectDefault.left,rectDefault.bottom,rectDefault.right);
	strDefault.Format("%d,%d,%d,%d",rectDefault.left,rectDefault.top,rectDefault.right,rectDefault.bottom);

	CString strRect = GetString(strEntry,strDefault);

   //new Version found
   if( 4==sscanf(strRect,"%d,%d,%d,%d",&rectDefault.left,&rectDefault.top,&rectDefault.right,&rectDefault.bottom))
	   return rectReturn;
   //old Version found
   sscanf(strRect,"(%d,%d,%d,%d)", &rectReturn.top,&rectReturn.left,&rectReturn.bottom,&rectReturn.right);
	return rectReturn;
}
COLORREF CIni::GetColRef(CString strEntry, COLORREF crDefault, LPCSTR strSection)
{
	int temp[3]={	GetRValue(crDefault),
					GetGValue(crDefault),
					GetBValue(crDefault)};

	CString strDefault;
	strDefault.Format("RGB(%hd,%hd,%hd)",temp[0],temp[1],temp[2]);

	CString strColRef = GetString(strEntry,strDefault);
	sscanf(strColRef,"RGB(%d,%d,%d)", temp, temp+1, temp+2);

	return RGB(temp[0],temp[1],temp[2]);
}
	
void CIni::WriteString(CString strEntry,CString	str, LPCSTR strSection/* = NULL*/)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	WritePrivateProfileString(m_strSection,strEntry,str,m_strFileName);
}
void CIni::WriteDouble(CString strEntry,double f, LPCSTR strSection/*= NULL*/)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	CString strBuffer;
	strBuffer.Format("%g",f);
		WritePrivateProfileString(m_strSection,strEntry,strBuffer,m_strFileName);
}
void CIni::WriteFloat(CString strEntry,float f, LPCSTR strSection/* = NULL*/)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	CString strBuffer;
	strBuffer.Format("%g",f);
		WritePrivateProfileString(m_strSection,strEntry,strBuffer,m_strFileName);
}
void CIni::WriteInt(CString strEntry,int n, LPCSTR strSection/* = NULL*/)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	CString strBuffer;
	strBuffer.Format("%d",n);
		WritePrivateProfileString(m_strSection,strEntry,strBuffer,m_strFileName);
}
void CIni::WriteWORD(CString strEntry,WORD n, LPCSTR strSection/* = NULL*/)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	CString strBuffer;
	strBuffer.Format("%u",n);
		WritePrivateProfileString(m_strSection,strEntry,strBuffer,m_strFileName);
}
void CIni::WriteBool(CString strEntry,BOOL b, LPCSTR strSection/* = NULL*/)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	CString strBuffer;
	strBuffer.Format("%d",b);
		WritePrivateProfileString(m_strSection, strEntry, strBuffer, m_strFileName);
}
void CIni::WritePoint(CString strEntry,CPoint pt, LPCSTR strSection)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	CString strBuffer;
	strBuffer.Format("(%d,%d)",pt.x,pt.y);
	Write(m_strFileName,m_strSection,strEntry,strBuffer);
}
void CIni::WriteRect(CString strEntry,CRect rect, LPCSTR strSection)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	CString strBuffer;
	strBuffer.Format("(%d,%d,%d,%d)",rect.top,rect.left,rect.bottom,rect.right);
	Write(m_strFileName,m_strSection,strEntry,strBuffer);
}
void CIni::WriteColRef(CString strEntry,COLORREF cr, LPCSTR strSection)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	CString strBuffer;
	strBuffer.Format("RGB(%d,%d,%d)",GetRValue(cr), GetGValue(cr), GetBValue(cr));
	Write(m_strFileName,m_strSection,strEntry,strBuffer);
}
char* CIni::GetLPCSTR(CString strEntry, LPCSTR strSection, LPCSTR strDefault)
{
	// evtl Section neu setzen
	if(strSection != NULL)
		m_strSection = strSection;

	CString temp;
	if(strDefault == NULL)
		temp = Read(m_strFileName,m_strSection,strEntry,CString());
	else
		temp = Read(m_strFileName,m_strSection,strEntry,strDefault);
	return (char*)memcpy(m_chBuffer,(LPCTSTR)temp,temp.GetLength()+1);// '+1' damit die Null am Ende mit kopiert wird
}
void CIni::SerGetString(	BOOL bGet,CString &	str,CString strEntry,LPCSTR strSection,LPCSTR strDefault)
{
	if(bGet)
		str = GetString(strEntry,strDefault/*=NULL*/,strSection/* = NULL*/);
	else
		WriteString(strEntry,str, strSection/* = NULL*/);
}
void CIni::SerGetDouble(	BOOL bGet,double&	f,	CString strEntry,LPCSTR strSection/* = NULL*/,double fDefault/* = 0.0*/)
{
	if(bGet)
		f = GetDouble(strEntry,fDefault/*=NULL*/,strSection/* = NULL*/);
	else
		WriteDouble(strEntry,f, strSection/* = NULL*/);
}
void CIni::SerGetFloat(		BOOL bGet,float	&	f,	CString strEntry, LPCSTR strSection/* = NULL*/,float fDefault/* = 0.0*/)
{
	if(bGet)
		f = GetFloat(strEntry,fDefault/*=NULL*/,strSection/* = NULL*/);
	else
		WriteFloat(strEntry,f, strSection/* = NULL*/);
}
void CIni::SerGetInt(		BOOL bGet,int	&	n,	CString strEntry,LPCSTR strSection/* = NULL*/,int nDefault/* = 0*/)
{
	if(bGet)
		n = GetInt(strEntry,nDefault/*=NULL*/,strSection/* = NULL*/);
	else
		WriteInt(strEntry,n, strSection/* = NULL*/);
}
void CIni::SerGetDWORD(		BOOL bGet,DWORD	&	n,	CString strEntry,LPCSTR strSection/* = NULL*/,DWORD nDefault/* = 0*/)
{
	if(bGet)
		n = (DWORD)GetInt(strEntry,nDefault/*=NULL*/,strSection/* = NULL*/);
	else
		WriteInt(strEntry,n, strSection/* = NULL*/);
}
void CIni::SerGetBool(		BOOL bGet,BOOL	&	b,	CString strEntry,LPCSTR strSection/* = NULL*/,BOOL bDefault/* = FALSE*/)
{
	if(bGet)
		b = GetBool(strEntry,bDefault/*=NULL*/,strSection/* = NULL*/);
	else
		WriteBool(strEntry,b, strSection/* = NULL*/);
}

void CIni::SerGetPoint(	BOOL bGet,CPoint	& pt,	CString strEntry,	LPCSTR strSection,	CPoint ptDefault)
{
	if(bGet)
		pt = GetPoint(strEntry,ptDefault,strSection);
	else
		WritePoint(strEntry,pt, strSection);
}
void CIni::SerGetRect(		BOOL bGet,CRect		& rect,	CString strEntry,	LPCSTR strSection,	CRect rectDefault)
{
	if(bGet)
		rect = GetRect(strEntry,rectDefault,strSection);
	else
		WriteRect(strEntry,rect, strSection);
}
void CIni::SerGetColRef(	BOOL bGet,COLORREF	& cr,	CString strEntry,	LPCSTR strSection,	COLORREF crDefault)
{
	if(bGet)
		cr = GetColRef(strEntry,crDefault,strSection);
	else
		WriteColRef(strEntry,cr, strSection);
}
// Überladene Methoden //////////////////////////////////////////////////////////////////////////////////////////////////77
// Einfache Typen /////////////////////////////////////////////////////////////////////////////////////////////////////////
void		CIni::SerGet(	BOOL bGet,CString	& str,	CString strEntry,	LPCSTR strSection/*= NULL*/,	LPCSTR strDefault/*= NULL*/)
{
   SerGetString(bGet,str,strEntry,strSection,strDefault);
}
void		CIni::SerGet(	BOOL bGet,double	& f,	CString strEntry,	LPCSTR strSection/*= NULL*/,	double fDefault/* = 0.0*/)
{
   SerGetDouble(bGet,f,strEntry,strSection,fDefault);
}
void		CIni::SerGet(	BOOL bGet,float		& f,	CString strEntry,	LPCSTR strSection/*= NULL*/,	float fDefault/* = 0.0*/)
{
   SerGetFloat(bGet,f,strEntry,strSection,fDefault);
}
void		CIni::SerGet(	BOOL bGet,int		& n,	CString strEntry,	LPCSTR strSection/*= NULL*/,	int nDefault/* = 0*/)
{
   SerGetInt(bGet,n,strEntry,strSection,nDefault);
}
void		CIni::SerGet(	BOOL bGet,short		& n,	CString strEntry,	LPCSTR strSection/*= NULL*/,	int nDefault/* = 0*/)
{
   int nTemp = n;
   SerGetInt(bGet,nTemp,strEntry,strSection,nDefault);
   n = nTemp;
}
void		CIni::SerGet(	BOOL bGet,DWORD		& n,	CString strEntry,	LPCSTR strSection/*= NULL*/,	DWORD nDefault/* = 0*/)
{
   SerGetDWORD(bGet,n,strEntry,strSection,nDefault);
}
void		CIni::SerGet(	BOOL bGet,WORD		& n,	CString strEntry,	LPCSTR strSection/*= NULL*/,	DWORD nDefault/* = 0*/)
{
   DWORD dwTemp = n;
   SerGetDWORD(bGet,dwTemp,strEntry,strSection,nDefault);
   n = dwTemp;
}
//	void		SerGet(	BOOL bGet,BOOL		& b,	CString strEntry,	LPCSTR strSection = NULL,	BOOL bDefault = FALSE);
void		CIni::SerGet(	BOOL bGet,CPoint	& pt,	CString strEntry,	LPCSTR strSection/*= NULL*/,	CPoint ptDefault/* = CPoint(0,0)*/)
{
   SerGetPoint(bGet,pt,strEntry,strSection,ptDefault);
}
void		CIni::SerGet(	BOOL bGet,CRect		& rect,	CString strEntry,	LPCSTR strSection/*= NULL*/,	CRect rectDefault/* = CRect(0,0,0,0)*/)
{
   SerGetRect(bGet,rect,strEntry,strSection,rectDefault);
}
//	void		SerGet(	BOOL bGet,COLORREF	& cr,	CString strEntry,	LPCSTR strSection = NULL,	COLORREF crDefault = RGB(128,128,128));

// Überladene Methoden ////////////////////////////////////////////////////////////////////////////////////////////
// ARRAYS /////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entries werden durch Unterstrich + Index ergenzt////////////////////////////////////////////////////////////////
void CIni::SerGet(BOOL bGet, CString *ar, int nCount, CString strEntry, LPCSTR strSection/*=NULL*/, LPCSTR Default/*=NULL*/)
{
	if(nCount > 0) {
		CString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, ar[i]);
				if(ar[i].GetLength() == 0)
					ar[i] = Default;
			}

		} else {
			strBuffer = ar[0];
			for(int i = 1; i < nCount; i++) {
				strBuffer.AppendChar(',');
				strBuffer.Append(ar[i]);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}

void CIni::SerGet(BOOL bGet, double *ar, int nCount, CString strEntry, LPCSTR strSection/*=NULL*/, double Default/* = 0.0*/)
{
	if(nCount > 0) {
		CString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			CString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.GetLength() == 0)
					ar[i] = Default;
				else
					ar[i] = atof(strTemp);
			}

		} else {
			CString strTemp;
			strBuffer.Format("%g", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp.Format("%g", ar[i]);
				strBuffer.AppendChar(',');
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void CIni::SerGet(BOOL bGet, float *ar, int nCount, CString strEntry, LPCSTR strSection/*=NULL*/, float Default/* = 0.0*/)
{
	if(nCount > 0) {
		CString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			CString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.GetLength() == 0)
					ar[i] = Default;
				else
					ar[i] = (float)atof(strTemp);
			}

		} else {
			CString strTemp;
			strBuffer.Format("%g", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp.Format("%g", ar[i]);
				strBuffer.AppendChar(',');
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void CIni::SerGet(BOOL bGet, int *ar, int nCount, CString strEntry, LPCSTR strSection/*=NULL*/, int Default/* = 0*/)
{
	if(nCount > 0) {
		CString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			CString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.GetLength() == 0)
					ar[i] = Default;
				else
					ar[i] = atoi(strTemp);
			}

		} else {
			CString strTemp;
			strBuffer.Format("%d", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp.Format("%d", ar[i]);
				strBuffer.AppendChar(',');
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void CIni::SerGet(BOOL bGet, unsigned char *ar, int nCount, CString strEntry, LPCSTR strSection/*=NULL*/, unsigned char Default/* = 0*/)
{
	if(nCount > 0) {
		CString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			CString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.GetLength() == 0)
					ar[i] = Default;
				else
					ar[i] = (unsigned char)atoi(strTemp);
			}

		} else {
			CString strTemp;
			strBuffer.Format("%d", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp.Format("%d", ar[i]);
				strBuffer.AppendChar(',');
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void CIni::SerGet(BOOL bGet, short *ar, int nCount, CString strEntry, LPCSTR strSection/*=NULL*/, int Default/* = 0*/)
{
	if(nCount > 0) {
		CString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			CString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.GetLength() == 0)
					ar[i] = Default;
				else
					ar[i] = (short)atoi(strTemp);
			}

		} else {
			CString strTemp;
			strBuffer.Format("%d", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp.Format("%d", ar[i]);
				strBuffer.AppendChar(',');
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void CIni::SerGet(BOOL bGet, DWORD *ar, int nCount, CString strEntry, LPCSTR strSection/*=NULL*/, DWORD Default/* = 0*/)
{
	if(nCount > 0) {
		CString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			CString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.GetLength() == 0)
					ar[i] = Default;
				else
					ar[i] = (DWORD)atoi(strTemp);
			}

		} else {
			CString strTemp;
			strBuffer.Format("%d", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp.Format("%d", ar[i]);
				strBuffer.AppendChar(',');
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void CIni::SerGet(BOOL bGet, WORD *ar, int nCount, CString strEntry, LPCSTR strSection/*=NULL*/, DWORD Default/* = 0*/)
{
	if(nCount > 0) {
		CString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			CString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.GetLength() == 0)
					ar[i] = Default;
				else
					ar[i] = (WORD)atoi(strTemp);
			}

		} else {
			CString strTemp;
			strBuffer.Format("%d", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp.Format("%d", ar[i]);
				strBuffer.AppendChar(',');
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void		CIni::SerGet(	BOOL bGet,CPoint	* ar,	   int nCount, CString strEntry,	LPCSTR strSection/*=NULL*/,	CPoint Default/* = CPoint(0,0)*/)
{
   CString strBuffer;
   for( int i=0 ; i<nCount ; i++)
   {
      strBuffer.Format("_%i",i);
      strBuffer = strEntry + strBuffer;
      SerGet(bGet,ar[i],strBuffer,strSection,Default);
   }
}
void		CIni::SerGet(	BOOL bGet,CRect	* ar,	   int nCount, CString strEntry,	LPCSTR strSection/*=NULL*/,	CRect Default/* = CRect(0,0,0,0)*/)
{
   CString strBuffer;
   for( int i=0 ; i<nCount ; i++)
   {
      strBuffer.Format("_%i",i);
      strBuffer = strEntry + strBuffer;
      SerGet(bGet,ar[i],strBuffer,strSection,Default);
   }
}

int			CIni::Parse(CString &strIn, int nOffset, CString &strOut) {

	strOut.Empty();
	int nLength = strIn.GetLength();

	if(nOffset < nLength) {
		if(nOffset != 0 && strIn[nOffset] == ',')
			nOffset++;

		while(nOffset < nLength) {
			if(!isspace(strIn[nOffset]))
				break;

			nOffset++;
		}

		while(nOffset < nLength) {
			strOut += strIn[nOffset];

			if(strIn[++nOffset] == ',')
				break;
		}

		strOut.Trim();
	}
	return nOffset;
}

//////////////////////////////////////////////////////////////////////
// statische Methoden
//////////////////////////////////////////////////////////////////////
CString CIni::Read(CString const& strFileName, CString const& strSection, CString const& strEntry, CString const& strDefault)
{
	CString strReturn;
	GetPrivateProfileString(strSection,
							strEntry,
							strDefault,
							strReturn.GetBufferSetLength(MAX_INI_BUFFER),
							MAX_INI_BUFFER,
							strFileName);
	strReturn.ReleaseBuffer();
	return strReturn;
}
void CIni::Write(CString const& strFileName, CString const& strSection, CString const& strEntry, CString const& strValue)
{
	WritePrivateProfileString(strSection,
							strEntry,
							strValue,
							strFileName);
}

BOOL CIni::GetBinary(LPCTSTR lpszEntry, BYTE** ppData, UINT* pBytes, LPCTSTR pszSection)
{
	*ppData = NULL;
	*pBytes = 0;

	CString str = GetString(lpszEntry, NULL, pszSection);
	if (str.IsEmpty())
		return FALSE;
	ASSERT(str.GetLength()%2 == 0);
	INT_PTR nLen = str.GetLength();
	*pBytes = UINT(nLen)/2;
	*ppData = new BYTE[*pBytes];
	for (int i=0;i<nLen;i+=2)
	{
		(*ppData)[i/2] = (BYTE)(((str[i+1] - 'A') << 4) + (str[i] - 'A'));
	}
	return TRUE;
}

BOOL CIni::WriteBinary(LPCTSTR lpszEntry, LPBYTE pData, UINT nBytes, LPCTSTR pszSection)
{
	// convert to string and write out
	LPTSTR lpsz = new TCHAR[nBytes*2+1];
	UINT i;
	for (i = 0; i < nBytes; i++)
	{
		lpsz[i*2] = (TCHAR)((pData[i] & 0x0F) + 'A'); //low nibble
		lpsz[i*2+1] = (TCHAR)(((pData[i] >> 4) & 0x0F) + 'A'); //high nibble
	}
	lpsz[i*2] = 0;


	WriteString(lpszEntry, lpsz, pszSection);
	delete[] lpsz;
	return TRUE;
}
