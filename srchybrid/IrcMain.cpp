//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
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
#include "stdafx.h"
#pragma comment(lib, "winmm.lib")
#include <Mmsystem.h>
#include "emule.h"
#include "IRCMain.h"
#include "OtherFunctions.h"
#include "ED2KLink.h"
#include "DownloadQueue.h"
#include "Server.h"
#include "IRCSocket.h"
#include "MenuCmds.h"
#include "Sockets.h"
#include "FriendList.h"
#ifndef _CONSOLE
#include "emuledlg.h"
#include "ServerWnd.h"
#include "IRCWnd.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CIrcMain::CIrcMain(void)
{
	ircsocket = NULL;
	m_pwndIRC = 0; // i_a 
	preParseBuffer = "";
	srand( (unsigned)time( NULL ) );
	SetVerify();
}

CIrcMain::~CIrcMain(void)
{
}

void CIrcMain::PreParseMessage( CString buffer ){
	try
	{
		CString rawMessage;
		preParseBuffer += buffer;
		int test = preParseBuffer.Find('\n');
		try
		{
			while( test != -1 )
			{
				rawMessage = preParseBuffer.Left(test);
				rawMessage.Replace( "\n", "" );
				rawMessage.Replace( "\r", "" );
				ParseMessage( rawMessage );
				preParseBuffer = preParseBuffer.Mid(test+1);
				test = preParseBuffer.Find("\n");
			}
		}
		catch(...)
		{
			if (thePrefs.GetVerbose())
				AddDebugLogLine(false, "Exception in CIrcMain::PreParseMessage(1)");
		}
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, "Exception in CIrcMain::PreParseMessage(2)");
	}
}

//extern void URLDecode(CString& result, const char* buff);

void CIrcMain::ProcessLink( CString ed2kLink ) 
{
	try 
	{
		CString link;
		link=URLDecode(ed2kLink);
		CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(link);
		_ASSERT( pLink !=0 );
		switch (pLink->GetKind()) {
		case CED2KLink::kFile:
			{
				CED2KFileLink* pFileLink = pLink->GetFileLink();
				_ASSERT(pFileLink !=0);
				theApp.downloadqueue->AddFileLinkToDownload(pFileLink);
			}
			break;
		case CED2KLink::kServerList:
			{
				CED2KServerListLink* pListLink = pLink->GetServerListLink(); 
				_ASSERT( pListLink !=0 ); 
				CString strAddress = pListLink->GetAddress(); 
				if(strAddress.GetLength() != 0)
					theApp.emuledlg->serverwnd->UpdateServerMetFromURL(strAddress);
			}
			break;
		case CED2KLink::kServer:
			{
				CString defName;
				CED2KServerLink* pSrvLink = pLink->GetServerLink();
				_ASSERT( pSrvLink !=0 );
				in_addr host;
				host.S_un.S_addr = pSrvLink->GetIP();
				CServer* pSrv = new CServer(pSrvLink->GetPort(),inet_ntoa(host));
				_ASSERT( pSrv !=0 );
				pSrvLink->GetDefaultName(defName);
				pSrv->SetListName(defName.GetBuffer());

				// Barry - Default all new irc servers to high priority
				if( thePrefs.GetManualHighPrio() )
					pSrv->SetPreference(SRV_PR_HIGH);

				if (!theApp.emuledlg->serverwnd->serverlistctrl.AddServer(pSrv,true)) 
					delete pSrv; 
				else
					AddLogLine(true,GetResString(IDS_SERVERADDED), pSrv->GetListName());
			}
			break;
		default:
			break;
		}
		delete pLink;
	} 
	catch(...) 
	{
		AddLogLine(true, GetResString(IDS_LINKNOTADDED));
		ASSERT(0);
	}
}

void CIrcMain::ParseMessage( CString rawMessage )
{
	try
	{
		if( rawMessage.Left(6) == "PING :" )
		{
			ircsocket->SendString( "PONG " + rawMessage.Right(rawMessage.GetLength()-6) );
			m_pwndIRC->AddStatus(  "PING?/PONG" );
			return;
		}
		rawMessage.Replace( "%", "\004" );
		CString source = "";
		int sourceIndex = -1;
		CString sourceIp = "";
		CString command = "";
		int commandIndex = -1;
		CString target = "";
		int targetIndex = -1;
		CString target2 = "";
		int target2Index = -1;
		CString message = "";
		int messageIndex = -1;
	
		sourceIndex = rawMessage.Find( '!' );
		commandIndex = rawMessage.Find( ' ' );
		targetIndex = rawMessage.Find( ' ', commandIndex + 1);
		command = rawMessage.Mid( commandIndex + 1, targetIndex - commandIndex - 1);
	
		if( sourceIndex < commandIndex && sourceIndex > 0)
		{
			source = rawMessage.Mid( 1, sourceIndex - 1);
			sourceIp = rawMessage.Mid( sourceIndex + 1, commandIndex - sourceIndex - 1);
			messageIndex = rawMessage.Find( ' ', targetIndex + 1);
			if( messageIndex > sourceIndex )
			{
				target = rawMessage.Mid( targetIndex + 1, messageIndex - targetIndex - 1);
				message = rawMessage.Mid( messageIndex );
				if( target.Left(1) == ":" )
					target = target.Mid(1);
				if( message.Left(1) == ":" )
					message = message.Mid(1);
				if( target.Left(2) == " :" )
					target = target.Mid(2);
				if( message.Left(2) == " :" )
					message = message.Mid(2);
			}
			else
			{
				target = rawMessage.Mid( targetIndex + 1, rawMessage.GetLength() - targetIndex - 1);
					if( target.Left(1) == ":" )
						target = target.Mid(1);
					if( message.Left(1) == ":" )
						message = message.Mid(1);
					if( target.Left(2) == " :" )
						target = target.Mid(2);
					if( message.Left(2) == " :" )
						message = message.Mid(2);
			}
		}
		else
		{
			source = rawMessage.Mid( 1, commandIndex - 1);
			message = rawMessage.Mid( targetIndex + 1);
		}
		if( command == "PRIVMSG" )
		{
			if ( target.Left(1) == "#" )
			{
				if( message.Left(1) == "\001" )
				{
					message = message.Mid( 1, message.GetLength() - 2);
					if( message.Left(6) == "ACTION" )
					{
						m_pwndIRC->AddInfoMessage( target, "* %s%s", source, message.Mid(6) );
						return;
					}
					if( message.Left(5) == "SOUND")
					{
						message = message.Mid(6);
						int soundlen = message.Find( " " );
						CString sound;
						if( soundlen != -1 )
						{
							sound.Format("%sSounds\\IRC\\%s", thePrefs.GetAppDir(), message.Left(soundlen));
							message = message.Mid(soundlen);
						}
						else
						{
							sound.Format("%sSounds\\IRC\\%s", thePrefs.GetAppDir(), message);
							message = " [sound]";
						}
						if(thePrefs.GetIrcSoundEvents())
						{
							PlaySound(sound, NULL, SND_FILENAME | SND_NOSTOP | SND_NOWAIT | SND_ASYNC);
						}
						m_pwndIRC->AddInfoMessage( target, "* %s%s", source, message );
					}
					if( message.Left(7) == "VERSION")
					{
						version = "eMule" + theApp.m_strCurVersionLong + (CString)Irc_Version;
						CString build;
						build.Format( "NOTICE %s :\001VERSION %s\001", source, version );
						ircsocket->SendString( build );
						return;
					}
				}
				
				else
				{
	                m_pwndIRC->AddMessage( target, source, message);
					return;
				}
			}
			else
			{
				if( message.Left(1) == "\001" ){
					message = message.Mid(1, message.GetLength() -2);
					if( message.Left(6) == "ACTION")
					{
						m_pwndIRC->AddInfoMessage( source, "* %s%s", source, message.Mid(6) );
						return;
					}
					if( message.Left(5) == "SOUND")
					{
						message = message.Mid(6);
						int soundlen = message.Find( " " );
						CString sound;
						if( soundlen != -1 )
						{
							sound.Format("%sSounds\\IRC\\%s", thePrefs.GetAppDir(), message.Left(soundlen));
							message = message.Mid(soundlen);
						}
						else
						{
							sound.Format("%sSounds\\IRC\\%s", thePrefs.GetAppDir(), message);
							message = " [sound]";
						}
						if(thePrefs.GetIrcSoundEvents())
						{
							PlaySound(sound, NULL, SND_FILENAME | SND_NOSTOP | SND_NOWAIT | SND_ASYNC);
						}
						m_pwndIRC->AddInfoMessage( source, "* %s%s", source, message );
					}
					if( message.Left(7) == "VERSION")
					{
						version = "eMule" + theApp.m_strCurVersionLong + (CString)Irc_Version;
						CString build;
						build.Format( "NOTICE %s :\001VERSION %s\001", source, version );
						ircsocket->SendString( build );
					}
					if( message.Left(9) == "RQSFRIEND" )
					{
						message = message.Mid(9);
						int index1 = message.Find( "|" );
						if( index1 == -1 || index1 == message.GetLength() )
							return;
						int index2 = message.Find( "|", index1+1 );
						if( index2 == -1 || index1 > index2 )
							return;
						CString sverify = message.Mid(index1+1, index2-index1-1);
						CString sip;
						if(theApp.serverconnect->IsConnected())
							sip.Format( "%i", theApp.serverconnect->GetCurrentServer()->GetIP());
						else
							sip = "0.0.0.0";
						CString sport;
						if(theApp.serverconnect->IsConnected())
							sport.Format( "%i", theApp.serverconnect->GetCurrentServer()->GetPort());
						else
							sport = "0";
						CString build;
						build.Format( "PRIVMSG %s :\001REPFRIEND eMule%s%s|%s|%u:%u|%s:%s|%s|\001", source, theApp.m_strCurVersionLong, Irc_Version, sverify, theApp.IsFirewalled() ? 0 : theApp.GetID(), thePrefs.GetPort(), sip, sport, EncodeBase16((const unsigned char*)thePrefs.GetUserHash(), 16));
						ircsocket->SendString( build );
						build.Format( "%s %s", source, GetResString(IDS_IRC_ADDASFRIEND));
						if( !thePrefs.GetIrcIgnoreEmuleProtoAddFriend() )
							m_pwndIRC->NoticeMessage( "*EmuleProto*", build );
						return;
					}
					if( message.Left(8) == "SENDLINK" )
					{
						if ( !thePrefs.GetIrcAcceptLinks() )
						{
							if( !thePrefs.GetIrcIgnoreEmuleProtoSendLink() )
							{
								m_pwndIRC->NoticeMessage( "*EmuleProto*", source + " attempted to send you a file. If you wanted to accept the files from this person, enable Recieve files in the IRC Preferences.");
							}
							return;
						}
						message = message.Mid(8);
						int index1, index2;
						index1 = message.Find( "|" );
						if( index1 == -1 || index1 == message.GetLength() )
							return;
						index2 = message.Find( "|", index1+1 );
						if( index2 == -1 || index1 > index2 )
							return;
						CString hash = message.Mid(index1+1, index2-index1-1);
						uchar userid[16];
						if (hash.GetLength()!=32 || !DecodeBase16(hash.GetBuffer(),hash.GetLength(),userid,ARRSIZE(userid)))
							return;
						CString RecieveString, build;
						if(!theApp.friendlist->SearchFriend(userid, 0, 0))
						{
							if( thePrefs.GetIrcAcceptLinksFriends() )
							{
								if( !thePrefs.GetIrcIgnoreEmuleProtoSendLink() )
								{
									m_pwndIRC->NoticeMessage( "*EmuleProto*", source + " attempted to send you a file but wasn't a friend. If you wanted to accept files from this person, add person as a friend or disable from friends only in the IRC preferences.");
								}
								return;
							}
						}
						RecieveString = message.Mid( index2+1 );
						if( !RecieveString.IsEmpty() )
						{
							build.Format( GetResString(IDS_IRC_RECIEVEDLINK), source, RecieveString );
							if( !thePrefs.GetIrcIgnoreEmuleProtoSendLink() )
								m_pwndIRC->NoticeMessage( "*EmuleProto*", build );
							ProcessLink( RecieveString );
						}
						return;
					}
					if( message.Left(9) == "REPFRIEND" )
					{
						message = message.Mid(9);
						int index1 = message.Find( "|" );
						if( index1 == -1 || index1 == message.GetLength() )
							return;
						int index2 = message.Find( "|", index1+1 );
						if( index2 == -1 || index1 > index2 )
							return;
						CString sverify = message.Mid(index1+1, index2-index1-1);
						if( verify != atoi(sverify))
							return;
						SetVerify();
						index1 = message.Find( ":", index2+1);
						if( index1 == -1 || index2 > index1 )
							return;
						uint32 newClientID = atoi(message.Mid( index2 + 1, index1 - index2 -1));
						index2 = message.Find( '|', index1+1);
						if( index2 == -1 || index1 >= index2 )
							return;
						uint16 newClientPort = atoi(message.Mid( index1 + 1, index2 - index1 -1));
						index1 = message.Find( ':', index2+1);
						if( index1 == -1 || index2 >= index1 )
							return;
						//uint32 newClientServerIP = atoi(message.Mid( index2 + 1, index1 - index2 -1));
						index2 = message.Find( '|', index1+1);
						if( index2 == -1 || index1 >= index2 )
							return;
						//uint16 newClientServerPort = atoi(message.Mid( index1 + 1, index2 - index1 -1));
						index1 = message.Find( '|', index2+1);
						if( index1 == -1 || index2 >= index1 )
							return;
						CString hash = message.Mid( index2 + 1, index1 - index2 -1);
						uchar userid[16];
						if (hash.GetLength()!=32 || !DecodeBase16(hash.GetBuffer(),hash.GetLength(),userid,ARRSIZE(userid)))
							return;
						theApp.friendlist->AddFriend( userid, 0, newClientID, newClientPort, 0, source, 1);
					}
					return;
				}
				else
				{
					m_pwndIRC->AddMessage( source, source, message);
					return;
				}
			}
		}
		if( command == "JOIN" )
		{
			if( source == nick )
			{
				m_pwndIRC->AddInfoMessage( target, GetResString(IDS_IRC_HASJOINED), source, target );
				return;
			}
			if( !thePrefs.GetIrcIgnoreJoinMessage() )
				m_pwndIRC->AddInfoMessage( target, GetResString(IDS_IRC_HASJOINED), source, target );
			m_pwndIRC->NewNick( target, source );
			return;
	
		}
		if( command == "PART" )
		{
			if ( source == nick )
			{
				m_pwndIRC->RemoveChannel( target );
				return;
			}
			m_pwndIRC->RemoveNick( target, source );
			if( !thePrefs.GetIrcIgnorePartMessage() )
				m_pwndIRC->AddInfoMessage( target, GetResString(IDS_IRC_HASPARTED), source, target, message );
			return;
		}
	
		if( command == "TOPIC" )
		{
			m_pwndIRC->SetTitle( target, message );
			return;
		}
	
		if( command == "QUIT" )
		{
			m_pwndIRC->DeleteNickInAll( source, message );
			return;
		}
	
		if( command == "NICK" )
		{
			if( source == nick )
			{
				nick = target;
				thePrefs.SetIRCNick( nick.GetBuffer() );
			}
			m_pwndIRC->ChangeAllNick( source, target );
			return;
		}
	
		if( command == "KICK" )
		{
			target2Index = message.Find(':');
			if( target2Index > 0 ){
				target2 = message.Mid( 1, target2Index - 2);
				message = message.Mid( target2Index + 1 );
			}
			if( target2 == nick )
			{
				m_pwndIRC->RemoveChannel( target );
				m_pwndIRC->AddStatus(  GetResString(IDS_IRC_WASKICKEDBY), target2, source, message );
				return;
			}
			m_pwndIRC->RemoveNick( target, target2 );
			if( !thePrefs.GetIrcIgnoreMiscMessage() )
				m_pwndIRC->AddInfoMessage( target, GetResString(IDS_IRC_WASKICKEDBY), target2, source, message );
			return;
		}
		if( command == "MODE" )
		{
			commandIndex = message.Find( ' ', 1 );
			command = message.Mid( 1, commandIndex - 1 );
			command.Replace( "\004", "%" );
			target2 = message.Mid( commandIndex + 1 );
			m_pwndIRC->ParseChangeMode( target, source, command, target2 );
			return;
		}
		if( command == "NOTICE")
		{
			m_pwndIRC->NoticeMessage( source, message );
			return;
		}
		if( command == "001" )
		{
			m_pwndIRC->SetLoggedIn( true );
			if( thePrefs.GetIRCListOnConnect() )
				ircsocket->SendString("list");
			ParsePerform();
		}
	
		if( command == "321" )
		{
			m_pwndIRC->ResetServerChannelList();
			return;
		}
	
		if( command == "322")
		{
			CString chanName, chanNum, chanDesc;
			int chanNameIndex, chanNumIndex, chanDescIndex;
			chanNameIndex = message.Find( ' ' );
			chanNumIndex = message.Find( ' ', chanNameIndex + 1 );
			chanDescIndex = message.Find( ' ', chanNumIndex + 1);
			chanName = message.Mid( chanNameIndex + 1,  chanNumIndex - chanNameIndex - 1 );
			chanNum = message.Mid( chanNumIndex + 1, chanDescIndex - chanNumIndex - 1 );
			if( chanDescIndex > 0 )
			{
				chanDescIndex = message.Find( ' ', chanDescIndex+1 );
				if( chanDescIndex > 0)
					chanDesc = message.Mid( chanDescIndex );
				else
					chanDesc = "";
			}
			m_pwndIRC->AddChannelToList( chanName, chanNum, chanDesc );
			return;
		}
		if( command == "332" )
		{
			target2 = message.Mid( message.Find( '#' ), message.Find( ':' )-message.Find( '#' ) -1 );
			message = message.Mid( message.Find(':') + 1);
			m_pwndIRC->SetTitle( target2, message );
			m_pwndIRC->AddInfoMessage( target2, "* Channel Title: %s", message );
			return;
		}
		if( command == "353" )
		{
			int getNickChannelIndex = -1;
			CString getNickChannel;
			int getNickIndex = 1;
			CString getNick;
			int count = 0;
			VERIFY ( (getNickChannelIndex = rawMessage.Find(' ', targetIndex + 1)) != (-1) );
			getNickChannelIndex = rawMessage.Find(' ', targetIndex + 1);
			getNickIndex = rawMessage.Find( ' ', getNickChannelIndex + 3);
			getNickChannel = rawMessage.Mid( getNickChannelIndex + 2, getNickIndex - getNickChannelIndex - 2);
			getNickChannelIndex = rawMessage.Find( ':', getNickChannelIndex );
			getNickIndex = rawMessage.Find( ' ', getNickChannelIndex);
			rawMessage.Replace( "\004", "%" );
			while( getNickIndex > 0 )
			{
				count ++;
				getNick = rawMessage.Mid( getNickChannelIndex + 1, getNickIndex - getNickChannelIndex - 1);
				getNickChannelIndex = getNickIndex;
				m_pwndIRC->NewNick( getNickChannel, getNick );
				getNickIndex = rawMessage.Find( ' ', getNickChannelIndex + 1 );
			}
			return;
		}
		if( command == "433" )
		{
			if( !m_pwndIRC->GetLoggedIn() )
				Disconnect();
			m_pwndIRC->AddStatus( GetResString(IDS_IRC_NICKUSED));
			return;
		}
		m_pwndIRC->AddStatus( message );
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, "Exception in CIrcMain::ParseMessage");
	}
}

void CIrcMain::SendLogin()
{
	try
	{
		ircsocket->SendString(user);
		CString temp = "NICK " + nick;
		ircsocket->SendString(temp);
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, "Exception in CIrcMain::SendLogin");
	}
}

void CIrcMain::ParsePerform()
{
	//We need to do the perform first and seperate from the help option.
	//This allows you to do all your passwords and stuff before joining the
	//help channel and to keep both options from interfering with each other.
	try
	{
		if (thePrefs.GetIrcUsePerform())
		{
			CString strUserPerform = thePrefs.GetIrcPerformString();
			strUserPerform.Trim();
			if (!strUserPerform.IsEmpty())
			{
				int iPos = 0;
				CString str = strUserPerform.Tokenize(_T("|"), iPos);
				str.Trim();
				while (!str.IsEmpty())
				{
					if (str.Left(1) == _T('/'))
						str = str.Mid(1);
					if (str.Left(3) == _T("msg"))
						str = _T("PRIVMSG") + str.Mid(3);
					if (str.Left(16).CompareNoCase(_T("PRIVMSG nickserv")) == 0)
						str = _T("ns") + str.Mid(16);
					if (str.Left(16).CompareNoCase(_T("PRIVMSG chanserv")) == 0)
						str = _T("cs") + str.Mid(16);
					ircsocket->SendString(str);
					str = strUserPerform.Tokenize(_T("|"), iPos);
					str.Trim();
				}
			}
		}
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, "Exception in CIrcMain::ParsePerform(1)");
	}

	try
	{
		if (thePrefs.GetIrcHelpChannel())
		{
			// NOTE: putting this IRC command string into the language resource file is not a good idea. most 
			// translators do not know that this resource string does NOT have to be translated.
	
			// Well, I meant to make this option a static perform string within the language so the default help could
			// be change to what ever channel by just changing the language.. I will just have to check these strings
			// before release.
			// This also allows the help string to do more then join one channel. It could add other features later.
			CString strJoinHelpChannel = GetResString(IDS_IRC_HELPCHANNELPERFORM);
			strJoinHelpChannel.Trim();
			if (!strJoinHelpChannel.IsEmpty())
			{
				int iPos = 0;
				CString str = strJoinHelpChannel.Tokenize(_T("|"), iPos);
				str.Trim();
				while (!str.IsEmpty())
				{
					if (str.Left(1) == _T('/'))
						str = str.Mid(1);
					if (str.Left(3) == _T("msg"))
						str = _T("PRIVMSG") + str.Mid(3);
					if (str.Left(16).CompareNoCase(_T("PRIVMSG nickserv")) == 0)
						str = _T("ns") + str.Mid(16);
					if (str.Left(16).CompareNoCase(_T("PRIVMSG chanserv")) == 0)
						str = _T("cs") + str.Mid(16);
					ircsocket->SendString(str);
					str = strJoinHelpChannel.Tokenize(_T("|"), iPos);
					str.Trim();
				}
			}
		}
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, "Exception in CIrcMain::ParsePerform(2)");
	}
}
void CIrcMain::Connect()
{
	try
	{
		CString ident;
		uint16 ident_int = 0;
		for( int i = 0; i < 16; i++)
		{
			ident_int += thePrefs.GetUserHash()[i] * thePrefs.GetUserHash()[15-i];
		}
		ident.Format("e%u", ident_int);
		if( ident.GetLength() > 8 )
			ident.Truncate(8);
		ircsocket = new CIrcSocket(this);
		nick = (CString)thePrefs.GetIRCNick();
		nick.Replace(".", "");
		nick.Replace(" ", "");
		nick.Replace(":", "");
		nick.Replace("/", "");
		nick.Replace("@", "");
		if( nick.MakeLower() == "emule" )
		{
			uint16 ident_int = 0;
			for( int i = 0; i < 16; i++)
			{
				ident_int += thePrefs.GetUserHash()[i] * thePrefs.GetUserHash()[15-i];
			}
			nick.Format("eMuleIRC%u", ident_int);
		}
		nick = nick.Left(20);
		version = "eMule" + theApp.m_strCurVersionLong + (CString)Irc_Version;
		user = "USER " + ident + " 8 * :" + version;
		ircsocket->Create();
		ircsocket->Connect();
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, "Exception in CIrcMain::Connect");
	}
}

void CIrcMain::Disconnect(bool isshuttingdown)
{
	try
	{
		ircsocket->Close();
		delete ircsocket;
		ircsocket = NULL;
		if( !isshuttingdown )
			m_pwndIRC->SetConnectStatus(false);
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, "Exception in CIrcMain::Disconnect");
	}
}

void CIrcMain::SetConnectStatus( bool connected )
{
	try
	{
		m_pwndIRC->SetConnectStatus( connected );
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, "Exception in CIrcMain::SetConnectStatus");
	}
}

int CIrcMain::SendString( CString send )
{
	return ircsocket->SendString(send);
}