//--- xrmb:funnynick ---
#pragma once


class CFunnyNick
{
public:
	CFunnyNick();
	char *gimmeFunnyNick(const uchar *id);

private:
	CStringList p;
	CStringList s;
};

extern CFunnyNick funnyNick;
