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
#define LANGID_DA_DK MAKELANGID(LANG_DANISH,SUBLANG_DEFAULT)
#define LANGID_DE_DE MAKELANGID(LANG_GERMAN,SUBLANG_DEFAULT)
#define LANGID_EL_GR MAKELANGID(LANG_GREEK,SUBLANG_DEFAULT)
#define LANGID_EN_US MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT)
#define LANGID_ES_ES_T MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH)
#define LANGID_ET_EE MAKELANGID(LANG_ESTONIAN,SUBLANG_DEFAULT)
#define LANGID_FI_FI MAKELANGID(LANG_FINNISH,SUBLANG_DEFAULT)
#define LANGID_FR_FR MAKELANGID(LANG_FRENCH,SUBLANG_DEFAULT)
#define LANGID_GL_ES MAKELANGID(LANG_GALICIAN,SUBLANG_DEFAULT)
#define LANGID_HU_HU MAKELANGID(LANG_HUNGARIAN,SUBLANG_DEFAULT)
#define LANGID_IT_IT MAKELANGID(LANG_ITALIAN,SUBLANG_DEFAULT)
#define LANGID_KO_KR MAKELANGID(LANG_KOREAN,SUBLANG_DEFAULT)
#define LANGID_LT_LT MAKELANGID(LANG_LITHUANIAN,SUBLANG_DEFAULT)
#define LANGID_LV_LV MAKELANGID(LANG_LATVIAN,SUBLANG_DEFAULT)
#define LANGID_NB_NO MAKELANGID(LANG_NORWEGIAN,SUBLANG_NORWEGIAN_BOKMAL)
#define LANGID_NL_NL MAKELANGID(LANG_DUTCH,SUBLANG_DEFAULT)
#define LANGID_PL_PL MAKELANGID(LANG_POLISH,SUBLANG_DEFAULT)
#define LANGID_PT_BR MAKELANGID(LANG_PORTUGUESE,SUBLANG_PORTUGUESE_BRAZILIAN)
#define LANGID_PT_PT MAKELANGID(LANG_PORTUGUESE,SUBLANG_PORTUGUESE)
#define LANGID_RU_RU MAKELANGID(LANG_RUSSIAN,SUBLANG_DEFAULT)
#define LANGID_SL_SI MAKELANGID(LANG_SLOVENIAN,SUBLANG_DEFAULT)
#define LANGID_SV_SE MAKELANGID(LANG_SWEDISH,SUBLANG_DEFAULT)
#define LANGID_TR_TR MAKELANGID(LANG_TURKISH,SUBLANG_DEFAULT)
#define LANGID_ZH_CN MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED)
#define LANGID_ZH_TW MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL)
#define LANGID_HE_IL MAKELANGID(LANG_HEBREW,SUBLANG_DEFAULT)
#define LANGID_JP_JP MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT)


static HINSTANCE _hLangDLL = NULL;

CString GetResString(UINT uStringID, WORD languageID)
{
	CString resString;
	if (_hLangDLL)
		resString.LoadString(_hLangDLL, uStringID, languageID);
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
};

static SLanguage _aLanguages[] =
{
	{LANGID_AR_AE,	_T(""),				FALSE,	_T("ar_AE")},
	{LANGID_BG_BG,	_T("hun"),			FALSE,	_T("bg_BG")},
	{LANGID_HU_HU,	_T("hungarian"),	FALSE,	_T("hu_HU")},
	{LANGID_CA_ES,	_T(""),				FALSE,	_T("ca_ES")},
	{LANGID_ZH_CN,	_T("chs"),			FALSE,	_T("zh_CN")},
	{LANGID_ZH_TW,	_T("cht"),			FALSE,	_T("zh_TW")},
	{LANGID_DA_DK,	_T("danish"),		FALSE,	_T("da_DK")},
	{LANGID_NL_NL,	_T("dutch"),		FALSE,	_T("nl_NL")},
	{LANGID_EN_US,	_T("english"),		TRUE,	_T("en_US")},
	{LANGID_ET_EE,	_T(""),				FALSE,	_T("et_EE")},
	{LANGID_FI_FI,	_T("finnish"),		FALSE,	_T("fi_FI")},
	{LANGID_FR_FR,	_T("french"),		FALSE,	_T("fr_FR")},
	{LANGID_GL_ES,	_T(""),				FALSE,	_T("gl_ES")},
	{LANGID_DE_DE,	_T("german"),		FALSE,	_T("de_DE")},
	{LANGID_EL_GR,	_T("greek"),		FALSE,	_T("el_GR")},
	{LANGID_IT_IT,	_T("italian"),		FALSE,	_T("it_IT")},
	{LANGID_KO_KR,	_T("korean"),		FALSE,	_T("ko_KR")},
	{LANGID_LV_LV,	_T(""),				FALSE,	_T("lv_LV")},
	{LANGID_LT_LT,	_T(""),				FALSE,	_T("lt_LT")},
	{LANGID_NB_NO,	_T("norwegian"),	FALSE,	_T("nb_NO")},
	{LANGID_PL_PL,	_T("polish"),		FALSE,	_T("pl_PL")},
	{LANGID_PT_PT,	_T("ptg"),			FALSE,	_T("pt_PT")},
	{LANGID_PT_BR,	_T("ptb"),			FALSE,	_T("pt_BR")},
	{LANGID_RU_RU,	_T("russian"),		FALSE,	_T("ru_RU")},
	{LANGID_SL_SI,	_T(""),				FALSE,	_T("sl_SI")},
	{LANGID_ES_ES_T,_T("spanish"),		FALSE,	_T("es_ES_T")},
	{LANGID_SV_SE,	_T("swedish"),		FALSE,	_T("sv_SE")},
	{LANGID_TR_TR,	_T("turkish"),		FALSE,	_T("tr_TR")},
	{LANGID_HE_IL,	_T(""),				FALSE,	_T("he_IL")},
	{LANGID_JP_JP,	_T(""),				FALSE,	_T("jp_JP")},
	{LANG_CZECH,	_T(""),				FALSE,	_T("cz_CZ")},
	{0, NULL, 0}
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

void CPreferences::GetLanguages(CWordArray& aLanguageIDs) const
{
	const SLanguage* pLang = _aLanguages;
	while (pLang->lid){
		//if (pLang->bSupported)
		//show all languages, offer download if not supported ones later
		aLanguageIDs.Add(pLang->lid);
		pLang++;
	}
}

WORD CPreferences::GetLanguageID() const
{
	return prefs->languageID;
}

void CPreferences::SetLanguageID(WORD lid)
{
	prefs->languageID = lid;
}

bool CheckLangDLLVersion(const CString& rstrLangDLL)
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

bool LoadLangLib(const CString& rstrLangDir, LANGID lid)
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
				if (bLoadedLib){
					//_tsetlocale(LC_ALL, pLangs->pszLocale);
					//SetThreadLocale(lid);
					return true;
				}
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
	if (prefs->languageID)
		bFoundLang = LoadLangLib(GetLangDir(), prefs->languageID);

	if (!bFoundLang){
		LANGID lidLocale = (LANGID)::GetThreadLocale();
		//LANGID lidLocalePri = PRIMARYLANGID(::GetThreadLocale());
		//LANGID lidLocaleSub = SUBLANGID(::GetThreadLocale());

		bFoundLang = LoadLangLib(GetLangDir(), lidLocale);
		if (!bFoundLang){
			LoadLangLib(GetLangDir(), LANGID_EN_US);
			prefs->languageID = LANGID_EN_US;
			CString strLngEnglish = GetResString(IDS_MB_LANGUAGEINFO);
			AfxMessageBox(strLngEnglish, MB_ICONASTERISK);
		}
		else
			prefs->languageID = lidLocale;
	}

	// if loading a string fails, set language to English
	if (GetResString(IDS_MB_LANGUAGEINFO).IsEmpty()) {
		LoadLangLib(GetLangDir(), LANGID_EN_US);
		prefs->languageID = LANGID_EN_US;
	}
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
