//--- xrmb:funnynick ---

#include "stdafx.h"
#include "FunnyNick.h"
#include "otherfunctions.h"


CFunnyNick::CFunnyNick()
{
	// preffix table
	p.AddTail("ATX-");
	p.AddTail("Gameboy ");
	p.AddTail("PS/2-");
	p.AddTail("USB-");
	p.AddTail("Angry ");
	p.AddTail("Atrocious ");
	p.AddTail("Attractive ");
	p.AddTail("Bad ");
	p.AddTail("Barbarious ");
	p.AddTail("Beautiful ");
	p.AddTail("Black ");
	p.AddTail("Blond ");
	p.AddTail("Blue ");
	p.AddTail("Bright ");
	p.AddTail("Brown ");
	p.AddTail("Cool ");
	p.AddTail("Cruel ");
	p.AddTail("Cubic ");
	p.AddTail("Cute ");
	p.AddTail("Dance ");
	p.AddTail("Dark ");
	p.AddTail("Dinky ");
	p.AddTail("Drunk ");
	p.AddTail("Dumb ");
	p.AddTail("E");
	p.AddTail("Electro ");
	p.AddTail("Elite ");
	p.AddTail("Fast ");
	p.AddTail("Flying ");
	p.AddTail("Fourios ");
	p.AddTail("Frustraded ");
	p.AddTail("Funny ");
	p.AddTail("Furious ");
	p.AddTail("Giant ");
	p.AddTail("Giga ");
	p.AddTail("Green ");
	p.AddTail("Handsome ");
	p.AddTail("Hard ");
	p.AddTail("Harsh ");
	p.AddTail("Hiphop ");
	p.AddTail("Holy ");
	p.AddTail("Horny ");
	p.AddTail("Hot ");
	p.AddTail("House ");
	p.AddTail("I");
	p.AddTail("Lame ");
	p.AddTail("Leaking ");
	p.AddTail("Lone ");
	p.AddTail("Lovely ");
	p.AddTail("Lucky ");
	p.AddTail("Micro ");
	p.AddTail("Mighty ");
	p.AddTail("Mini ");
	p.AddTail("Nice ");
	p.AddTail("Orange ");
	p.AddTail("Pretty ");
	p.AddTail("Red ");
	p.AddTail("Sexy ");
	p.AddTail("Slow ");
	p.AddTail("Smooth ");
	p.AddTail("Stinky ");
	p.AddTail("Strong ");
	p.AddTail("Super ");
	p.AddTail("Unholy ");
	p.AddTail("White ");
	p.AddTail("Wild ");
	p.AddTail("X");
	p.AddTail("XBox ");
	p.AddTail("Yellow ");
	p.AddTail("Kentucky Fried ");
	p.AddTail("Mc");
	p.AddTail("Alien ");
	p.AddTail("Bavarian ");
	p.AddTail("Crazy ");
	p.AddTail("Death ");
	p.AddTail("Drunken ");
	p.AddTail("Fat ");
	p.AddTail("Hazardous ");
	p.AddTail("Holy ");
	p.AddTail("Infested ");
	p.AddTail("Insane ");
	p.AddTail("Mutated ");
	p.AddTail("Nasty ");
	p.AddTail("Purple ");
	p.AddTail("Radioactive ");
	p.AddTail("Ugly ");
	p.AddTail("Green ");

	// suffix table
	s.AddTail("16");
	s.AddTail("3");
	s.AddTail("6");
	s.AddTail("7");
	s.AddTail("Abe");
	s.AddTail("Bee");
	s.AddTail("Bird");
	s.AddTail("Boy");
	s.AddTail("Cat");
	s.AddTail("Cow");
	s.AddTail("Crow");
	s.AddTail("DJ");
	s.AddTail("Dad");
	s.AddTail("Deer");
	s.AddTail("Dog");
	s.AddTail("Donkey");
	s.AddTail("Duck");
	s.AddTail("Eagle");
	s.AddTail("Elephant");
	s.AddTail("Fly");
	s.AddTail("Fox");
	s.AddTail("Frog");
	s.AddTail("Girl");
	s.AddTail("Girlie");
	s.AddTail("Guinea Pig");
	s.AddTail("Hasi");
	s.AddTail("Hawk");
	s.AddTail("Jackal");
	s.AddTail("Lizard");
	s.AddTail("MC");
	s.AddTail("Men");
	s.AddTail("Mom");
	s.AddTail("Mouse");
	s.AddTail("Mule");
	s.AddTail("Pig");
	s.AddTail("Rabbit");
	s.AddTail("Rat");
	s.AddTail("Rhino");
	s.AddTail("Smurf");
	s.AddTail("Snail");
	s.AddTail("Snake");
	s.AddTail("Star");
	s.AddTail("Tiger");
	s.AddTail("Wolf");
	s.AddTail("Butterfly");
	s.AddTail("Elk");
	s.AddTail("Godzilla");
	s.AddTail("Horse");
	s.AddTail("Penguin");
	s.AddTail("Pony"); 
	s.AddTail("Reindeer");
	s.AddTail("Sheep");
	s.AddTail("Sock Puppet");
	s.AddTail("Worm");
	s.AddTail("Bermuda");
}



char* CFunnyNick::gimmeFunnyNick(const uchar *id)
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
	"[FunnyNick] " + p.GetAt(posp)+s.GetAt(poss);

	return nstrdup(nn.GetBuffer());
}

CFunnyNick funnyNick;
