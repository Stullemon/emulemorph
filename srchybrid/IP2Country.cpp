//EastShare Start - added by AndCycle, IP to Country

/*
the IP to country data is provided by http://ip-to-country.webhosting.info/

"IP2Country uses the IP-to-Country Database
 provided by WebHosting.Info (http://www.webhosting.info),
 available from http://ip-to-country.webhosting.info."

 */

// by Superlexx, based on IPFilter by Bouc7

#include "StdAfx.h"
#include "IP2Country.h"
#include "emule.h"
#include "otherfunctions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CIP2Country::CIP2Country()
{
	defaultIP2Country.IPstart=0;
	defaultIP2Country.IPend=0;

	defaultIP2Country.ShortCountryName="NA";
	defaultIP2Country.MidCountryName="N/A";
	defaultIP2Country.LongCountryName="Not Applicable";

	EnableIP2Country = (theApp.glob_prefs->GetIP2CountryNameMode() != IP2CountryName_DISABLE);
	if(EnableIP2Country){
		EnableIP2Country = LoadFromFile();
		AddLogLine(false, "IP2Country uses the IP-to-Country Database provided by WebHosting.Info (http://www.webhosting.info), available from http://ip-to-country.webhosting.info.");
	}
}

CIP2Country::~CIP2Country()
{
	RemoveAllIPs();
}

bool CIP2Country::LoadFromFile(){

	char buffer[1024];
	int lenBuf = 1024;

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

				int curPos = 0;

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

			AddLogLine(false, "IP2Countryfile loaded, %i IP range have loaded in", count);
			return true;
		}
		else{
			throw CString(_T("Failed to load in"));
		}
	}
	catch(CString error){
		AddLogLine(false, "%s %s", error, ip2countryCSVfile);
		RemoveAllIPs();
	}
	return false;
}

void CIP2Country::RemoveAllIPs(){
	IPRange_Struct2* search;

	std::map<uint32, IPRange_Struct2*>::const_iterator it;
	for ( it = iplist.begin(); it != iplist.end(); ++it ) {
		search=(*it).second;
		delete search;
	}

	iplist.clear();
}

bool CIP2Country::AddIPRange(uint32 IPfrom,uint32 IPto, CString shortCountryName, CString midCountryName, CString longCountryName){
	IPRange_Struct2* newRange = new IPRange_Struct2();

	newRange->IPstart = IPfrom;
	newRange->IPend = IPto;
	newRange->ShortCountryName = shortCountryName;
	newRange->MidCountryName = midCountryName;
	newRange->LongCountryName = longCountryName;

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

//EastShare End - added by AndCycle, IP to Country