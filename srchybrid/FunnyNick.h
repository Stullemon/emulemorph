//--- xrmb:funnynick ---
#pragma once


class CFunnyNick
{
public:
	CFunnyNick();
	TCHAR *gimmeFunnyNick(const uchar *id);

private:
	CStringList p;
	CStringList s;
};

extern CFunnyNick funnyNick;
