//EastShare Start - added by AndCycle, IP to Country

// by Superlexx, based on IPFilter by Bouc7
#pragma once
#include "loggable.h"
#include <map>

struct IPRange_Struct2{
	uint32          IPstart;
	uint32          IPend;
	CString			ShortCountryName;
	CString			MidCountryName;
	CString			LongCountryName;
	~IPRange_Struct2() {  }
};

class CIP2Country: public CLoggable
{
	public:
		CIP2Country(void);
		~CIP2Country(void);
		
		bool	IsIP2Country()			{return EnableIP2Country;}
		IPRange_Struct2*	GetDefaultIP2Country()	{return &defaultIP2Country;}
		bool	LoadFromFile();
		void	RemoveAllIPs();
		bool	AddIPRange(uint32 IPfrom,uint32 IPto, CString shortCountryName, CString midCountryName, CString longCountryName);
		IPRange_Struct2*	GetCountryFromIP(uint32 IP);
	private:
		bool	EnableIP2Country;
		struct	IPRange_Struct2 defaultIP2Country;
		std::map<uint32,IPRange_Struct2*> iplist;
};

//EastShare End - added by AndCycle, IP to Country
