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

//A lot of documentation for the IRC protocol within this code came
//directly from http://www.irchelp.org/irchelp/rfc/rfc2812.txt
//Much of it may never be used, but it's here just in case..

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
#include "emuledlg.h"
#include "ServerWnd.h"
#include "IRCWnd.h"

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

void CIrcMain::PreParseMessage( CStringA buffer )
{
	try
	{
		CStringA rawMessage;
		preParseBuffer += buffer;
		int index = preParseBuffer.Find('\n');
		try
		{
			while( index != -1 )
			{
				rawMessage = preParseBuffer.Left(index);
				rawMessage.Remove('\n');
				rawMessage.Remove('\r');
				ParseMessage( rawMessage );
				preParseBuffer = preParseBuffer.Mid(index+1);
				index = preParseBuffer.Find("\n");
			}
		}
		catch(...)
		{
			if (thePrefs.GetVerbose())
				AddDebugLogLine(false, _T("Exception in CIrcMain::PreParseMessage(1)"));
		}
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Exception in CIrcMain::PreParseMessage(2)"));
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
		switch (pLink->GetKind()) 
		{
			case CED2KLink::kFile:
			{
				//MORPH START - Changed by SiRoB, Selection category support khaos::categorymod+
				/*
				CED2KFileLink* pFileLink = pLink->GetFileLink();
				_ASSERT(pFileLink !=0);
				theApp.downloadqueue->AddFileLinkToDownload(pFileLink);
				/*/
				CED2KFileLink* pFileLink = (CED2KFileLink*)CED2KLink::CreateLinkFromUrl(link);
				theApp.downloadqueue->AddFileLinkToDownload(pFileLink, -1, true);
				/**/
				//MORPH END   - Changed by SiRoB, Selection category support khaos::categorymod-
				break;
			}
			case CED2KLink::kServerList:
			{
				CED2KServerListLink* pListLink = pLink->GetServerListLink(); 
				_ASSERT( pListLink !=0 ); 
				CString strAddress = pListLink->GetAddress(); 
				if(strAddress.GetLength() != 0)
					theApp.emuledlg->serverwnd->UpdateServerMetFromURL(strAddress);
				break;
			}
			case CED2KLink::kServer:
			{
				CString defName;
				CED2KServerLink* pSrvLink = pLink->GetServerLink();
				_ASSERT( pSrvLink !=0 );
				CServer* pSrv = new CServer(pSrvLink->GetPort(),ipstr(pSrvLink->GetIP()));
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
				break;
			}
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

void CIrcMain::ParseMessage( CStringA rawMessage )
{
	try
	{
		if( rawMessage.GetLength() < 6 )
		{
			//TODO : We probably should disconnect here as I don't know of anything that should
			//come from the server this small..
			return;
		}
		if( rawMessage.Left(6) == "PING :" )
		{
			//If the server pinged us, we must pong back or get disconnected..
			//Anything after the ":" must be sent back or it will fail..
			ircsocket->SendString( CString("PONG " + rawMessage.Right(rawMessage.GetLength()-6)) );
			m_pwndIRC->AddStatus(  _T("PING?/PONG") );
			return;
		}
		//I temp replace % with \004 as it will mess up sending this string with parameters later.
		rawMessage.Replace( "%", "\004" );
		//Source of the message
		CString source;
		int sourceIndex = -1;
		//IP address of the source of the message
		CString sourceIp;
		//Command that the message is sending
		CString command;
		int commandIndex = -1;
		//Target of the message
		CString target;
		int targetIndex = -1;
		CString target2;
		int target2Index = -1;
		//The actual message
		CString message;
		int messageIndex = -1;
		//Check if there is a source which always ends with a !
		//Messages without a source is usually a server message.
		sourceIndex = rawMessage.Find( '!' );
		if( rawMessage.Left(1) == ":" )
		{
			//If the message starts with a ":", the first space must be the beginning of the command
			//Although, it seems some servers are miscofigured..
			commandIndex = rawMessage.Find( ' ' );
		}
		else
		{
			//If the message doesn't start with a ":", the first word is the command.
			commandIndex = 0;
		}
		if( commandIndex == -1 )
			throw CString(_T("SMIRC Error: Received a message had no command."));
		//The second space is the beginning of the target or message
		targetIndex = rawMessage.Find( ' ', commandIndex + 1);
		if( targetIndex == -1 )
			throw CString(_T("SMIRC Errow: Received a message with no target or message."));
		//This will pull out the command
		if( commandIndex == 0 )
		{
			//Command is the first word.. Strange as I don't see this in any standard..
			command = rawMessage.Mid( commandIndex, targetIndex - commandIndex);
		}
		else
		{
			//Command is where it should be.
			command = rawMessage.Mid( commandIndex + 1, targetIndex - commandIndex - 1);
		}
		if( sourceIndex < commandIndex && sourceIndex > 0)
		{
			//Raw message had a source.

			//Get source and IP of source
			source = rawMessage.Mid( 1, sourceIndex - 1);
			sourceIp = rawMessage.Mid( sourceIndex + 1, commandIndex - sourceIndex - 1);
			messageIndex = rawMessage.Find( ' ', targetIndex + 1);
			if( messageIndex > sourceIndex )
			{
				//Raw message had a message

				//Get target and message
				target = rawMessage.Mid( targetIndex + 1, messageIndex - targetIndex - 1);
				message = rawMessage.Mid( messageIndex );
				//I'm not sure why some messages have different formats, but this cleans them up.
				if( target.Left(1) == _T(":") )
					target = target.Mid(1);
				if( message.Left(1) == _T(":") )
					message = message.Mid(1);
				if( target.Left(2) == _T(" :") )
					target = target.Mid(2);
				if( message.Left(2) == _T(" :") )
					message = message.Mid(2);
			}
			else
			{
				//Raw message had no message

				//Get target
				target = rawMessage.Mid( targetIndex + 1, rawMessage.GetLength() - targetIndex - 1);
				//I'm not sure why some messages have different formats, but this cleans them up.
				if( target.Left(1) == _T(":") )
					target = target.Mid(1);
				if( target.Left(2) == _T(" :") )
					target = target.Mid(2);
			}
		}
		else
		{
			if( commandIndex == 0 )
			{
				//Raw message had no source, must be from the server
				source = _T("");
			}
			else
			{
				//Get source
				source = rawMessage.Mid( 1, commandIndex - 1);
			}
			message = rawMessage.Mid( targetIndex + 1);
		}

		if( command == _T("PRIVMSG") )
		{
			// Messages
			if ( target.Left(1) == _T("#") )
			{
				//Belongs to a channel..
				if( message.Left(1) == _T("\001") )
				{
					//This is a specal message.. Find out what kind..
					if( message.GetLength() < 4 )
						throw CString( _T("SMIRC Error: Received Invalid special channel message") );
					message = message.Mid( 1, message.GetLength() - 2);
					if( message.Left(6) == _T("ACTION") )
					{
						//Channel Action..
						if( message.GetLength() < 8 )
							throw CString( _T("SMIRC Error: Received Invalid channel action") );
						m_pwndIRC->AddInfoMessage( target, _T("* %s %s"), source, message.Mid(7) );
						return;
					}
					if( message.Left(5) == _T("SOUND"))
					{
						//Sound
						if( message.GetLength() < 7 )
							throw CString( _T("SMIRC Error: Received Invalid channel sound") );
						message = message.Mid(6);
						int soundlen = message.Find( _T(" ") );
						CString sound;
						CString strFileName;
						if( soundlen != -1 )
						{
							//Sound event also had a message
							strFileName = message.Left(soundlen);
							message = message.Mid(soundlen);
						}
						else
						{
							//Sound event didn't have a message
							strFileName = message;
							message = _T(" [SOUND]");
						}
						//Check for proper form.
						strFileName.Remove(_T('\\'));
						strFileName.Remove(_T('/'));
						if( (strFileName.Right(4) != _T(".wav")) && (strFileName.Right(4) != _T(".mp3")) && (strFileName.Right(4) != _T(".WAV")) && (strFileName.Right(4) != _T(".MP3")) )
							throw CString( _T("SMIRC Error: Received invalid sound event.") );
						sound.Format(_T("%sSounds\\IRC\\%s"), thePrefs.GetAppDir(), strFileName);
						if(thePrefs.GetIrcSoundEvents())
						{
							PlaySound(sound, NULL, SND_FILENAME | SND_NOSTOP | SND_NOWAIT | SND_ASYNC);
						}
						m_pwndIRC->AddInfoMessage( target, _T("* %s%s"), source, message );
						return;
					}
					if( message.Left(7) == _T("VERSION"))
					{
						//Get client version.
						version = _T("eMule") + theApp.m_strCurVersionLong + _T(Irc_Version);
						CString build;
						build.Format( _T("NOTICE %s :\001VERSION %s\001"), source, version );
						ircsocket->SendString( build );
						return;
					}
					if( message.Left(4) == _T("PING"))
					{
						//TODO - Respond to a user ping.
						return;
					}
				}
				else
				{
					//This is a normal channel message.. 
	                m_pwndIRC->AddMessage( target, source, message);
					return;
				}
			}
			else
			{
				//Private Message
				if( message.Left(1) == _T("\001") ){
					//Special message
					if( message.GetLength() < 4 )
						throw CString( _T("SMIRC Error: Received Invalid special private message") );
					message = message.Mid(1, message.GetLength() -2);
					if( message.Left(6) == _T("ACTION"))
					{
						//Action
						if( message.GetLength() < 8 )
							throw CString( _T("SMIRC Error: Received Invalid private action") );
						m_pwndIRC->AddInfoMessage( source, _T("* %s %s"), source, message.Mid(7) );
						return;
					}
					if( message.Left(5) == _T("SOUND"))
					{
						//sound event
						if( message.GetLength() < 7 )
							throw CString( _T("SMIRC Error: Received Invalid private sound") );
						message = message.Mid(6);
						int soundlen = message.Find( _T(" ") );
						CString sound;
						CString strFileName;
						if( soundlen != -1 )
						{
							//Sound event also had a message
							strFileName = message.Left(soundlen);
							message = message.Mid(soundlen);
						}
						else
						{
							//Sound event didn't have a message
							strFileName = message;
							message = _T(" [sound]");
						}
						//Check for proper form.
						strFileName.Remove(_T('\\'));
						strFileName.Remove(_T('/'));
						if( (strFileName.Right(4) != _T(".wav")) && (strFileName.Right(4) != _T(".mp3")) && (strFileName.Right(4) != _T(".WAV")) && (strFileName.Right(4) != _T(".MP3")) )
							throw CString( _T("SMIRC Error: Received invalid sound event") );
						sound.Format(_T("%sSounds\\IRC\\%s"), thePrefs.GetAppDir(), strFileName);
						if(thePrefs.GetIrcSoundEvents())
						{
							PlaySound(sound, NULL, SND_FILENAME | SND_NOSTOP | SND_NOWAIT | SND_ASYNC);
						}
						m_pwndIRC->AddInfoMessage( source, _T("* %s%s"), source, message );
						return;
					}
					if( message.Left(7) == _T("VERSION"))
					{
						//Get client version.
						version = _T("eMule") + theApp.m_strCurVersionLong + _T(Irc_Version);
						CString build;
						build.Format( _T("NOTICE %s :\001VERSION %s\001"), source, version );
						ircsocket->SendString( build );
						return;
					}
					if( message.Left(4) == _T("PING"))
					{
						//TODO - Respond to a user ping.
						return;
					}
					if( message.Left(9) == _T("RQSFRIEND") )
					{
						//eMule user requested to add you as friend.
						if( !thePrefs.GetIrcAllowEmuleProtoAddFriend() )
							return;
						if( message.GetLength() < 10 )
							throw CString( _T("SMIRC Error: Received Invalid friend request") );
						message = message.Mid(9);
						int index1 = message.Find( _T("|") );
						if( index1 == -1 || index1 == message.GetLength() )
							throw CString( _T("SMIRC Error: Received Invalid friend request") );
						int index2 = message.Find( _T("|"), index1+1 );
						if( index2 == -1 || index1 > index2 )
							throw CString( _T("SMIRC Error: Received Invalid friend request") );
						//Adds a little protection
						CString sverify = message.Mid(index1+1, index2-index1-1);
						CString sip;
						CString sport;
						if(theApp.serverconnect->IsConnected())
						{
							//Tell them what server we are on for possibel lowID support later.
							sip.Format( _T("%i"), theApp.serverconnect->GetCurrentServer()->GetIP());
							sport.Format( _T("%i"), theApp.serverconnect->GetCurrentServer()->GetPort());
						}
						else
						{
							//Not on a server.
							sip = _T("0.0.0.0");
							sport = _T("0");
						}
						//Create our response.
						CString build;
						build.Format( _T("PRIVMSG %s :\001REPFRIEND eMule%s%s|%s|%u:%u|%s:%s|%s|\001"), source, theApp.m_strCurVersionLong, Irc_Version, sverify, theApp.IsFirewalled() ? 0 : theApp.GetID(), thePrefs.GetPort(), sip, sport, EncodeBase16((const unsigned char*)thePrefs.GetUserHash(), 16));
						ircsocket->SendString( build );
						build.Format( _T("%s %s"), source, GetResString(IDS_IRC_ADDASFRIEND));
						if( !thePrefs.GetIrcIgnoreEmuleProtoAddFriend() )
							m_pwndIRC->NoticeMessage( _T("*EmuleProto*"), build );
						return;
					}
					if( message.Left(9) == _T("REPFRIEND") )
					{
						if( message.GetLength() < 10 )
							throw CString( _T("SMIRC Error: Received Invalid friend reply") );
						message = message.Mid(9);
						int index1 = message.Find( _T("|") );
						if( index1 == -1 || index1 == message.GetLength() )
							throw CString( _T("SMIRC Error: Received Invalid friend reply") );
						int index2 = message.Find( _T("|"), index1+1 );
						if( index2 == -1 || index1 > index2 )
							throw CString( _T("SMIRC Error: Received Invalid friend reply") );
						CString sverify = message.Mid(index1+1, index2-index1-1);
						//Make sure we requested this!
						if( verify != _tstoi(sverify))
							return;
						//Pick a new random verify.
						SetVerify();
						index1 = message.Find( _T(":"), index2+1);
						if( index1 == -1 || index2 > index1 )
							throw CString( _T("SMIRC Error: Received Invalid friend reply") );
						uint32 newClientID = _tstoi(message.Mid( index2 + 1, index1 - index2 -1));
						index2 = message.Find( _T('|'), index1+1);
						if( index2 == -1 || index1 >= index2 )
							throw CString( _T("SMIRC Error: Received Invalid friend reply") );
						uint16 newClientPort = _tstoi(message.Mid( index1 + 1, index2 - index1 -1));
						index1 = message.Find( _T(':'), index2+1);
						if( index1 == -1 || index2 >= index1 )
							throw CString( _T("SMIRC Error: Received Invalid friend reply") );
						//This is if we decide to support lowID clients.
						//uint32 newClientServerIP = _tstoi(message.Mid( index2 + 1, index1 - index2 -1));
						index2 = message.Find( _T('|'), index1+1);
						if( index2 == -1 || index1 >= index2 )
							throw CString( _T("SMIRC Error: Received Invalid friend reply") );
						//This is if we decide to support lowID clients.
						//uint16 newClientServerPort = _tstoi(message.Mid( index1 + 1, index2 - index1 -1));
						index1 = message.Find( _T('|'), index2+1);
						if( index1 == -1 || index2 >= index1 )
							throw CString( _T("SMIRC Error: Received Invalid friend reply") );
						CString hash(message.Mid( index2 + 1, index1 - index2 -1));
						uchar userid[16];
						if (hash.GetLength()!=32 || !DecodeBase16(hash.GetBuffer(),hash.GetLength(),userid,ARRSIZE(userid)))
							throw CString( _T("SMIRC Error: Received Invalid friend reply") );
						theApp.friendlist->AddFriend( userid, 0, newClientID, newClientPort, 0, source, 1);
						return;
					}
					if( message.Left(8) == _T("SENDLINK") )
					{
						//Received a ED2K link from someone.
						if ( !thePrefs.GetIrcAcceptLinks() )
						{
							if( !thePrefs.GetIrcIgnoreEmuleProtoSendLink() )
							{
								m_pwndIRC->NoticeMessage( _T("*EmuleProto*"), source + _T(" attempted to send you a file. If you wanted to accept the files from this person, enable Receive files in the IRC Preferences."));
							}
							return;
						}
						if( message.GetLength() < 9 )
							throw CString( _T("SMIRC Error: Received Invalid ED2K link") );
						message = message.Mid(8);
						int index1, index2;
						index1 = message.Find( _T("|") );
						if( index1 == -1 || index1 == message.GetLength() )
							throw CString( _T("SMIRC Error: Received Invalid ED2K link") );
						index2 = message.Find( _T("|"), index1+1 );
						if( index2 == -1 || index1 > index2 )
							throw CString( _T("SMIRC Error: Received Invalid ED2K link") );
						CString hash = message.Mid(index1+1, index2-index1-1);
						uchar userid[16];
						if (hash.GetLength()!=32 || !DecodeBase16(hash.GetBuffer(),hash.GetLength(),userid,ARRSIZE(userid)))
							throw CString( _T("SMIRC Error: Received Invalid ED2K link") );
						if(!theApp.friendlist->SearchFriend(userid, 0, 0))
						{
							//This wasn't a friend..
							if( thePrefs.GetIrcAcceptLinksFriends() )
							{
								//We don't accept from non-friends.
								if( !thePrefs.GetIrcIgnoreEmuleProtoSendLink() )
								{
									m_pwndIRC->NoticeMessage( _T("*EmuleProto*"), source + _T(" attempted to send you a file but wasn't a friend. If you wanted to accept files from this person, add person as a friend or disable from friends only in the IRC preferences."));
								}
								return;
							}
						}
						message = message.Mid( index2+1 );
						if( !message.IsEmpty() )
						{
							CString build;
							build.Format( GetResString(IDS_IRC_RECIEVEDLINK), source, message );
							if( !thePrefs.GetIrcIgnoreEmuleProtoSendLink() )
								m_pwndIRC->NoticeMessage( _T("*EmuleProto*"), build );
							ProcessLink( message );
						}
						return;
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
		if( command == _T("JOIN") )
		{
			//Channel join
			if( source == nick )
			{
				//This was you.. So for it to add a new channel..
				m_pwndIRC->AddInfoMessage( target, GetResString(IDS_IRC_HASJOINED), source, target );
				return;
			}
			if( !thePrefs.GetIrcIgnoreJoinMessage() )
				m_pwndIRC->AddInfoMessage( target, GetResString(IDS_IRC_HASJOINED), source, target );
			//Add new nick to your channel.
			m_pwndIRC->m_nicklist.NewNick( target, source );
			return;
	
		}
		if( command == _T("PART") )
		{
			//Part message
			if ( source == nick )
			{
				//This was you, so remove channel.
				m_pwndIRC->m_channelselect.RemoveChannel( target );
				return;
			}
			if( !thePrefs.GetIrcIgnorePartMessage() )
				m_pwndIRC->AddInfoMessage( target, GetResString(IDS_IRC_HASPARTED), source, target, message );
			//Remove nick from your channel.
			m_pwndIRC->m_nicklist.RemoveNick( target, source );
			return;
		}
	
		if( command == _T("TOPIC") )
		{
			//Topic was set, update it.
			m_pwndIRC->SetTitle( target, message );
			return;
		}
	
		if( command == _T("QUIT") )
		{
			//This user left the network.. Remove from all Channels..
			m_pwndIRC->m_nicklist.DeleteNickInAll( source, message );
			return;
		}
	
		if( command == _T("NICK") )
		{
			//Someone changed a nick..
			if( source == nick )
			{
				//It was you.. Update!
				nick = target;
				thePrefs.SetIRCNick( nick.GetBuffer() );
			}
			//UPdate new nick in all channles..
			m_pwndIRC->m_nicklist.ChangeAllNick( source, target );
			return;
		}
	
		if( command == _T("KICK") )
		{
			//Someone was kicked from a channel..
			target2Index = message.Find(_T(':'));
			if( target2Index > 0 ){
				target2 = message.Mid( 1, target2Index - 2);
				message = message.Mid( target2Index + 1 );
			}
			if( target2 == nick )
			{
				//It was you!
				m_pwndIRC->m_channelselect.RemoveChannel( target );
				m_pwndIRC->AddStatus(  GetResString(IDS_IRC_WASKICKEDBY), target2, source, message );
				return;
			}
			if( !thePrefs.GetIrcIgnoreMiscMessage() )
				m_pwndIRC->AddInfoMessage( target, GetResString(IDS_IRC_WASKICKEDBY), target2, source, message );
			//Remove nick from your channel.
			m_pwndIRC->m_nicklist.RemoveNick( target, target2 );
			return;
		}
		if( command == _T("MODE") )
		{
			if( target != _T("") )
			{
				//A mode was set..
				commandIndex = message.Find( _T(' '), 1 );
				if( commandIndex < 2 )
					throw CString( _T("SMIRC Error: Received Invalid Mode change.") );
				command = message.Mid( 1, commandIndex - 1 );
				command.Replace( _T("\004"), _T("%") );
				target2 = message.Mid( commandIndex + 1 );
				m_pwndIRC->ParseChangeMode( target, source, command, target2 );
				return;
			}
			else
			{
				//The server just set a server user mode that relates to no channels!
				//Atm, we do not handle these modes.
			}
		}
		if( command == _T("NOTICE"))
		{
			//Receive notive..
			m_pwndIRC->NoticeMessage( source, message );
			return;
		}

		//TODO: Double and trible check this.. I don't know of any 3 letter commands
		//So I'm currently assuming it's a numberical command..
		if( command.GetLength() == 3 )
		{
			messageIndex = message.Find(_T(" "));
			if( messageIndex == -1 )
				throw CString(_T("SMIRC Error: received [") + command + _T("] command with invalid messageIndex"));
			message = message.Mid( messageIndex + 1);
			//The first line of each command appears to start with a ":"
			if( message.Left(1) == _T(":") )
			{
				m_pwndIRC->AddStatus( _T("") );
				message = message.Mid(1);
			}
			uint16 intCommand = _tstoi(command);
			switch(intCommand)
			{
				//- The server sends Replies 001 to 004 to a user upon
				//successful registration.				
				//001    RPL_WELCOME
				//"Welcome to the Internet Relay Network
				//<nick>!<user>@<host>"
				//002    RPL_YOURHOST
				//"Your host is <servername>, running version <ver>"
				//003    RPL_CREATED
				//"This server was created <date>"
				//004    RPL_MYINFO
				//"<servername> <version> <available user modes>
				//<available channel modes>"
				case 1:
				{
					m_pwndIRC->SetLoggedIn( true );
					if( thePrefs.GetIRCListOnConnect() )
						ircsocket->SendString(_T("list"));
					ParsePerform();
					m_pwndIRC->AddStatus( message );
					return;
				}
				case 2:
				case 3:
				case 4:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- Sent by the server to a user to suggest an alternative
				//server.  This is often used when the connection is
				//refused because the server is already full.
                //005    RPL_BOUNCE
				//"Try server <server name>, port <port number>"

				//005 is actually confusing.. Different sites say different things?
				//It appears this is also RPL_ISUPPORT which tells you what modes are
				//availabile to this server..
				case 5:
				{
					//This get our viable user modes
					int prefixStart = message.Find(_T("PREFIX=(")) ;
					if( prefixStart != -1 )
					{
						int prefixEnd = message.Find(_T(")"), prefixStart );
						if( prefixEnd != -1 )
						{
							//Set our channel modes actions
							m_pwndIRC->m_nicklist.m_sUserModeSettings = message.Mid( prefixStart+8, prefixEnd-prefixStart-8 );
							m_pwndIRC->m_nicklist.m_sUserModeSettings.Replace( _T("\004"), _T("%") );
							//Set our channel modes symbols
							m_pwndIRC->m_nicklist.m_sUserModeSymbols = message.Mid( prefixEnd+1, prefixEnd-prefixStart-8);
							m_pwndIRC->m_nicklist.m_sUserModeSymbols.Replace( _T("\004"), _T("%") );

						}
					}
					int chanmodesStart = message.Find(_T("CHANMODES=")) ;
					int chanmodesEnd = -1;
					if( chanmodesStart != -1 )
					{
						chanmodesEnd = message.Find(_T(","), chanmodesStart );
						if( chanmodesEnd != -1 )
						{
							chanmodesStart += 10;
							//Mode that adds or removes a nick or address to a list. Always has a parameter.
							//Modes of type A return the list when there is no parameter present.
							m_pwndIRC->m_channelselect.m_sChannelModeSettingsTypeA = message.Mid( chanmodesStart, chanmodesEnd-chanmodesStart );
							m_pwndIRC->m_channelselect.m_sChannelModeSettingsTypeA.Replace( _T("\004"), _T("%") );
						}
					}
					chanmodesStart = chanmodesEnd+1;
					if( chanmodesStart != -1 )
					{
						chanmodesEnd = message.Find(_T(","), chanmodesStart );
						if( chanmodesEnd != -1 )
						{
							//Mode that changes a setting and always has a parameter.
							m_pwndIRC->m_channelselect.m_sChannelModeSettingsTypeB = message.Mid( chanmodesStart, chanmodesEnd-chanmodesStart );
							m_pwndIRC->m_channelselect.m_sChannelModeSettingsTypeB.Replace( _T("\004"), _T("%") );
						}
					}
					chanmodesStart = chanmodesEnd+1;
					if( chanmodesStart != -1 )
					{
						chanmodesEnd = message.Find(_T(","), chanmodesStart );
						if( chanmodesEnd != -1 )
						{
							//Mode that changes a setting and only has a parameter when set.
							m_pwndIRC->m_channelselect.m_sChannelModeSettingsTypeC = message.Mid( chanmodesStart, chanmodesEnd-chanmodesStart );
							m_pwndIRC->m_channelselect.m_sChannelModeSettingsTypeC.Replace( _T("\004"), _T("%") );
						}
					}
					chanmodesStart = chanmodesEnd+1;
					if( chanmodesStart != -1 )
					{
						chanmodesEnd = message.Find(_T(" "), chanmodesStart );
						if( chanmodesEnd != -1 )
						{
							//Mode that changes a setting and never has a parameter.
							m_pwndIRC->m_channelselect.m_sChannelModeSettingsTypeD = message.Mid( chanmodesStart, chanmodesEnd-chanmodesStart );
							m_pwndIRC->m_channelselect.m_sChannelModeSettingsTypeD.Replace( _T("\004"), _T("%") );
						}
					}
					m_pwndIRC->AddStatus( message );
					return;
				}

                //- Reply format used by USERHOST to list replies to
                //the query list.  The reply string is composed as
				//follows:
				//
				//reply = nickname [ "*" ] "=" ( "+" / "-" ) hostname
				//
				//The '*' indicates whether the client has registered
				//as an Operator.  The '-' or '+' characters represent
				//whether the client has set an AWAY message or not
                //respectively.
				//302    RPL_USERHOST
				//":*1<reply> *( " " <reply> )"
				case 302:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- Reply format used by ISON to list replies to the
				//query list.
					
				//303    RPL_ISON
				//":*1<nick> *( " " <nick> )"
				case 303:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- These replies are used with the AWAY command (if
				//allowed).  RPL_AWAY is sent to any client sending a
				//PRIVMSG to a client which is away.  RPL_AWAY is only
				//sent by the server to which the client is connected.
				//Replies RPL_UNAWAY and RPL_NOWAWAY are sent when the
				//client removes and sets an AWAY message.		
				//301    RPL_AWAY
				//"<nick> :<away message>"
				//305    RPL_UNAWAY
				//":You are no longer marked as being away"
				//306    RPL_NOWAWAY
				//":You have been marked as being away"
				case 301:
				case 305:
				case 306:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Replies 311 - 313, 317 - 319 are all replies
                //generated in response to a WHOIS message.  Given that
				//there are enough parameters present, the answering
				//server MUST either formulate a reply out of the above
				//numerics (if the query nick is found) or return an
				//error reply.  The '*' in RPL_WHOISUSER is there as
				//the literal character and not as a wild card.  For
				//each reply set, only RPL_WHOISCHANNELS may appear
				//more than once (for long lists of channel names).
				//The '@' and '+' characters next to the channel name
				//indicate whether a client is a channel operator or
				//has been granted permission to speak on a moderated
				//channel.  The RPL_ENDOFWHOIS reply is used to mark
				//the end of processing a WHOIS message.
				//311    RPL_WHOISUSER
				//"<nick> <user> <host> * :<real name>"
				//312    RPL_WHOISSERVER
				//"<nick> <server> :<server info>"
				//313    RPL_WHOISOPERATOR
				//"<nick> :is an IRC operator"
				//317    RPL_WHOISIDLE
				//"<nick> <integer> :seconds idle"
				//318    RPL_ENDOFWHOIS
				//"<nick> :End of WHOIS list"
				//319    RPL_WHOISCHANNELS
				//"<nick> :*( ( "@" / "+" ) <channel> " " )"
				case 311:
				case 312:
				case 313:
				case 317:
				case 318:
				case 319:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- When replying to a WHOWAS message, a server MUST use
				//the replies RPL_WHOWASUSER, RPL_WHOISSERVER or
				//ERR_WASNOSUCHNICK for each nickname in the presented
				//list.  At the end of all reply batches, there MUST
				//be RPL_ENDOFWHOWAS (even if there was only one reply
				//and it was an error).
				//314    RPL_WHOWASUSER
				//"<nick> <user> <host> * :<real name>"
				//369    RPL_ENDOFWHOWAS
				//"<nick> :End of WHOWAS"
				case 314:
				case 369:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- Replies RPL_LIST, RPL_LISTEND mark the actual replies
				//with data and end of the server's response to a LIST
				//command.  If there are no channels available to return,
				//only the end reply MUST be sent.
				//321    RPL_LISTSTART
				//Obsolete. Not used.
                //322    RPL_LIST
				//"<channel> <# visible> :<topic>"
				//323    RPL_LISTEND
				//":End of LIST"
				case 321:
				{
					//Although it says this is obsolete, so far every server has sent it, so I use it.. :/
					m_pwndIRC->AddStatus( _T("Start of /LIST") );
					m_pwndIRC->m_serverChannelList.ResetServerChannelList();
					return;
				}
				case 322:
				{
					CString chanName, chanNum, chanDesc;
					int chanNumIndex, chanDescIndex;
					chanNumIndex = message.Find( _T(' ') );
					chanDescIndex = message.Find( _T(' '), chanNumIndex + 1);
					chanName = message.Left( chanNumIndex );
					chanNum = message.Mid( chanNumIndex + 1, chanDescIndex - chanNumIndex - 1 );
					if( chanDescIndex > 0 )
						chanDesc = message.Mid( chanDescIndex );
					else
						chanDesc = _T("");

					m_pwndIRC->m_serverChannelList.AddChannelToList( chanName, chanNum, chanDesc );
					return;
				}
				case 323:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- When sending a TOPIC message to determine the
				//channel topic, one of two replies is sent.  If
				//the topic is set, RPL_TOPIC is sent back else
				//RPL_NOTOPIC.
				//325    RPL_UNIQOPIS
				//"<channel> <nickname>"
				//324    RPL_CHANNELMODEIS
				//"<channel> <mode> <mode params>"
				//331    RPL_NOTOPIC
				//"<channel> :No topic is set"
				//332    RPL_TOPIC
				//"<channel> :<topic>"
				case 325:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				case 324:
				{
					//A mode was set..
					targetIndex = message.Find(_T(' '));
					if( targetIndex == -1 )
						throw CString( _T("SMIRC Error: Received Invalid Channel Mode (324).") );
					target = message.Mid(0, targetIndex);
					commandIndex = message.Find( _T(' '), targetIndex+1 );
					if( commandIndex == -1 )
						throw CString( _T("SMIRC Error: Received Invalid Channel Mode (324).") );
					command = message.Mid( targetIndex+1, commandIndex - targetIndex-1);
					command.Replace( _T("\004"), _T("%") );
					target2 = message.Mid( commandIndex + 1 );
					m_pwndIRC->ParseChangeMode( target, source, command, target2 );
					return;
				}
				case 331:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				case 332:
				{
					m_pwndIRC->AddStatus( message );
					target2Index = message.Find( _T(':') );
					if( target2Index == -1 )
						throw CString(_T("SMIRC Error: Received [322] with invalid new topic message"));
					target2 = message.Left( target2Index-1 );
					message = message.Mid( target2Index + 1);
					m_pwndIRC->SetTitle( target2, message );
					m_pwndIRC->AddInfoMessage( target2, _T("* Channel Title: %s"), message );
					return;
				}

				//- Returned by the server to indicate that the
				//attempted INVITE message was successful and is
				//being passed onto the end client.
				//341    RPL_INVITING
				//"<channel> <nick>"
				case 341:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- Returned by a server answering a SUMMON message to
				//indicate that it is summoning that user.
				//342    RPL_SUMMONING
				//"<user> :Summoning user to IRC"
				case 342:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- When listing the 'invitations masks' for a given channel,
				//a server is required to send the list back using the
				//RPL_INVITELIST and RPL_ENDOFINVITELIST messages.  A
				//separate RPL_INVITELIST is sent for each active mask.
				//After the masks have been listed (or if none present) a
				//RPL_ENDOFINVITELIST MUST be sent.
				//346    RPL_INVITELIST
				//"<channel> <invitemask>"
				//347    RPL_ENDOFINVITELIST
				//"<channel> :End of channel invite list
				case 346:
				case 347:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- When listing the 'exception masks' for a given channel,
				//a server is required to send the list back using the
				//RPL_EXCEPTLIST and RPL_ENDOFEXCEPTLIST messages.  A
				//separate RPL_EXCEPTLIST is sent for each active mask.
				//After the masks have been listed (or if none present)
				//a RPL_ENDOFEXCEPTLIST MUST be sent.
				//348    RPL_EXCEPTLIST
				//"<channel> <exceptionmask>"
				//349    RPL_ENDOFEXCEPTLIST
				//"<channel> :End of channel exception list"
				case 348:
				case 349:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- Reply by the server showing its version details.
				//The <version> is the version of the software being
				//used (including any patchlevel revisions) and the
				//<debuglevel> is used to indicate if the server is
				//running in "debug mode".
				//The "comments" field may contain any comments about
				//the version or further version details.
				//351    RPL_VERSION
				//"<version>.<debuglevel> <server> :<comments>"
				case 351:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- The RPL_WHOREPLY and RPL_ENDOFWHO pair are used
				//to answer a WHO message.  The RPL_WHOREPLY is only
				//sent if there is an appropriate match to the WHO
				//query.  If there is a list of parameters supplied
				//with a WHO message, a RPL_ENDOFWHO MUST be sent
				//after processing each list item with <name> being
				//the item.
				//352    RPL_WHOREPLY
				//"<channel> <user> <host> <server> <nick>
				//( "H" / "G" > ["*"] [ ( "@" / "+" ) ]
				//:<hopcount> <real name>"
				//315    RPL_ENDOFWHO
				//"<name> :End of WHO list"
				case 352:
				case 315:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- To reply to a NAMES message, a reply pair consisting
				//of RPL_NAMREPLY and RPL_ENDOFNAMES is sent by the
				//server back to the client.  If there is no channel
				//found as in the query, then only RPL_ENDOFNAMES is
				//returned.  The exception to this is when a NAMES
				//message is sent with no parameters and all visible
				//channels and contents are sent back in a series of
				//RPL_NAMEREPLY messages with a RPL_ENDOFNAMES to mark
				//the end.
				//353    RPL_NAMREPLY
				//"( "=" / "*" / "@" ) <channel>
				//:[ "@" / "+" ] <nick> *( " " [ "@" / "+" ] <nick> )
				//- "@" is used for secret channels, "*" for private
				//channels, and "=" for others (public channels).
				//366    RPL_ENDOFNAMES
				//"<channel> :End of NAMES list"
				case 353:
				{
					m_pwndIRC->m_nicklist.ShowWindow(SW_HIDE);;
					int getNickIndex1 = -1;
					CString getNickChannel;
					int getNickIndex = 1;
					CString getNick;
					int count = 0;

					getNickIndex1 = message.Find(_T(':'));
					if( getNickIndex1 < 4 )
						throw CString( _T("SMIRC Error: Received [353 command with misformated channel name list") );
					getNickChannel = message.Mid( 2, getNickIndex1 - 3 );

					m_pwndIRC->AddStatus( _T("") );
					m_pwndIRC->AddStatus( getNickChannel + _T(" ") + message.Mid( getNickIndex1 ));

					getNickIndex = message.Find( _T(' '), getNickIndex1);
					message.Replace( _T("\004"), _T("%") );
					while( getNickIndex > 0 )
					{
						count ++;
						getNick = message.Mid( getNickIndex1 + 1, getNickIndex - getNickIndex1 - 1);
						getNickIndex1 = getNickIndex;
						m_pwndIRC->m_nicklist.NewNick( getNickChannel, getNick );
						getNickIndex = message.Find( _T(' '), getNickIndex1 + 1 );
					}
					return;
				}
				case 366:
				{
					CString channel;
					channel = message.Mid(0, message.Find(_T(' ')));
					SendString(_T("MODE ")+channel);
					m_pwndIRC->m_nicklist.ShowWindow(SW_SHOW);
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- In replying to the LINKS message, a server MUST send
				//replies back using the RPL_LINKS numeric and mark the
				//end of the list using an RPL_ENDOFLINKS reply.
				//364    RPL_LINKS
				//"<mask> <server> :<hopcount> <server info>"
				//365    RPL_ENDOFLINKS
				//"<mask> :End of LINKS list"
				case 364:
				case 365:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- When listing the active 'bans' for a given channel,
				//a server is required to send the list back using the
				//RPL_BANLIST and RPL_ENDOFBANLIST messages.  A separate
				//RPL_BANLIST is sent for each active banmask.  After the
				//banmasks have been listed (or if none present) a
				//RPL_ENDOFBANLIST MUST be sent.
				//367    RPL_BANLIST
				//"<channel> <banmask>"
				//368    RPL_ENDOFBANLIST
				//"<channel> :End of channel ban list"
				case 367:
				case 368:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- A server responding to an INFO message is required to
				//send all its 'info' in a series of RPL_INFO messages
				//with a RPL_ENDOFINFO reply to indicate the end of the
				//replies.
				//371    RPL_INFO
				//":<string>"
				//374    RPL_ENDOFINFO
				//":End of INFO list"
				case 371:
				case 374:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- When responding to the MOTD message and the MOTD file
				//is found, the file is displayed line by line, with
				//each line no longer than 80 characters, using
				//RPL_MOTD format replies.  These MUST be surrounded
				//by a RPL_MOTDSTART (before the RPL_MOTDs) and an
				//RPL_ENDOFMOTD (after).
				//375    RPL_MOTDSTART
				//":- <server> Message of the day - "
				//372    RPL_MOTD
				//":- <text>"
				//376    RPL_ENDOFMOTD
				//":End of MOTD command"
				case 375:
				case 372:
				case 376:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- RPL_YOUREOPER is sent back to a client which has
				//just successfully issued an OPER message and gained
				//operator status.
				//381    RPL_YOUREOPER
				//":You are now an IRC operator"
				case 381:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- If the REHASH option is used and an operator sends
				//a REHASH message, an RPL_REHASHING is sent back to
				//the operator.
				//382    RPL_REHASHING
				//"<config file> :Rehashing"
				case 382:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Sent by the server to a service upon successful
				//registration.
				//383    RPL_YOURESERVICE
				//"You are service <servicename>"
				case 383:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- When replying to the TIME message, a server MUST send
				//the reply using the RPL_TIME format above.  The string
				//showing the time need only contain the correct day and
				//time there.  There is no further requirement for the
				//time string.
				//391    RPL_TIME
				//"<server> :<string showing server's local time>"
				case 391:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- If the USERS message is handled by a server, the
				//replies RPL_USERSTART, RPL_USERS, RPL_ENDOFUSERS and
				//RPL_NOUSERS are used.  RPL_USERSSTART MUST be sent
				//first, following by either a sequence of RPL_USERS
				//or a single RPL_NOUSER.  Following this is
				//RPL_ENDOFUSERS.
				//392    RPL_USERSSTART
				//":UserID   Terminal  Host"
				//393    RPL_USERS
				//":<username> <ttyline> <hostname>"
				//       394    RPL_ENDOFUSERS
				//              ":End of users"
				//395    RPL_NOUSERS
				//":Nobody logged in"
				case 392:
				case 393:
				case 395:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- The RPL_TRACE* are all returned by the server in
				//response to the TRACE message.  How many are
				//returned is dependent on the TRACE message and
				//whether it was sent by an operator or not.  There
				//is no predefined order for which occurs first.
				//Replies RPL_TRACEUNKNOWN, RPL_TRACECONNECTING and
				//RPL_TRACEHANDSHAKE are all used for connections
				//which have not been fully established and are either
				//unknown, still attempting to connect or in the
				//process of completing the 'server handshake'.
				//RPL_TRACELINK is sent by any server which handles
				//a TRACE message and has to pass it on to another
				//server.  The list of RPL_TRACELINKs sent in
				//response to a TRACE command traversing the IRC
				//network should reflect the actual connectivity of
				//the servers themselves along that path.
				//RPL_TRACENEWTYPE is to be used for any connection
				//which does not fit in the other categories but is
				//being displayed anyway.
				//RPL_TRACEEND is sent to indicate the end of the list.
				//200    RPL_TRACELINK
				//"Link <version & debug level> <destination>
				//<next server> V<protocol version>
				//<link uptime in seconds> <backstream sendq>
				//<upstream sendq>"
				//201    RPL_TRACECONNECTING
				//"Try. <class> <server>"
				//202    RPL_TRACEHANDSHAKE
				//"H.S. <class> <server>"
				//203    RPL_TRACEUNKNOWN
				//"???? <class> [<client IP address in dot form>]"
				//204    RPL_TRACEOPERATOR
				//"Oper <class> <nick>"
				//205    RPL_TRACEUSER
				//"User <class> <nick>"
				//206    RPL_TRACESERVER
				//"Serv <class> <int>S <int>C <server>
				//<nick!user|*!*>@<host|server> V<protocol version>"
				//207    RPL_TRACESERVICE
				//"Service <class> <name> <type> <active type>"
				//208    RPL_TRACENEWTYPE
				//"<newtype> 0 <client name>"
				//209    RPL_TRACECLASS
				//"Class <class> <count>"
				//210    RPL_TRACERECONNECT
				//Unused.
				//261    RPL_TRACELOG
				//"File <logfile> <debug level>"
				//262    RPL_TRACEEND
				//"<server name> <version & debug level> :End of TRACE"
				case 200:
				case 201:
				case 202:
				case 203:
				case 204:
				case 205:
				case 206:
				case 207:
				case 208:
				case 209:
				case 210:
				case 261:
				case 262:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- reports statistics on a connection.  <linkname>
				//identifies the particular connection, <sendq> is
				//the amount of data that is queued and waiting to be
				//sent <sent messages> the number of messages sent,
				//and <sent Kbytes> the amount of data sent, in
				//Kbytes. <received messages> and <received Kbytes>
				//are the equivalent of <sent messages> and <sent
				//Kbytes> for received data, respectively.  <time
				//open> indicates how long ago the connection was
				//opened, in seconds.
				//211    RPL_STATSLINKINFO
				//"<linkname> <sendq> <sent messages>
				//<sent Kbytes> <received messages>
				//<received Kbytes> <time open>"
				case 211:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- reports statistics on commands usage.
				//212    RPL_STATSCOMMANDS
				//"<command> <count> <byte count> <remote count>"
				case 212:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- reports the server uptime.
				//219    RPL_ENDOFSTATS
				//"<stats letter> :End of STATS report"
				//242    RPL_STATSUPTIME
				//":Server Up %d days %d:%02d:%02d"
				case 219:
				case 242:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- reports the allowed hosts from where user may become IRC
				//operators.
				//243    RPL_STATSOLINE
				//"O <hostmask> * <name>"
				case 243:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- To answer a query about a client's own mode,
				//RPL_UMODEIS is sent back.
				//221    RPL_UMODEIS
				//"<user mode string>"
				case 221:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- When listing services in reply to a SERVLIST message,
				//a server is required to send the list back using the
				//RPL_SERVLIST and RPL_SERVLISTEND messages.  A separate
				//RPL_SERVLIST is sent for each service.  After the
				//services have been listed (or if none present) a
				//RPL_SERVLISTEND MUST be sent.
				//234    RPL_SERVLIST
				//"<name> <server> <mask> <type> <hopcount> <info>"
				//235    RPL_SERVLISTEND
				//"<mask> <type> :End of service listing"
				case 234:
				case 235:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

                //- In processing an LUSERS message, the server
				//sends a set of replies from RPL_LUSERCLIENT,
				//RPL_LUSEROP, RPL_USERUNKNOWN,
				//RPL_LUSERCHANNELS and RPL_LUSERME.  When
				//replying, a server MUST send back
				//RPL_LUSERCLIENT and RPL_LUSERME.  The other
				//replies are only sent back if a non-zero count
				//is found for them.
				//251    RPL_LUSERCLIENT
				//":There are <integer> users and <integer>
				//services on <integer> servers"
				//252    RPL_LUSEROP
				//"<integer> :operator(s) online"
				//253    RPL_LUSERUNKNOWN
				//"<integer> :unknown connection(s)"
				//254    RPL_LUSERCHANNELS
				//"<integer> :channels formed"
				//255    RPL_LUSERME
				//":I have <integer> clients and <integer>
				//servers"
				case 251:
				case 252:
				case 253:
				case 254:
				case 255:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- When replying to an ADMIN message, a server
				//is expected to use replies RPL_ADMINME
				//through to RPL_ADMINEMAIL and provide a text
				//message with each.  For RPL_ADMINLOC1 a
				//description of what city, state and country
				//the server is in is expected, followed by
				//details of the institution (RPL_ADMINLOC2)
				//and finally the administrative contact for the
				//server (an email address here is REQUIRED)
				//in RPL_ADMINEMAIL.
				//256    RPL_ADMINME
				//"<server> :Administrative info"
				//257    RPL_ADMINLOC1
				//":<admin info>"
				//258    RPL_ADMINLOC2
				//":<admin info>"
				//259    RPL_ADMINEMAIL
				//":<admin info>"
				case 256:
				case 257:
				case 258:
				case 259:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- When a server drops a command without processing it,
				//it MUST use the reply RPL_TRYAGAIN to inform the
				//originating client.
				//263    RPL_TRYAGAIN
				//"<command> :Please wait a while and try again."
				case 263:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Used to indicate the nickname parameter supplied to a
				//command is currently unused.
				//401    ERR_NOSUCHNICK
				//"<nickname> :No such nick/channel"
				case 401:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

                //- Used to indicate the server name given currently
				//does not exist.
				//402    ERR_NOSUCHSERVER
				//"<server name> :No such server"
				case 402:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

                //- Used to indicate the given channel name is invalid.
				//403    ERR_NOSUCHCHANNEL
				//"<channel name> :No such channel"
				case 403:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

                //- Sent to a user who is either (a) not on a channel
				//which is mode +n or (b) not a chanop (or mode +v) on
				//a channel which has mode +m set or where the user is
				//banned and is trying to send a PRIVMSG message to
				//that channel.
				//404    ERR_CANNOTSENDTOCHAN
				//"<channel name> :Cannot send to channel"
				case 404:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- Sent to a user when they have joined the maximum
				//number of allowed channels and they try to join
				//another channel.
				//405    ERR_TOOMANYCHANNELS
				//"<channel name> :You have joined too many channels"
				case 405:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

                //- Returned by WHOWAS to indicate there is no history
				//information for that nickname.
				//406    ERR_WASNOSUCHNICK
				//"<nickname> :There was no such nickname"
				case 406:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

                //- Returned to a client which is attempting to send a
				//PRIVMSG/NOTICE using the user@host destination format
				//and for a user@host which has several occurrences.
				//- Returned to a client which trying to send a
				//PRIVMSG/NOTICE to too many recipients.
				//- Returned to a client which is attempting to JOIN a safe
				//channel using the shortname when there are more than one
				//such channel.
				//407    ERR_TOOMANYTARGETS
				//"<target> :<error code> recipients. <abort message>"
				case 407:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

				//- Returned to a client which is attempting to send a SQUERY
				//to a service which does not exist.
				//408    ERR_NOSUCHSERVICE
				//"<service name> :No such service"
				case 408:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- PING or PONG message missing the originator parameter.
				//409    ERR_NOORIGIN
				//":No origin specified"
				case 409:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- 412 - 415 are returned by PRIVMSG to indicate that
				//the message wasn't delivered for some reason.
				//ERR_NOTOPLEVEL and ERR_WILDTOPLEVEL are errors that
				//are returned when an invalid use of
				//"PRIVMSG $<server>" or "PRIVMSG #<host>" is attempted.
				//411    ERR_NORECIPIENT
				//":No recipient given (<command>)"
				//412    ERR_NOTEXTTOSEND
				//":No text to send"
				//413    ERR_NOTOPLEVEL
				//"<mask> :No toplevel domain specified"
				//414    ERR_WILDTOPLEVEL
				//"<mask> :Wildcard in toplevel domain"
				//415    ERR_BADMASK
				//"<mask> :Bad Server/host mask"
				case 411:
				case 412:
				case 413:
				case 414:
				case 415:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned to a registered client to indicate that the
				//command sent is unknown by the server.
				//421    ERR_UNKNOWNCOMMAND
				//"<command> :Unknown command"
				case 421:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Server's MOTD file could not be opened by the server.
				//422    ERR_NOMOTD
				//":MOTD File is missing"
				case 422:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned by a server in response to an ADMIN message
				//when there is an error in finding the appropriate
				//information.
				//423    ERR_NOADMININFO
				//"<server> :No administrative info available"
				case 423:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Generic error message used to report a failed file
				//operation during the processing of a message.
				//424    ERR_FILEERROR
				//":File error doing <file op> on <file>"
				case 424:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned when a nickname parameter expected for a
				//command and isn't found.
				//431    ERR_NONICKNAMEGIVEN
				//":No nickname given"
				case 431:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned after receiving a NICK message which contains
				//characters which do not fall in the defined set.  See
				//section 2.3.1 for details on valid nicknames.
				//432    ERR_ERRONEUSNICKNAME
				//"<nick> :Erroneous nickname"
				case 432:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned when a NICK message is processed that results
				//in an attempt to change to a currently existing
				//nickname.
				//433    ERR_NICKNAMEINUSE
				//"<nick> :Nickname is already in use"
				case 433:
				{
					if( !m_pwndIRC->GetLoggedIn() )
						Disconnect();
					m_pwndIRC->AddStatus( GetResString(IDS_IRC_NICKUSED));
					return;
				}

                //- Returned by a server to a client when it detects a
				//nickname collision (registered of a NICK that
				//already exists by another server).
				//436    ERR_NICKCOLLISION
				//"<nick> :Nickname collision KILL from <user>@<host>"
				case 436:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}

                //- Returned by a server to a user trying to join a channel
				//currently blocked by the channel delay mechanism.
				//- Returned by a server to a user trying to change nickname
				//when the desired nickname is blocked by the nick delay
				//mechanism.
				//437    ERR_UNAVAILRESOURCE
				//"<nick/channel> :Nick/channel is temporarily unavailable"
				case 437:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned by the server to indicate that the target
				//user of the command is not on the given channel.
				//441    ERR_USERNOTINCHANNEL
				//"<nick> <channel> :They aren't on that channel"
				case 441:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned by the server whenever a client tries to
				//perform a channel affecting command for which the
				//client isn't a member.
				//442    ERR_NOTONCHANNEL
				//"<channel> :You're not on that channel"
				case 442:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned when a client tries to invite a user to a
				//channel they are already on.
				//443    ERR_USERONCHANNEL
				//"<user> <channel> :is already on channel"
				case 443:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned by the summon after a SUMMON command for a
				//user was unable to be performed since they were not
				//logged in.
				//444    ERR_NOLOGIN
				//"<user> :User not logged in"
				case 444:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned as a response to the SUMMON command.  MUST be
				//returned by any server which doesn't implement it.
				//445    ERR_SUMMONDISABLED
				//":SUMMON has been disabled"
				case 445:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned as a response to the USERS command.  MUST be
				//returned by any server which does not implement it.
				//446    ERR_USERSDISABLED
				//":USERS has been disabled"
				case 446:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned by the server to indicate that the client
				//MUST be registered before the server will allow it
				//to be parsed in detail.
				//451    ERR_NOTREGISTERED
				//":You have not registered"
				case 451:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
                
				//- Returned by the server by numerous commands to
				//indicate to the client that it didn't supply enough
				//parameters.
				//461    ERR_NEEDMOREPARAMS
				//"<command> :Not enough parameters"
				case 461:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned by the server to any link which tries to
				//change part of the registered details (such as
				//password or user details from second USER message).
				//462    ERR_ALREADYREGISTRED
				//":Unauthorized command (already registered)"
				case 462:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned to a client which attempts to register with
				//a server which does not been setup to allow
				//connections from the host the attempted connection
				//is tried.
				//463    ERR_NOPERMFORHOST
				//":Your host isn't among the privileged"
				case 463:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned to indicate a failed attempt at registering
				//a connection for which a password was required and
				//was either not given or incorrect.
				//464    ERR_PASSWDMISMATCH
				//":Password incorrect"
				case 464:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned after an attempt to connect and register
				//yourself with a server which has been setup to
				//explicitly deny connections to you.
				//465    ERR_YOUREBANNEDCREEP
				//":You are banned from this server"
				case 465:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
                //- Sent by a server to a user to inform that access to the
				//server will soon be denied.
				//466    ERR_YOUWILLBEBANNED
				case 466:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Any command requiring operator privileges to operate
				//MUST return this error to indicate the attempt was
				//unsuccessful.
				//467    ERR_KEYSET
				//"<channel> :Channel key already set"
				//471    ERR_CHANNELISFULL"<channel> :Cannot join channel (+l)"
				//472    ERR_UNKNOWNMODE
				//"<char> :is unknown mode char to me for <channel>"
				//473    ERR_INVITEONLYCHAN
				//"<channel> :Cannot join channel (+i)"
				//474    ERR_BANNEDFROMCHAN
				//"<channel> :Cannot join channel (+b)"
				//475    ERR_BADCHANNELKEY
				//"<channel> :Cannot join channel (+k)"
				//476    ERR_BADCHANMASK
				//"<channel> :Bad Channel Mask"
				//477    ERR_NOCHANMODES
				//"<channel> :Channel doesn't support modes"
				//478    ERR_BANLISTFULL
				//"<channel> <char> :Channel list is full"
				//481    ERR_NOPRIVILEGES
				//":Permission Denied- You're not an IRC operator"
				case 467:
				case 471:
				case 472:
				case 473:
				case 474:
				case 475:
				case 476:
				case 477:
				case 478:
				case 481:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Any command requiring 'chanop' privileges (such as
				//MODE messages) MUST return this error if the client
				//making the attempt is not a chanop on the specified
				//channel.
				//482    ERR_CHANOPRIVSNEEDED
				//"<channel> :You're not channel operator"
				case 482:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Any attempts to use the KILL command on a server
				//are to be refused and this error returned directly
				//to the client.
				//483    ERR_CANTKILLSERVER
				//":You can't kill a server!"
				case 483:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Sent by the server to a user upon connection to indicate
				//the restricted nature of the connection (user mode "+r").
				//484    ERR_RESTRICTED
				//":Your connection is restricted!"
				case 484:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Any MODE requiring "channel creator" privileges MUST
				//return this error if the client making the attempt is not
				//a chanop on the specified channel.
				//485    ERR_UNIQOPPRIVSNEEDED
				//":You're not the original channel operator"
				case 485:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- If a client sends an OPER message and the server has
				//not been configured to allow connections from the
				//client's host as an operator, this error MUST be
				//returned.
				//491    ERR_NOOPERHOST
				//":No O-lines for your host"
				case 491:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Returned by the server to indicate that a MODE
				//message was sent with a nickname parameter and that
				//the a mode flag sent was not recognized.
				//501    ERR_UMODEUNKNOWNFLAG
				//":Unknown MODE flag"
				case 501:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
				
				//- Error sent to any user trying to view or change the
				//user mode for a user other than themselves.
				//502    ERR_USERSDONTMATCH
				//":Cannot change mode for other users"
				case 502:
				{
					m_pwndIRC->AddStatus( message );
					return;
				}
			}
		}
		m_pwndIRC->AddStatus( _T("[") + command + _T("]") + message );
	}
	catch(CString e )
	{
		m_pwndIRC->AddStatus(e);
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Exception in CIrcMain::ParseMessage"));
	}
}

void CIrcMain::SendLogin()
{
	try
	{
		ircsocket->SendString(user);
		CString temp = _T("NICK ") + nick;
		ircsocket->SendString(temp);
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Exception in CIrcMain::SendLogin"));
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
			AddDebugLogLine(false, _T("Exception in CIrcMain::ParsePerform(1)"));
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
			AddDebugLogLine(false, _T("Exception in CIrcMain::ParsePerform(2)"));
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
		ident.Format(_T("%ue%u"), thePrefs.GetLanguageID(), ident_int);
		if( ident.GetLength() > 8 )
			ident.Truncate(8);
		if(ircsocket)
			Disconnect();
		ircsocket = new CIrcSocket(this);
		nick = (CString)thePrefs.GetIRCNick();
		nick.Replace(_T("."), _T(""));
		nick.Replace(_T(" "), _T(""));
		nick.Replace(_T(":"), _T(""));
		nick.Replace(_T("/"), _T(""));
		nick.Replace(_T("@"), _T(""));
		if( !nick.CompareNoCase( _T("emule") ) )
		{
			uint16 ident_int = 0;
			for( int i = 0; i < 16; i++)
			{
				ident_int += thePrefs.GetUserHash()[i] * thePrefs.GetUserHash()[15-i];
			}
			nick.Format(_T("eMuleIRC%u-%u"), thePrefs.GetLanguageID(), ident_int);
		}
		nick = nick.Left(25);
		version = _T("eMule") + theApp.m_strCurVersionLong + (CString)Irc_Version;
		user = _T("USER ") + ident + _T(" 8 * :") + version;
		ircsocket->Create();
		//TODO: Make this multilanguage..
		m_pwndIRC->AddStatus( _T("Connecting") );
		ircsocket->Connect();
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Exception in CIrcMain::Connect"));
	}
}

void CIrcMain::Disconnect(bool isshuttingdown)
{
	try
	{
		ircsocket->Close();
		delete ircsocket;
		ircsocket = NULL;
		preParseBuffer = _T("");
		if( !isshuttingdown )
			m_pwndIRC->SetConnectStatus(false);
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Exception in CIrcMain::Disconnect"));
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
			AddDebugLogLine(false, _T("Exception in CIrcMain::SetConnectStatus"));
	}
}

int CIrcMain::SendString( CString send )
{
	return ircsocket->SendString(send);
}