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

class CIP2Country: public CLoggable
{
	public:
		CIP2Country(void);
		~CIP2Country(void);
		
		bool	IsIP2Country()			{return EnableIP2Country;}
		bool	LoadedCountryFlag()		{return EnableCountryFlag;}
		bool	ShowCountryFlag();
		IPRange_Struct2*	GetDefaultIP2Country()	{return &defaultIP2Country;}
		bool	LoadFromFile();
		bool	LoadCountryFlagLib();
		void	RemoveAllIPs();
		void	RemoveAllFlags();
		bool	AddIPRange(uint32 IPfrom,uint32 IPto, CString shortCountryName, CString midCountryName, CString longCountryName);
		IPRange_Struct2*	GetCountryFromIP(uint32 IP);
		HICON	GetCountryFlagByIndex(int index);
		int		GetCountryFlagAmount(){return FlagAmount;};
		WORD	GetFlagResIDfromCountryCode(CString shortCountryName);
	private:
		HINSTANCE _hCountryFlagDll;
		int		FlagAmount;

		bool	EnableIP2Country;
		bool	EnableCountryFlag;
		struct	IPRange_Struct2 defaultIP2Country;

		CRBMap<uint32, IPRange_Struct2*> iplist;
		CRBMap<uint16, HICON>	CountryFlagIcon;
		CRBMap<CString, uint16>	CountryIDtoFlagIndex;
};

//EastShare End - added by AndCycle, IP to Country
