//EastShare Start - added by AndCycle, IP to Country

/*
the IP to country data is provided by http://ip-to-country.webhosting.info/

"IP2Country uses the IP-to-Country Database
 provided by WebHosting.Info (http://www.webhosting.info),
 available from http://ip-to-country.webhosting.info."

 */

/*

flags are from http://sf.net/projects/flags/

*/

// by Superlexx, based on IPFilter by Bouc7

#include "StdAfx.h"
#include "IP2Country.h"
#include "emule.h"
#include "otherfunctions.h"
#include <flag/resource.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	NO_FLAG	65535

CIP2Country::CIP2Country()
{
	defaultIP2Country.IPstart = 0;
	defaultIP2Country.IPend = 0;

	defaultIP2Country.ShortCountryName = "N/A";
	defaultIP2Country.MidCountryName = "N/A";
	defaultIP2Country.LongCountryName = "Not Applicable";

	defaultIP2Country.FlagIndex = NO_FLAG;

	EnableIP2Country = false;
	EnableCountryFlag = false;

	if(theApp.glob_prefs->GetIP2CountryNameMode() != IP2CountryName_DISABLE || 
		theApp.glob_prefs->IsIP2CountryShowFlag()){

		EnableCountryFlag = LoadCountryFlagLib();//flag lib first, so ip range can map to flag
		EnableIP2Country = LoadFromFile();

		AddLogLine(false, "IP2Country uses the IP-to-Country Database provided by WebHosting.Info (http://www.webhosting.info), available from http://ip-to-country.webhosting.info.");
	}
}

CIP2Country::~CIP2Country()
{
	RemoveAllIPs();
	RemoveAllFlags();
}

bool CIP2Country::LoadFromFile(){

	char buffer[1024];
	int	lenBuf = 1024;

	RemoveAllIPs();

	CString ip2countryCSVfile = theApp.glob_prefs->GetConfigDir()+"ip-to-country.csv";

	FILE* readFile = fopen(ip2countryCSVfile, "r");

	try{
		if (readFile != NULL) {

			int count = 0;

			while (!feof(readFile)) {
				if (fgets(buffer,lenBuf,readFile)==0) break;
				CString	sbuffer;
				sbuffer = buffer;
				/*
					http://ip-to-country.webhosting.info/node/view/54

					This is a sample of how the CSV file is structured:

					"0033996344","0033996351","GB","GBR","UNITED KINGDOM"
					"0050331648","0083886079","US","USA","UNITED STATES"
					"0094585424","0094585439","SE","SWE","SWEDEN"

					FIELD  			DATA TYPE		  	FIELD DESCRIPTION
					IP_FROM 		NUMERICAL (DOUBLE) 	Beginning of IP address range.
					IP_TO			NUMERICAL (DOUBLE) 	Ending of IP address range.
					COUNTRY_CODE2 	CHAR(2)				Two-character country code based on ISO 3166.
					COUNTRY_CODE3 	CHAR(3)				Three-character country code based on ISO 3166.
					COUNTRY_NAME 	VARCHAR(50) 		Country name based on ISO 3166
				*/
				// we assume that the ip-to-country.csv is valid and doesn't cause any troubles
				// get & process IP range
				sbuffer.Remove('"'); // get rid of the " signs

				CString tempStr[5];

				int curPos;
				curPos = 0;

				for(int forCount = 0; forCount !=  5; forCount++){
					tempStr[forCount] = sbuffer.Tokenize(",", curPos);
					if(tempStr[forCount].IsEmpty()) {
						throw CString(_T("error line in"));
					}
					count++;
					AddIPRange(atoi(tempStr[0]),atoi(tempStr[1]), tempStr[2], tempStr[3], tempStr[4]);
				}
			}
			fclose(readFile);
		}
		else{
			throw CString(_T("Failed to load in"));
		}
	}
	catch(CString error){
		AddLogLine(false, "%s %s", error, ip2countryCSVfile);
		RemoveAllIPs();
		return false;
	}
	AddLogLine(false, "IP2Countryfile loaded");
	return true;

}

bool CIP2Country::LoadCountryFlagLib(){

	CString ip2countryCountryFlag = theApp.glob_prefs->GetConfigDir()+"countryflag.dll";

	try{
		_hCountryFlagDll = LoadLibrary(ip2countryCountryFlag); 
		if (_hCountryFlagDll == NULL) 
		{ 
			throw CString(_T("CountryFlag Disabled, failed to load"));
		} 

		uint16	resID[] = {
			IDI_COUNTRY_FLAG_AD, IDI_COUNTRY_FLAG_AE, IDI_COUNTRY_FLAG_AF, IDI_COUNTRY_FLAG_AG, 
			IDI_COUNTRY_FLAG_AI, IDI_COUNTRY_FLAG_AL, IDI_COUNTRY_FLAG_AM, IDI_COUNTRY_FLAG_AN, 
			IDI_COUNTRY_FLAG_AO, IDI_COUNTRY_FLAG_AR, IDI_COUNTRY_FLAG_AS, IDI_COUNTRY_FLAG_AT, 
			IDI_COUNTRY_FLAG_AU, IDI_COUNTRY_FLAG_AW, IDI_COUNTRY_FLAG_AZ, IDI_COUNTRY_FLAG_BA, 
			IDI_COUNTRY_FLAG_BB, IDI_COUNTRY_FLAG_BD, IDI_COUNTRY_FLAG_BE, IDI_COUNTRY_FLAG_BF, 
			IDI_COUNTRY_FLAG_BG, IDI_COUNTRY_FLAG_BH, IDI_COUNTRY_FLAG_BI, IDI_COUNTRY_FLAG_BJ, 
			IDI_COUNTRY_FLAG_BM, IDI_COUNTRY_FLAG_BN, IDI_COUNTRY_FLAG_BO, IDI_COUNTRY_FLAG_BR, 
			IDI_COUNTRY_FLAG_BS, IDI_COUNTRY_FLAG_BT, IDI_COUNTRY_FLAG_BW, IDI_COUNTRY_FLAG_BY, 
			IDI_COUNTRY_FLAG_BZ, IDI_COUNTRY_FLAG_CA, IDI_COUNTRY_FLAG_CC, IDI_COUNTRY_FLAG_CD, 
			IDI_COUNTRY_FLAG_CF, IDI_COUNTRY_FLAG_CG, IDI_COUNTRY_FLAG_CH, IDI_COUNTRY_FLAG_CI, 
			IDI_COUNTRY_FLAG_CK, IDI_COUNTRY_FLAG_CL, IDI_COUNTRY_FLAG_CM, IDI_COUNTRY_FLAG_CN, 
			IDI_COUNTRY_FLAG_CO, IDI_COUNTRY_FLAG_CR, IDI_COUNTRY_FLAG_CU, IDI_COUNTRY_FLAG_CV, 
			IDI_COUNTRY_FLAG_CX, IDI_COUNTRY_FLAG_CY, IDI_COUNTRY_FLAG_CZ, IDI_COUNTRY_FLAG_DE, 
			IDI_COUNTRY_FLAG_DJ, IDI_COUNTRY_FLAG_DK, IDI_COUNTRY_FLAG_DM, IDI_COUNTRY_FLAG_DO, 
			IDI_COUNTRY_FLAG_DZ, IDI_COUNTRY_FLAG_EC, IDI_COUNTRY_FLAG_EE, IDI_COUNTRY_FLAG_EG, 
			IDI_COUNTRY_FLAG_EH, IDI_COUNTRY_FLAG_ER, IDI_COUNTRY_FLAG_ES, IDI_COUNTRY_FLAG_ET, 
			IDI_COUNTRY_FLAG_FI, IDI_COUNTRY_FLAG_FJ, IDI_COUNTRY_FLAG_FK, IDI_COUNTRY_FLAG_FM, 
			IDI_COUNTRY_FLAG_FO, IDI_COUNTRY_FLAG_FR, IDI_COUNTRY_FLAG_GA, IDI_COUNTRY_FLAG_GB, 
			IDI_COUNTRY_FLAG_GD, IDI_COUNTRY_FLAG_GE, IDI_COUNTRY_FLAG_GG, IDI_COUNTRY_FLAG_GH, 
			IDI_COUNTRY_FLAG_GI, IDI_COUNTRY_FLAG_GK, IDI_COUNTRY_FLAG_GL, IDI_COUNTRY_FLAG_GM, 
			IDI_COUNTRY_FLAG_GN, IDI_COUNTRY_FLAG_GP, IDI_COUNTRY_FLAG_GQ, IDI_COUNTRY_FLAG_GR, 
			IDI_COUNTRY_FLAG_GS, IDI_COUNTRY_FLAG_GT, IDI_COUNTRY_FLAG_GU, IDI_COUNTRY_FLAG_GW, 
			IDI_COUNTRY_FLAG_GY, IDI_COUNTRY_FLAG_HK, IDI_COUNTRY_FLAG_HN, IDI_COUNTRY_FLAG_HR, 
			IDI_COUNTRY_FLAG_HT, IDI_COUNTRY_FLAG_HU, IDI_COUNTRY_FLAG_ID, IDI_COUNTRY_FLAG_IE, 
			IDI_COUNTRY_FLAG_IL, IDI_COUNTRY_FLAG_IM, IDI_COUNTRY_FLAG_IN, IDI_COUNTRY_FLAG_IO, 
			IDI_COUNTRY_FLAG_IQ, IDI_COUNTRY_FLAG_IR, IDI_COUNTRY_FLAG_IS, IDI_COUNTRY_FLAG_IT, 
			IDI_COUNTRY_FLAG_JE, IDI_COUNTRY_FLAG_JM, IDI_COUNTRY_FLAG_JO, IDI_COUNTRY_FLAG_JP, 
			IDI_COUNTRY_FLAG_KE, IDI_COUNTRY_FLAG_KG, IDI_COUNTRY_FLAG_KH, IDI_COUNTRY_FLAG_KI, 
			IDI_COUNTRY_FLAG_KM, IDI_COUNTRY_FLAG_KN, IDI_COUNTRY_FLAG_KP, IDI_COUNTRY_FLAG_KR, 
			IDI_COUNTRY_FLAG_KW, IDI_COUNTRY_FLAG_KY, IDI_COUNTRY_FLAG_KZ, IDI_COUNTRY_FLAG_LA, 
			IDI_COUNTRY_FLAG_LB, IDI_COUNTRY_FLAG_LC, IDI_COUNTRY_FLAG_LI, IDI_COUNTRY_FLAG_LK, 
			IDI_COUNTRY_FLAG_LR, IDI_COUNTRY_FLAG_LS, IDI_COUNTRY_FLAG_LT, IDI_COUNTRY_FLAG_LU, 
			IDI_COUNTRY_FLAG_LV, IDI_COUNTRY_FLAG_LY, IDI_COUNTRY_FLAG_MA, IDI_COUNTRY_FLAG_MC, 
			IDI_COUNTRY_FLAG_MD, IDI_COUNTRY_FLAG_MG, IDI_COUNTRY_FLAG_MH, IDI_COUNTRY_FLAG_MK, 
			IDI_COUNTRY_FLAG_ML, IDI_COUNTRY_FLAG_MM, IDI_COUNTRY_FLAG_MN, IDI_COUNTRY_FLAG_MO, 
			IDI_COUNTRY_FLAG_MP, IDI_COUNTRY_FLAG_MQ, IDI_COUNTRY_FLAG_MR, IDI_COUNTRY_FLAG_MS, 
			IDI_COUNTRY_FLAG_MT, IDI_COUNTRY_FLAG_MU, IDI_COUNTRY_FLAG_MV, IDI_COUNTRY_FLAG_MW, 
			IDI_COUNTRY_FLAG_MX, IDI_COUNTRY_FLAG_MY, IDI_COUNTRY_FLAG_MZ, IDI_COUNTRY_FLAG_NA, 
			IDI_COUNTRY_FLAG_NC, IDI_COUNTRY_FLAG_NE, IDI_COUNTRY_FLAG_NF, IDI_COUNTRY_FLAG_NG, 
			IDI_COUNTRY_FLAG_NI, IDI_COUNTRY_FLAG_NL, IDI_COUNTRY_FLAG_NO, IDI_COUNTRY_FLAG_NP, 
			IDI_COUNTRY_FLAG_NR, IDI_COUNTRY_FLAG_NU, IDI_COUNTRY_FLAG_NZ, IDI_COUNTRY_FLAG_OM, 
			IDI_COUNTRY_FLAG_PA, IDI_COUNTRY_FLAG_PC, IDI_COUNTRY_FLAG_PE, IDI_COUNTRY_FLAG_PF, 
			IDI_COUNTRY_FLAG_PG, IDI_COUNTRY_FLAG_PH, IDI_COUNTRY_FLAG_PK, IDI_COUNTRY_FLAG_PL, 
			IDI_COUNTRY_FLAG_PM, IDI_COUNTRY_FLAG_PN, IDI_COUNTRY_FLAG_PR, IDI_COUNTRY_FLAG_PS, 
			IDI_COUNTRY_FLAG_PT, IDI_COUNTRY_FLAG_PW, IDI_COUNTRY_FLAG_PY, IDI_COUNTRY_FLAG_QA, 
			IDI_COUNTRY_FLAG_RO, IDI_COUNTRY_FLAG_RU, IDI_COUNTRY_FLAG_RW, IDI_COUNTRY_FLAG_SA, 
			IDI_COUNTRY_FLAG_SB, IDI_COUNTRY_FLAG_SC, IDI_COUNTRY_FLAG_SD, IDI_COUNTRY_FLAG_SE, 
			IDI_COUNTRY_FLAG_SG, IDI_COUNTRY_FLAG_SH, IDI_COUNTRY_FLAG_SI, IDI_COUNTRY_FLAG_SK, 
			IDI_COUNTRY_FLAG_SL, IDI_COUNTRY_FLAG_SM, IDI_COUNTRY_FLAG_SN, IDI_COUNTRY_FLAG_SO, 
			IDI_COUNTRY_FLAG_SR, IDI_COUNTRY_FLAG_ST, IDI_COUNTRY_FLAG_SU, IDI_COUNTRY_FLAG_SV, 
			IDI_COUNTRY_FLAG_SY, IDI_COUNTRY_FLAG_SZ, IDI_COUNTRY_FLAG_TC, IDI_COUNTRY_FLAG_TD, 
			IDI_COUNTRY_FLAG_TF, IDI_COUNTRY_FLAG_TG, IDI_COUNTRY_FLAG_TH, IDI_COUNTRY_FLAG_TJ, 
			IDI_COUNTRY_FLAG_TK, IDI_COUNTRY_FLAG_TL, IDI_COUNTRY_FLAG_TM, IDI_COUNTRY_FLAG_TN, 
			IDI_COUNTRY_FLAG_TO, IDI_COUNTRY_FLAG_TR, IDI_COUNTRY_FLAG_TT, IDI_COUNTRY_FLAG_TV, 
			IDI_COUNTRY_FLAG_TW, IDI_COUNTRY_FLAG_TZ, IDI_COUNTRY_FLAG_UA, IDI_COUNTRY_FLAG_UG, 
			IDI_COUNTRY_FLAG_UM, IDI_COUNTRY_FLAG_US, IDI_COUNTRY_FLAG_UY, IDI_COUNTRY_FLAG_UZ, 
			IDI_COUNTRY_FLAG_VA, IDI_COUNTRY_FLAG_VC, IDI_COUNTRY_FLAG_VE, IDI_COUNTRY_FLAG_VG, 
			IDI_COUNTRY_FLAG_VI, IDI_COUNTRY_FLAG_VN, IDI_COUNTRY_FLAG_VU, IDI_COUNTRY_FLAG_WF, 
			IDI_COUNTRY_FLAG_WS, IDI_COUNTRY_FLAG_YE, IDI_COUNTRY_FLAG_YU, IDI_COUNTRY_FLAG_ZA, 
			IDI_COUNTRY_FLAG_ZM, IDI_COUNTRY_FLAG_ZW, 
			NO_FLAG};

		CString countryID[] = {
			"AD", "AE", "AF", "AG", "AI", "AL", "AM", "AN", "AO", "AR", "AS", "AT", "AU", "AW", "AZ", 
			"BA", "BB", "BD", "BE", "BF", "BG", "BH", "BI", "BJ", "BM", "BN", "BO", "BR", "BS", "BT", 
			"BW", "BY", "BZ", "CA", "CC", "CD", "CF", "CG", "CH", "CI", "CK", "CL", "CM", "CN", "CO", 
			"CR", "CU", "CV", "CX", "CY", "CZ", "DE", "DJ", "DK", "DM", "DO", "DZ", "EC", "EE", "EG", 
			"EH", "ER", "ES", "ET", "FI", "FJ", "FK", "FM", "FO", "FR", "GA", "GB", "GD", "GE", "GG", 
			"GH", "GI", "GK", "GL", "GM", "GN", "GP", "GQ", "GR", "GS", "GT", "GU", "GW", "GY", "HK", 
			"HN", "HR", "HT", "HU", "ID", "IE", "IL", "IM", "IN", "IO", "IQ", "IR", "IS", "IT", "JE", 
			"JM", "JO", "JP", "KE", "KG", "KH", "KI", "KM", "KN", "KP", "KR", "KW", "KY", "KZ", "LA", 
			"LB", "LC", "LI", "LK", "LR", "LS", "LT", "LU", "LV", "LY", "MA", "MC", "MD", "MG", "MH", 
			"MK", "ML", "MM", "MN", "MO", "MP", "MQ", "MR", "MS", "MT", "MU", "MV", "MW", "MX", "MY", 
			"MZ", "NA", "NC", "NE", "NF", "NG", "NI", "NL", "NO", "NP", "NR", "NU", "NZ", "OM", "PA", 
			"PC", "PE", "PF", "PG", "PH", "PK", "PL", "PM", "PN", "PR", "PS", "PT", "PW", "PY", "QA", 
			"RO", "RU", "RW", "SA", "SB", "SC", "SD", "SE", "SG", "SH", "SI", "SK", "SL", "SM", "SN", 
			"SO", "SR", "ST", "SU", "SV", "SY", "SZ", "TC", "TD", "TF", "TG", "TH", "TJ", "TK", "TL", 
			"TM", "TN", "TO", "TR", "TT", "TV", "TW", "TZ", "UA", "UG", "UM", "US", "UY", "UZ", "VA", 
			"VC", "VE", "VG", "VI", "VN", "VU", "WF", "WS", "YE", "YU", "ZA", "ZM", "ZW", 
			"N/A"
		};

		HICON iconHandle;

		for(int cur_pos = 0; resID[cur_pos] != NO_FLAG; cur_pos++){

			CountryIDtoFlagIndex[countryID[cur_pos]] = cur_pos;
			iconHandle = LoadIcon(_hCountryFlagDll, MAKEINTRESOURCE(resID[cur_pos]));
			if(iconHandle == NULL) throw CString(_T("Invalid resID"));
			CountryFlagIcon[cur_pos] = iconHandle;
			//CountryFlagIcon.AddTail(iconHandle);
		}

	}
	catch(CString error){
		AddLogLine(false, "%s in %s", error, ip2countryCountryFlag);
		RemoveAllFlags();
		return false;
	}
	AddLogLine(false, "Country Flags have been loaded");
	return true;
}

void CIP2Country::RemoveAllIPs(){
	IPRange_Struct2* search;

	std::map<uint32, IPRange_Struct2*>::const_iterator it;
	for (it = iplist.begin(); it != iplist.end(); it++ ) {
		search=(*it).second;
		delete search;
	}

	iplist.clear();
}

void CIP2Country::RemoveAllFlags(){


	std::map<uint16, HICON>::const_iterator it;
	for(it = CountryFlagIcon.begin(); it != CountryFlagIcon.end(); it++){
		if(it->second != NULL)	DestroyIcon(it->second);
	}

	CountryFlagIcon.clear();
}

bool CIP2Country::AddIPRange(uint32 IPfrom,uint32 IPto, CString shortCountryName, CString midCountryName, CString longCountryName){
	IPRange_Struct2* newRange = new IPRange_Struct2();

	newRange->IPstart = IPfrom;
	newRange->IPend = IPto;
	newRange->ShortCountryName = shortCountryName;
	newRange->MidCountryName = midCountryName;
	newRange->LongCountryName = longCountryName;

	if(EnableCountryFlag){
		std::map<CString, uint16>::const_iterator it;
		it = CountryIDtoFlagIndex.find(shortCountryName);

		if(it != NULL){
			newRange->FlagIndex = it->second;
		}
		else{
			newRange->FlagIndex = NO_FLAG;
		}
	}
	else{
		newRange->FlagIndex = NO_FLAG;
	}
	
	iplist[IPfrom] = newRange;
	return true;
}

struct IPRange_Struct2* CIP2Country::GetCountryFromIP(uint32 ClientIP){
	if (iplist.size()==0 || ClientIP==0){
		AddDebugLogLine(false, "CIP2Country::GetCountryFromIP doesn't have ip to search for, or iplist doesn't exist");
		return &defaultIP2Country;
	}

	IPRange_Struct2* search;
	ClientIP = htonl(ClientIP);

	std::map<uint32, IPRange_Struct2*>::const_iterator it = iplist.upper_bound(ClientIP);
	it--;
	do {
		search = (*it).second;
		if (search->IPend<ClientIP) return &defaultIP2Country;
		if (search->IPstart<=ClientIP && ClientIP<=search->IPend)	return search;
		it--;
	} while (it != iplist.begin());
	return &defaultIP2Country;
}

HICON CIP2Country::GetCountryFlagByIndex(int index){

	if(index == NO_FLAG) return NULL;

	std::map<uint16, HICON>::const_iterator it;
	it = CountryFlagIcon.find(index);
	if(it == NULL){
		return NULL;
	}
	return it->second;
}

int CIP2Country::GetCountryFlagAmount(){

	return CountryFlagIcon.size();
}

bool CIP2Country::ShowCountryFlag(){

	return (theApp.glob_prefs->IsIP2CountryShowFlag() && EnableCountryFlag);
}

//EastShare End - added by AndCycle, IP to Country