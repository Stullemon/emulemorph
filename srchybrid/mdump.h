#pragma once

struct _EXCEPTION_POINTERS;

class MiniDumper
{
private:
	static LPCTSTR m_szAppName;

	static LONG WINAPI TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo );

public:
	MiniDumper( LPCTSTR szAppName );
	~MiniDumper();
};
