//--- xrmb:funnynick ---

#include "stdafx.h"
#include "FunnyNick.h"
#include "otherfunctions.h"


CFunnyNick::CFunnyNick()
{
	// preffix table
	p.AddTail(_T("ATX-"));
	p.AddTail(_T("Gameboy "));
	p.AddTail(_T("PS/2-"));
	p.AddTail(_T("USB-"));
	p.AddTail(_T("Angry "));
	p.AddTail(_T("Atrocious "));
	p.AddTail(_T("Attractive "));
	p.AddTail(_T("Bad "));
	p.AddTail(_T("Barbarious "));
	p.AddTail(_T("Beautiful "));
	p.AddTail(_T("Black "));
	p.AddTail(_T("Blond "));
	p.AddTail(_T("Blue "));
	p.AddTail(_T("Bright "));
	p.AddTail(_T("Brown "));
	p.AddTail(_T("Cool "));
	p.AddTail(_T("Cruel "));
	p.AddTail(_T("Cubic "));
	p.AddTail(_T("Cute "));
	p.AddTail(_T("Dance "));
	p.AddTail(_T("Dark "));
	p.AddTail(_T("Dinky "));
	p.AddTail(_T("Drunk "));
	p.AddTail(_T("Dumb "));
	p.AddTail(_T("E"));
	p.AddTail(_T("Electro "));
	p.AddTail(_T("Elite "));
	p.AddTail(_T("Fast "));
	p.AddTail(_T("Flying "));
	p.AddTail(_T("Fourios "));
	p.AddTail(_T("Frustraded "));
	p.AddTail(_T("Funny "));
	p.AddTail(_T("Furious "));
	p.AddTail(_T("Giant "));
	p.AddTail(_T("Giga "));
	p.AddTail(_T("Green "));
	p.AddTail(_T("Handsome "));
	p.AddTail(_T("Hard "));
	p.AddTail(_T("Harsh "));
	p.AddTail(_T("Hiphop "));
	p.AddTail(_T("Holy "));
	p.AddTail(_T("Horny "));
	p.AddTail(_T("Hot "));
	p.AddTail(_T("House "));
	p.AddTail(_T("I"));
	p.AddTail(_T("Lame "));
	p.AddTail(_T("Leaking "));
	p.AddTail(_T("Lone "));
	p.AddTail(_T("Lovely "));
	p.AddTail(_T("Lucky "));
	p.AddTail(_T("Micro "));
	p.AddTail(_T("Mighty "));
	p.AddTail(_T("Mini "));
	p.AddTail(_T("Nice "));
	p.AddTail(_T("Orange "));
	p.AddTail(_T("Pretty "));
	p.AddTail(_T("Red "));
	p.AddTail(_T("Sexy "));
	p.AddTail(_T("Slow "));
	p.AddTail(_T("Smooth "));
	p.AddTail(_T("Stinky "));
	p.AddTail(_T("Strong "));
	p.AddTail(_T("Super "));
	p.AddTail(_T("Unholy "));
	p.AddTail(_T("White "));
	p.AddTail(_T("Wild "));
	p.AddTail(_T("X"));
	p.AddTail(_T("XBox "));
	p.AddTail(_T("Yellow "));
	p.AddTail(_T("Kentucky Fried "));
	p.AddTail(_T("Mc"));
	p.AddTail(_T("Alien "));
	p.AddTail(_T("Bavarian "));
	p.AddTail(_T("Crazy "));
	p.AddTail(_T("Death "));
	p.AddTail(_T("Drunken "));
	p.AddTail(_T("Fat "));
	p.AddTail(_T("Hazardous "));
	p.AddTail(_T("Holy "));
	p.AddTail(_T("Infested "));
	p.AddTail(_T("Insane "));
	p.AddTail(_T("Mutated "));
	p.AddTail(_T("Nasty "));
	p.AddTail(_T("Purple "));
	p.AddTail(_T("Radioactive "));
	p.AddTail(_T("Ugly "));
	p.AddTail(_T("Green "));

	// suffix table
	s.AddTail(_T("16"));
	s.AddTail(_T("3"));
	s.AddTail(_T("6"));
	s.AddTail(_T("7"));
	s.AddTail(_T("Abe"));
	s.AddTail(_T("Bee"));
	s.AddTail(_T("Bird"));
	s.AddTail(_T("Boy"));
	s.AddTail(_T("Cat"));
	s.AddTail(_T("Cow"));
	s.AddTail(_T("Crow"));
	s.AddTail(_T("DJ"));
	s.AddTail(_T("Dad"));
	s.AddTail(_T("Deer"));
	s.AddTail(_T("Dog"));
	s.AddTail(_T("Donkey"));
	s.AddTail(_T("Duck"));
	s.AddTail(_T("Eagle"));
	s.AddTail(_T("Elephant"));
	s.AddTail(_T("Fly"));
	s.AddTail(_T("Fox"));
	s.AddTail(_T("Frog"));
	s.AddTail(_T("Girl"));
	s.AddTail(_T("Girlie"));
	s.AddTail(_T("Guinea Pig"));
	s.AddTail(_T("Hasi"));
	s.AddTail(_T("Hawk"));
	s.AddTail(_T("Jackal"));
	s.AddTail(_T("Lizard"));
	s.AddTail(_T("MC"));
	s.AddTail(_T("Men"));
	s.AddTail(_T("Mom"));
	s.AddTail(_T("Mouse"));
	s.AddTail(_T("Mule"));
	s.AddTail(_T("Pig"));
	s.AddTail(_T("Rabbit"));
	s.AddTail(_T("Rat"));
	s.AddTail(_T("Rhino"));
	s.AddTail(_T("Smurf"));
	s.AddTail(_T("Snail"));
	s.AddTail(_T("Snake"));
	s.AddTail(_T("Star"));
	s.AddTail(_T("Tiger"));
	s.AddTail(_T("Wolf"));
	s.AddTail(_T("Butterfly"));
	s.AddTail(_T("Elk"));
	s.AddTail(_T("Godzilla"));
	s.AddTail(_T("Horse"));
	s.AddTail(_T("Penguin"));
	s.AddTail(_T("Pony")); 
	s.AddTail(_T("Reindeer"));
	s.AddTail(_T("Sheep"));
	s.AddTail(_T("Sock Puppet"));
	s.AddTail(_T("Worm"));
	s.AddTail(_T("Bermuda"));
}



TCHAR* CFunnyNick::gimmeFunnyNick(const uchar *id)
{
	//--- if we get an id, we can generate the same random name for this user over and over... so much about randomness :) ---
	if(id)
	{
		uint32	x=0x7d726d62; // < xrmb :)
		uint8	a=id[5]  ^ id[7]  ^ id[15] ^ id[4];
		uint8	b=id[11] ^ id[9]  ^ id[12] ^ id[1];
		uint8	c=id[3]  ^ id[14] ^ id[6]  ^ id[13];
		uint8	d=id[2]  ^ id[0]  ^ id[10] ^ id[8];
		uint32	e=(a<<24) + (b<<16) + (c<<8) + d;
		srand(e^x);
	}

	// pick random suffix and prefix
	POSITION posp=p.GetHeadPosition();
	int ri=rand()%p.GetCount();
	for(int i=0; i<ri; ++i)
		p.GetNext(posp);

	POSITION poss=s.GetHeadPosition();
	int si=rand()%s.GetCount();
	for(int i=0; i<si; ++i)
		s.GetNext(poss);
	
	//--- make the rand random again ---
	if(id)
		srand((unsigned)time(NULL));

	CString nn=
	_T("[FunnyNick] ") + p.GetAt(posp)+s.GetAt(poss);

	return _tcsdup(nn.GetBuffer());
}

CFunnyNick funnyNick;
