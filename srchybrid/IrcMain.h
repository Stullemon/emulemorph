//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once
#include "loggable.h"

class CIrcWnd;
class CIrcSocket;

class CIrcMain: public CLoggable
{
public:
	CIrcMain(void);
	~CIrcMain(void);
	void ParseMessage( CString message );
	void PreParseMessage( CString buffer );
	void SendLogin();
	void Connect();
	void Disconnect( bool isshuttingdown = false);
	void SetConnectStatus( bool connected );
	void SetIRCWnd(CIrcWnd* pwndIRC)	{m_pwndIRC = pwndIRC;}
	int SendString( CString send );
	void ParsePerform();
	void ProcessLink( CString ed2kLink );
	uint32 SetVerify()					{verify = rand();
						            	return verify;}

	CString GetNick()	{return nick;}
private:
	CIrcSocket*	ircsocket;
	CIrcWnd*	m_pwndIRC;
	CString		preParseBuffer;
	CString		user;
	CString		nick;
	CString		version;
	uint32		verify;
};
