#include "stdafx.h"
#include <locale.h>
#include "emule.h"
#include "OtherFunctions.h"
#include "Preferences.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define LANGID_AR_AE MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_UAE)
#define LANGID_BG_BG MAKELANGID(LANG_BULGARIAN ,SUBLANG_DEFAULT)
#define LANGID_CA_ES MAKELANGID(LANG_CATALAN ,SUBLANG_DEFAULT)
#define LANGID_CZ_CZ MAKELANGID(LANG_CZECH,SUBLANG_DEFAULT)
#define LANGID_DA_DK MAKELANGID(LANG_DANISH,SUBLANG_DEFAULT)
#define LANGID_DE_DE MAKELANGID(LANG_GERMAN,SUBLANG_DEFAULT)
#define LANGID_EL_GR MAKELANGID(LANG_GREEK,SUBLANG_DEFAULT)
#define LANGID_EN_US MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT)
#define LANGID_ES_ES_T MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH)
#define LANGID_ET_EE MAKELANGID(LANG_ESTONIAN,SUBLANG_DEFAULT)
#define LANGID_FI_FI MAKELANGID(LANG_FINNISH,SUBLANG_DEFAULT)
#define LANGID_FR_FR MAKELANGID(LANG_FRENCH,SUBLANG_DEFAULT)
#define LANGID_GL_ES MAKELANGID(LANG_GALICIAN,SUBLANG_DEFAULT)
#define LANGID_HE_IL MAKELANGID(LANG_HEBREW,SUBLANG_DEFAULT)
#define LANGID_HU_HU MAKELANGID(LANG_HUNGARIAN,SUBLANG_DEFAULT)
#define LANGID_IT_IT MAKELANGID(LANG_ITALIAN,SUBLANG_DEFAULT)
#define LANGID_JP_JP MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT)
#define LANGID_KO_KR MAKELANGID(LANG_KOREAN,SUBLANG_DEFAULT)
#define LANGID_LT_LT MAKELANGID(LANG_LITHUANIAN,SUBLANG_DEFAULT)
#define LANGID_LV_LV MAKELANGID(LANG_LATVIAN,SUBLANG_DEFAULT)
#define LANGID_NB_NO MAKELANGID(LANG_NORWEGIAN,SUBLANG_NORWEGIAN_BOKMAL)
#define LANGID_NL_NL MAKELANGID(LANG_DUTCH,SUBLANG_DEFAULT)
#define LANGID_PL_PL MAKELANGID(LANG_POLISH,SUBLANG_DEFAULT)
#define LANGID_PT_BR MAKELANGID(LANG_PORTUGUESE,SUBLANG_PORTUGUESE_BRAZILIAN)
#define LANGID_PT_PT MAKELANGID(LANG_PORTUGUESE,SUBLANG_PORTUGUESE)
#define LANGID_RO_RO MAKELANGID(LANG_ROMANIAN,SUBLANG_DEFAULT)
#define LANGID_RU_RU MAKELANGID(LANG_RUSSIAN,SUBLANG_DEFAULT)
#define LANGID_SL_SI MAKELANGID(LANG_SLOVENIAN,SUBLANG_DEFAULT)
#define LANGID_SV_SE MAKELANGID(LANG_SWEDISH,SUBLANG_DEFAULT)
#define LANGID_TR_TR MAKELANGID(LANG_TURKISH,SUBLANG_DEFAULT)
#define LANGID_ZH_CN MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED)
#define LANGID_ZH_TW MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL)


static HINSTANCE _hLangDLL = NULL;

CString GetResString(UINT uStringID, WORD wLanguageID)
{
	
	CString resString;
	if (_hLangDLL)
		resString.LoadString(_hLangDLL, uStringID, wLanguageID);
	if (resString.IsEmpty())
		resString.LoadString(GetModuleHandle(NULL), uStringID, LANGID_EN_US);
	return resString;
}

CString GetResString(UINT uStringID)
{
	CString resString;
	if (_hLangDLL)
		resString.LoadString(_hLangDLL, uStringID);
	if (resString.IsEmpty())
		resString.LoadString(GetModuleHandle(NULL), uStringID);
	return resString;
}

struct SLanguage {
	LANGID	lid;
	LPCTSTR pszLocale;
	BOOL	bSupported;
	LPCTSTR	pszISOLocale;
	UINT	uCodepage;
	LPCTSTR	pszHtmlCharset;
};


// Codepages (Windows)
// ---------------------------------------------------------------------
// 1250		ANSI - Central European
// 1251		ANSI - Cyrillic
// 1252		ANSI - Latin I
// 1253		ANSI - Greek
// 1254		ANSI - Turkish
// 1255		ANSI - Hebrew
// 1256		ANSI - Arabic
// 1257		ANSI - Baltic
// 1258		ANSI/OEM - Vietnamese
//  932		ANSI/OEM - Japanese, Shift-JIS
//  936		ANSI/OEM - Simplified Chinese (PRC, Singapore)
//  949		ANSI/OEM - Korean (Unified Hangeul Code)
//  950		ANSI/OEM - Traditional Chinese (Taiwan; Hong Kong SAR, PRC)

// HTML charsets	CodePg
// -------------------------------------------
// windows-1250		1250	Central European (Windows)
// windows-1251		1251	Cyrillic (Windows)
// windows-1252		1252	Western European (Windows)
// windows-1253		1253	Greek (Windows)
// windows-1254		1254	Turkish (Windows)
// windows-1255		1255	Hebrew (Windows)
// windows-1256		1256	Arabic (Windows)
// windows-1257		1257	Baltic (Windows)
//
// NOTE: the 'iso-...' charsets are more backward compatible than the 'windows-...' charsets.
// NOTE-ALSO: some of the 'iso-...' charsets are by default *not* installed by IE6 (e.g. Arabic (ISO)) or show up
//	with wrong chars - so, better use the 'windows-' charsets..
//
// iso-8859-1		1252	Western European (ISO)
// iso-8859-2		1250	Central European (ISO)
// iso-8859-3		1254	Latin 3 (ISO)
// iso-8859-4		1257	Baltic (ISO)
// iso-8859-5		1251	Cyrillic (ISO)			does not show up correctly in IE6
// iso-8859-6		1256	Arabic (ISO)			not installed (by default) with IE6
// iso-8859-7		1253	Greek (ISO)
// iso-8859-8		1255	Hebrew (ISO-Visual)
// iso-8859-9		1254	Turkish (ISO)
// iso-8859-15		1252	Latin 9 (ISO)
// iso-2022-jp		 932	Japanese (JIS)
// iso-2022-kr		 949	Korean (ISO)

static SLanguage _aLanguages[] =
{
	{LANGID_AR_AE,	_T(""),				FALSE,	_T("ar_AE"),	1256,	_T("windows-1256")},	// Arabic (UAE)
	{LANGID_BG_BG,	_T("hun"),			FALSE,	_T("bg_BG"),	1251,	_T("windows-1251")},	// Bulgarian
	{LANGID_CA_ES,	_T(""),				FALSE,	_T("ca_ES"),	1252,	_T("windows-1252")},	// Catalan
	{LANGID_CZ_CZ,	_T("czech"),		FALSE,	_T("cz_CZ"),	1250,	_T("windows-1250")},	// Czech
	{LANGID_DA_DK,	_T("danish"),		FALSE,	_T("da_DK"),	1252,	_T("windows-1252")},	// Danish
	{LANGID_DE_DE,	_T("german"),		FALSE,	_T("de_DE"),	1252,	_T("windows-1252")},	// German (Germany)
	{LANGID_EL_GR,	_T("greek"),		FALSE,	_T("el_GR"),	1253,	_T("windows-1253")},	// Greek
	{LANGID_EN_US,	_T("english"),		TRUE,	_T("en_US"),	1252,	_T("windows-1252")},	// English
	{LANGID_ES_ES_T,_T("spanish"),		FALSE,	_T("es_ES_T"),	1252,	_T("windows-1252")},	// Spanish (Castilian)
	{LANGID_ET_EE,	_T(""),				FALSE,	_T("et_EE"),	1257,	_T("windows-1257")},	// Estonian
	{LANGID_FI_FI,	_T("finnish"),		FALSE,	_T("fi_FI"),	1252,	_T("windows-1252")},	// Finnish
	{LANGID_FR_FR,	_T("french"),		FALSE,	_T("fr_FR"),	1252,	_T("windows-1252")},	// French (France)
	{LANGID_GL_ES,	_T(""),				FALSE,	_T("gl_ES"),	1252,	_T("windows-1252")},	// Galician
	{LANGID_HE_IL,	_T(""),				FALSE,	_T("he_IL"),	1255,	_T("windows-1255")},	// Hebrew
	{LANGID_HU_HU,	_T("hungarian"),	FALSE,	_T("hu_HU"),	1250,	_T("windows-1250")},	// Hungarian
	{LANGID_IT_IT,	_T("italian"),		FALSE,	_T("it_IT"),	1252,	_T("windows-1252")},	// Italian (Italy)
	{LANGID_JP_JP,	_T("japanese"),		FALSE,	_T("jp_JP"),	 932,	_T("shift_jis")},		// Japanese
	{LANGID_KO_KR,	_T("korean"),		FALSE,	_T("ko_KR"),	 949,	_T("euc-kr")},			// Korean
	{LANGID_LT_LT,	_T(""),				FALSE,	_T("lt_LT"),	1257,	_T("windows-1257")},	// Lithuanian
	{LANGID_LV_LV,	_T(""),				FALSE,	_T("lv_LV"),	1257,	_T("windows-1257")},	// Latvian
	{LANGID_NB_NO,	_T("norwegian"),	FALSE,	_T("nb_NO"),	1252,	_T("windows-1252")},	// Norwegian (Bokmal)
	{LANGID_NL_NL,	_T("dutch"),		FALSE,	_T("nl_NL"),	1252,	_T("windows-1252")},	// Dutch (Netherlands)
	{LANGID_PL_PL,	_T("polish"),		FALSE,	_T("pl_PL"),	1250,	_T("windows-1250")},	// Polish
	{LANGID_PT_BR,	_T("ptb"),			FALSE,	_T("pt_BR"),	1252,	_T("windows-1252")},	// Portuguese (Brazil)
	{LANGID_PT_PT,	_T("ptg"),			FALSE,	_T("pt_PT"),	1252,	_T("windows-1252")},	// Portuguese (Portugal)
	{LANGID_RO_RO,	_T(""),				FALSE,	_T("ro_RO"),	1250,	_T("windows-1250")},	// Rumänisch
	{LANGID_RU_RU,	_T("russian"),		FALSE,	_T("ru_RU"),	1251,	_T("windows-1251")},	// Russian
	{LANGID_SL_SI,	_T(""),				FALSE,	_T("sl_SI"),	1250,	_T("windows-1250")},	// Slovenian
	{LANGID_SV_SE,	_T("swedish"),		FALSE,	_T("sv_SE"),	1252,	_T("windows-1252")},	// Swedish
	{LANGID_TR_TR,	_T("turkish"),		FALSE,	_T("tr_TR"),	1254,	_T("windows-1254")},	// Turkish
	{LANGID_ZH_CN,	_T("chs"),			FALSE,	_T("zh_CN"),	 936,	_T("gb2312")},			// Chinese (P.R.C.)
	{LANGID_ZH_TW,	_T("cht"),			FALSE,	_T("zh_TW"),	 950,	_T("big5")},			// Chinese (Taiwan)
	{0, NULL, 0, 0}
};

static void InitLanguages(const CString& rstrLangDir, bool bReInit = false)
{
	static BOOL _bInitialized = FALSE;
	if (_bInitialized && !bReInit)
		return;
	_bInitialized = TRUE;

	CFileFind ff;
	bool bEnd = !ff.FindFile(rstrLangDir + _T("*.dll"), 0);
	while (!bEnd)
	{
		bEnd = !ff.FindNextFile();
		if (ff.IsDirectory())
			continue;
		TCHAR szLandDLLFileName[MAX_PATH];
		_tsplitpath(ff.GetFileName(), NULL, NULL, szLandDLLFileName, NULL);

		SLanguage* pLangs = _aLanguages;
		if (pLangs){
			while (pLangs->lid){
				if (_tcsicmp(pLangs->pszISOLocale, szLandDLLFileName) == 0){
					pLangs->bSupported = TRUE;
					break;
				}
				pLangs++;
			}
		}
	}
	ff.Close();
}

static void FreeLangDLL()
{
	if (_hLangDLL != NULL && _hLangDLL != GetModuleHandle(NULL)){
		VERIFY( FreeLibrary(_hLangDLL) );
		_hLangDLL = NULL;
	}
}

void CPreferences::GetLanguages(CWordArray& aLanguageIDs)
{
	const SLanguage* pLang = _aLanguages;
	while (pLang->lid){
		//if (pLang->bSupported)
		//show all languages, offer download if not supported ones later
		aLanguageIDs.Add(pLang->lid);
		pLang++;
	}
}

WORD CPreferences::GetLanguageID()
{
	return m_wLanguageID;
}

void CPreferences::SetLanguageID(WORD lid)
{
	m_wLanguageID = lid;
}

static bool CheckLangDLLVersion(const CString& rstrLangDLL)
{
	bool bResult = false;
	DWORD dwUnused;
	DWORD dwVerInfSize = GetFileVersionInfoSize(const_cast<LPTSTR>((LPCTSTR)rstrLangDLL), &dwUnused);
	if (dwVerInfSize != 0)
	{
		LPBYTE pucVerInf = (LPBYTE)calloc(dwVerInfSize, 1);
		if (pucVerInf)
		{
			if (GetFileVersionInfo(const_cast<LPTSTR>((LPCTSTR)rstrLangDLL), 0, dwVerInfSize, pucVerInf))
			{
				VS_FIXEDFILEINFO* pFileInf = NULL;
				UINT uLen = 0;
				if (VerQueryValue(pucVerInf, _T("\\"), (LPVOID*)&pFileInf, &uLen) && pFileInf && uLen)
				{
					bResult = (pFileInf->dwProductVersionMS == theApp.m_dwProductVersionMS &&
                               pFileInf->dwProductVersionLS == theApp.m_dwProductVersionLS);
				}
			}
			free(pucVerInf);
		}
	}

	// no messagebox anymore since we just offer the user to download the new one
	/*if (!bResult){
		CString strError;
		// Don't try to load a localized version of that string! ;)
		strError.Format(_T("Language DLL \"%s\" is not for this eMule version. Please update the language DLL!"), rstrLangDLL);
		AfxMessageBox(strError, MB_ICONSTOP);
	}*/

	return bResult;
}

static bool LoadLangLib(const CString& rstrLangDir, LANGID lid)
{
	const SLanguage* pLangs = _aLanguages;
	if (pLangs){
		while (pLangs->lid){
			if (pLangs->bSupported && pLangs->lid == lid){
				FreeLangDLL();

				bool bLoadedLib = false;
				if (pLangs->lid == LANGID_EN_US){
					_hLangDLL = NULL;
					bLoadedLib = true;
				}
				else{
					CString strLangDLL = rstrLangDir;
					strLangDLL += pLangs->pszISOLocale;
					strLangDLL += _T(".dll");
					if (CheckLangDLLVersion(strLangDLL)){
						_hLangDLL = LoadLibrary(strLangDLL);
						if (_hLangDLL)
							bLoadedLib = true;
					}
				}
				if (bLoadedLib)
					return true;
				break;
			}
			pLangs++;
		}
	}
	return false;
}

void CPreferences::SetLanguage()
{
	InitLanguages(GetLangDir());

	bool bFoundLang = false;
	if (m_wLanguageID)
		bFoundLang = LoadLangLib(GetLangDir(), m_wLanguageID);

	if (!bFoundLang){
		LANGID lidLocale = (LANGID)::GetThreadLocale();
		//LANGID lidLocalePri = PRIMARYLANGID(::GetThreadLocale());
		//LANGID lidLocaleSub = SUBLANGID(::GetThreadLocale());

		bFoundLang = LoadLangLib(GetLangDir(), lidLocale);
		if (!bFoundLang){
			LoadLangLib(GetLangDir(), LANGID_EN_US);
			m_wLanguageID = LANGID_EN_US;
			CString strLngEnglish = GetResString(IDS_MB_LANGUAGEINFO);
			AfxMessageBox(strLngEnglish, MB_ICONASTERISK);
		}
		else
			m_wLanguageID = lidLocale;
	}

	// if loading a string fails, set language to English
	if (GetResString(IDS_MB_LANGUAGEINFO).IsEmpty()) {
		LoadLangLib(GetLangDir(), LANGID_EN_US);
		m_wLanguageID = LANGID_EN_US;
	}

	InitThreadLocale();
}

bool CPreferences::IsLanguageSupported(LANGID lidSelected, bool bUpdateBefore){
	InitLanguages(GetLangDir(), bUpdateBefore);
	if (lidSelected == LANGID_EN_US)
		return true;
	const SLanguage* pLang = _aLanguages;
	for (;pLang->lid;pLang++){
		if (pLang->lid == lidSelected && pLang->bSupported){
			return CheckLangDLLVersion(GetLangDir()+CString(pLang->pszISOLocale) + _T(".dll"));
		}
	}
	return false; 
}

CString CPreferences::GetLangDLLNameByID(LANGID lidSelected){
	const SLanguage* pLang = _aLanguages;
	for (;pLang->lid;pLang++){
		if (pLang->lid == lidSelected)
			return CString(pLang->pszISOLocale) + _T(".dll"); 
	}
	ASSERT ( false );
	return CString("");
}

void CPreferences::InitThreadLocale()
{
	ASSERT( m_wLanguageID != 0 );
#ifdef _UNICODE
	// set thread locale, this is used for:
	//	- MBCS->Unicode conversions (e.g. search results).
	//	- Unicode->MBCS conversions (e.g. publishing local files (names) in network, or savint text files on local disk)...
	SetThreadLocale(m_wLanguageID);

	const SLanguage* pLangs = _aLanguages;
	while (pLangs->lid)
	{
		if (pLangs->lid == m_wLanguageID)
		{
			// if we set the thread locale (see comments above) we also have to specify the proper
			// codepage for the C-RTL, otherwise we may not be able to store some strings as MBCS (Unicode->MBCS
			// conversion may fail)
			if (pLangs->uCodepage)
			{
				CString strCodepage;
				strCodepage.Format(_T(".%u"), pLangs->uCodepage);
				_tsetlocale(LC_CTYPE, strCodepage);
			}
			break;
		}
		pLangs++;
	}
#endif
}

void InitThreadLocale()
{
	thePrefs.InitThreadLocale();
}

CString CPreferences::GetHtmlCharset()
{
	ASSERT( m_wLanguageID != 0 );

	LPCTSTR pszHtmlCharset = NULL;
	const SLanguage* pLangs = _aLanguages;
	while (pLangs->lid)
	{
		if (pLangs->lid == m_wLanguageID)
		{
			pszHtmlCharset = pLangs->pszHtmlCharset;
			break;
		}
		pLangs++;
	}

	if (pszHtmlCharset == NULL || pszHtmlCharset[0] == _T('\0'))
	{
		ASSERT(0); // should never come here

		// try to get charset from codepage
		LPCTSTR pszLcLocale = _tsetlocale(LC_CTYPE, NULL);
		if (pszLcLocale)
		{
			TCHAR szLocaleID[128];
			UINT uCodepage = 0;
			if (_stscanf(pszLcLocale, _T("%[a-zA-Z_].%u"), szLocaleID, &uCodepage) == 2 && uCodepage != 0)
			{
				CString strHtmlCodepage;
				strHtmlCodepage.Format(_T("windows-%u"), uCodepage);
				return strHtmlCodepage;
			}
		}
	}

	return pszHtmlCharset;
}
