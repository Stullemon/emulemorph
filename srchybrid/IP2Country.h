//EastShare Start - added by AndCycle, IP to Country

// by Superlexx, based on IPFilter by Bouc7
#pragma once
#include "loggable.h"
#include <atlcoll.h>

struct IPRange_Struct2{
	uint32          IPstart;
	uint32          IPend;
	CString			ShortCountryName;
	CString			MidCountryName;
	CString			LongCountryName;
	WORD			FlagIndex;
	~IPRange_Struct2() {  }
};

#define DFLT_IP2COUNTRY_FILENAME  _T("ip-to-country.csv")//Commander - Added: IP2Country auto-updating

class CIP2Country: public CLoggable
{
	public:
		CIP2Country(void);
		~CIP2Country(void);
		
		void	Load();
		void	Unload();

		//reset ip2country referense
		void	Reset();

		//refresh passive windows
		void	Refresh();

		bool	IsIP2Country()			{return EnableIP2Country;}
		bool	ShowCountryFlag();

		IPRange_Struct2*	GetDefaultIP2Country() {return &defaultIP2Country;}

		bool	LoadFromFile();
		bool	LoadCountryFlagLib();
		void	RemoveAllIPs();
		void	RemoveAllFlags();

		bool	AddIPRange(uint32 IPfrom,uint32 IPto, CString shortCountryName, CString midCountryName, CString longCountryName);

		IPRange_Struct2*	GetCountryFromIP(uint32 IP);
		WORD	GetFlagResIDfromCountryCode(CString shortCountryName);

		CImageList* GetFlagImageList() {return &CountryFlagImageList;}
		void    UpdateIP2CountryURL();//Commander - Added: IP2Country auto-updating

	private:

		//check is program current running, if it's under init or shutdown, set to false
		bool	m_bRunning;

		HINSTANCE _hCountryFlagDll;
		CImageList	CountryFlagImageList;

		bool	EnableIP2Country;
		bool	EnableCountryFlag;
		struct	IPRange_Struct2 defaultIP2Country;

		CRBMap<uint32, IPRange_Struct2*> iplist;
		CRBMap<CString, uint16>	CountryIDtoFlagIndex;
};

//EastShare End - added by AndCycle, IP to Country
