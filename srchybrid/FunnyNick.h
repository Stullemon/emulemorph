//--- xrmb:funnynick ---
#pragma once


class CFunnyNick
{
public:
	CFunnyNick();
	char *gimmeFunnyNick(const char *id);

private:
	CStringList p;
	CStringList s;
};

extern CFunnyNick funnyNick;
