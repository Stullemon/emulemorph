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

#include "StdAfx.h"
#include "ircmain.h"
#include "emule.h"
#include "ED2KLink.h"

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
	CString rawMessage;
	preParseBuffer += buffer;
	int test = preParseBuffer.Find('\n');
	while( test != -1 ){
		rawMessage = preParseBuffer.Left(test);
		rawMessage.Replace( "\n", "" );
		rawMessage.Replace( "\r", "" );
		ParseMessage( rawMessage );
		preParseBuffer = preParseBuffer.Mid(test+1);
		test = preParseBuffer.Find("\n");
	}
}

//extern void URLDecode(CString& result, const char* buff);

void CIrcMain::ProcessLink( CString ed2kLink ) {
	try {
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
					theApp.emuledlg->serverwnd.UpdateServerMetFromURL(strAddress);
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
				if( theApp.glob_prefs->GetManualHighPrio() )
					pSrv->SetPreference(SRV_PR_HIGH);

				if (!theApp.emuledlg->serverwnd.serverlistctrl.AddServer(pSrv,true)) 
					delete pSrv; 
				else
					AddLogLine(true,GetResString(IDS_SERVERADDED), pSrv->GetListName());
			}
			break;
		default:
			break;
		}
		delete pLink;
	} catch(...) {
		OUTPUT_DEBUG_TRACE();
		AddLogLine(true, GetResString(IDS_LINKNOTADDED));
	}
}

void CIrcMain::ParseMessage( CString rawMessage ){
	if( rawMessage.Left(6) == "PING :" ){
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

	if( sourceIndex < commandIndex && sourceIndex > 0){
		source = rawMessage.Mid( 1, sourceIndex - 1);
		sourceIp = rawMessage.Mid( sourceIndex + 1, commandIndex - sourceIndex - 1);
		messageIndex = rawMessage.Find( ' ', targetIndex + 1);
		if( messageIndex > sourceIndex ){
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
		else{
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
	else{
		source = rawMessage.Mid( 1, commandIndex - 1);
		message = rawMessage.Mid( targetIndex + 1);
	}
	if( command == "PRIVMSG" ){
		if ( target.Left(1) == "#" ){
			if( message.Left(1) == "\001" ){
				message = message.Mid( 1, message.GetLength() - 2);
				if( message.Left(6) == "ACTION" ){
					m_pwndIRC->AddInfoMessage( target, "* %s%s", source, message.Mid(6) );
					return;
				}
				if( message.Left(7) == "VERSION"){
					version = "eMule" + theApp.m_strCurVersionLong + (CString)Irc_Version;
					CString build;
					build.Format( "NOTICE %s :\001VERSION %s\001", source, version );
					ircsocket->SendString( build );
					return;
				}
			}
			
			else{
                m_pwndIRC->AddMessage( target, source, message);
				return;
			}
		}
		else{
			if( message.Left(1) == "\001" ){
				message = message.Mid(1, message.GetLength() -2);
				if( message.Left(6) == "ACTION"){
					m_pwndIRC->AddInfoMessage( source, "* %s%s", source, message.Mid(6) );
					return;
				}
				if( message.Left(7) == "VERSION"){
					version = "eMule" + theApp.m_strCurVersionLong + (CString)Irc_Version;
					CString build;
					build.Format( "NOTICE %s :\001VERSION %s\001", source, version );
					ircsocket->SendString( build );
				}
				if( message.Left(9) == "RQSFRIEND" ){
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
					build.Format( "PRIVMSG %s :\001REPFRIEND eMule%s%s|%s|%i:%i|%s:%s|%s|\001", source, theApp.m_strCurVersionLong, Irc_Version, sverify, theApp.IsFirewalled() ? 0 : theApp.GetID(), theApp.glob_prefs->GetPort(), sip, sport, EncodeBase16((const unsigned char*)theApp.glob_prefs->GetUserHash(), 16));
					ircsocket->SendString( build );
					build.Format( "%s %s", source, GetResString(IDS_IRC_ADDASFRIEND));
					if( !theApp.glob_prefs->GetIrcIgnoreEmuleProtoInfoMessage() )
						m_pwndIRC->NoticeMessage( "*EmuleProto*", build );
					return;
				}
				if( message.Left(8) == "SENDLINK" ){
					if ( !theApp.glob_prefs->GetIrcAcceptLinks() ){
						if( !theApp.glob_prefs->GetIrcIgnoreInfoMessage() ){
							m_pwndIRC->NoticeMessage( "*EmuleProto*", "Someone attempt to send you a file. If you wanted to accept the file, enable Recieve files in the Preferences.");
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
					DecodeBase16(hash.GetBuffer(),hash.GetLength(),userid);
					CString RecieveString, build;
					if(!theApp.friendlist->SearchFriend(userid, 0, 0)){
						if( !theApp.glob_prefs->GetIrcIgnoreInfoMessage() ){
							m_pwndIRC->NoticeMessage( "*EmuleProto*", "Someone attempt to send you a file but wasn't a friend. If you wanted to accept the file, disable From friends only in the preferences.");
						}
						return;
					}
					RecieveString = message.Mid( index2+1 );
					if( !RecieveString.IsEmpty() ){
						build.Format( GetResString(IDS_IRC_RECIEVEDLINK), source, RecieveString );
						if( !theApp.glob_prefs->GetIrcIgnoreInfoMessage() )
							m_pwndIRC->NoticeMessage( "*EmuleProto*", build );
						ProcessLink( RecieveString );
					}
					return;
				}
				if( message.Left(9) == "REPFRIEND" ){
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
					uint32 newClientServerIP = atoi(message.Mid( index2 + 1, index1 - index2 -1));
					index2 = message.Find( '|', index1+1);
					if( index2 == -1 || index1 >= index2 )
						return;
					uint16 newClientServerPort = atoi(message.Mid( index1 + 1, index2 - index1 -1));
					index1 = message.Find( '|', index2+1);
					if( index1 == -1 || index2 >= index1 )
						return;
					CString hash = message.Mid( index2 + 1, index1 - index2 -1);
					uchar userid[16];
					DecodeBase16(hash.GetBuffer(),hash.GetLength(),userid);
					theApp.friendlist->AddFriend( userid, 0, newClientID, newClientPort, 0, source, 1);
				}
				return;
			}
			else{
				m_pwndIRC->AddMessage( source, source, message);
				return;
			}
		}
	}
	if( command == "JOIN" ){
		if( source == nick ){
			m_pwndIRC->AddInfoMessage( target, GetResString(IDS_IRC_HASJOINED), source, target );
		return;
		}
		if( !theApp.glob_prefs->GetIrcIgnoreInfoMessage() )
			m_pwndIRC->AddInfoMessage( target, GetResString(IDS_IRC_HASJOINED), source, target );
		m_pwndIRC->NewNick( target, source );
		return;

	}
	if( command == "PART" ){
		CString test = nick;
		if ( source == nick ){
			m_pwndIRC->RemoveChannel( target );
			return;
		}
		m_pwndIRC->RemoveNick( target, source );
		if( !theApp.glob_prefs->GetIrcIgnoreInfoMessage() )
			m_pwndIRC->AddInfoMessage( target, GetResString(IDS_IRC_HASPARTED), source, target, message );
		return;
	}

	if( command == "TOPIC" ){
		m_pwndIRC->SetTitle( target, message );
		return;
	}

	if( command == "QUIT" ){
		m_pwndIRC->DeleteNickInAll( source, message );
		return;
	}

	if( command == "NICK" ){
		if( source == nick ){
			nick = target;
//			m_pwenIRC->SetNick( nick );
		}
		m_pwndIRC->ChangeAllNick( source, target );
		return;
	}

	if( command == "KICK" ){
		target2Index = message.Find(':');
		if( target2Index > 0 ){
			target2 = message.Mid( 1, target2Index - 2);
			message = message.Mid( target2Index + 1 );
		}
		if( target2 == nick ){
			m_pwndIRC->RemoveChannel( target );
			m_pwndIRC->AddStatus(  GetResString(IDS_IRC_WASKICKEDBY), target2, source, message );
			return;
		}
		m_pwndIRC->RemoveNick( target, target2 );
		if( !theApp.glob_prefs->GetIrcIgnoreInfoMessage() )
			m_pwndIRC->AddInfoMessage( target, GetResString(IDS_IRC_WASKICKEDBY), target2, source, message );
		return;
	}
	if( command == "MODE" ){
		commandIndex = message.Find( ' ', 1 );
		command = message.Mid( 1, commandIndex - 1 );
		command.Replace( "\004", "%" );
		target2 = message.Mid( commandIndex + 1 );
		m_pwndIRC->ParseChangeMode( target, source, command, target2 );
		return;
	}
	if( command == "NOTICE"){
		m_pwndIRC->NoticeMessage( source, message );
		return;
	}
	if( command == "001" ){
		m_pwndIRC->SetLoggedIn( true );
		if( theApp.glob_prefs->GetIRCListOnConnect() )
			ircsocket->SendString("list");
		ParsePerform();
	}

	if( command == "321" ){
		m_pwndIRC->ResetServerChannelList();
		return;
	}

	if( command == "322"){
		CString chanName, chanNum, chanDesc;
		int chanNameIndex, chanNumIndex, chanDescIndex;
		chanNameIndex = message.Find( ' ' );
		chanNumIndex = message.Find( ' ', chanNameIndex + 1 );
		chanDescIndex = message.Find( ' ', chanNumIndex + 1);
		chanName = message.Mid( chanNameIndex + 1,  chanNumIndex - chanNameIndex - 1 );
		chanNum = message.Mid( chanNumIndex + 1, chanDescIndex - chanNumIndex - 1 );
		if( chanDescIndex > 0 ){
			chanDescIndex = message.Find( ' ', chanDescIndex+1 );
			if( chanDescIndex > 0)
				chanDesc = message.Mid( chanDescIndex );
			else
				chanDesc = "";
		}
		m_pwndIRC->AddChannelToList( chanName, chanNum, chanDesc );
		return;
	}
	if( command == "332" ){
		target2 = message.Mid( message.Find( '#' ), message.Find( ':' )-message.Find( '#' ) -1 );
		message = message.Mid( message.Find(':') + 1);
		m_pwndIRC->SetTitle( target2, message );
		m_pwndIRC->AddInfoMessage( target2, "* Channel Title: %s", message );
		return;
	}
	if( command == "353" ){
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
		while( getNickIndex > 0 ){
			count ++;
			getNick = rawMessage.Mid( getNickChannelIndex + 1, getNickIndex - getNickChannelIndex - 1);
			getNickChannelIndex = getNickIndex;
			m_pwndIRC->NewNick( getNickChannel, getNick );
			getNickIndex = rawMessage.Find( ' ', getNickChannelIndex + 1 );
		}
		return;
	}
	if( command == "433" ){
		if( !m_pwndIRC->GetLoggedIn() )
			Disconnect();
		m_pwndIRC->AddStatus(  GetResString(IDS_IRC_NICKUSED));
		return;
	}
	m_pwndIRC->AddStatus( message );
}

void CIrcMain::SendLogin(){
	ircsocket->SendString(user);
	CString temp = "NICK " + nick;
	ircsocket->SendString(temp);
}

void CIrcMain::ParsePerform(){
	CString rawPerform = "";
	if(theApp.glob_prefs->GetIrcHelpChannel())
		rawPerform = "/" + GetResString(IDS_IRC_HELPCHANNELPERFORM) + "|";
	if(theApp.glob_prefs->GetIrcUsePerform())
		rawPerform += theApp.glob_prefs->GetIrcPerformString();
	if(rawPerform == "" )
		return;
	int index = 0;
	CString nextPerform;
	while( rawPerform.Find('|') != -1 ){
		index = rawPerform.Find( '|' );
		nextPerform = rawPerform.Left( index );
		nextPerform.TrimLeft( ' ' );
		if( nextPerform.Left(1) == '/' )
			nextPerform = nextPerform.Mid(1);
		if (nextPerform.Left(3) == "msg")
			nextPerform = CString("PRIVMSG") + nextPerform.Mid(3);
		if( (nextPerform.Left(16)).CompareNoCase( "PRIVMSG nickserv"  )== 0){
			nextPerform = CString("ns") + nextPerform.Mid(16);
		}
		if( (nextPerform.Left(16)).CompareNoCase( "PRIVMSG chanserv" )== 0){
			nextPerform = CString("cs") + nextPerform.Mid(16);
		}
		ircsocket->SendString( nextPerform );
		rawPerform = rawPerform.Mid( index+1 );
	}
	if( !rawPerform.IsEmpty() ){
		rawPerform.TrimLeft( ' ' );
		if( rawPerform.Left(1) == '/' )
			rawPerform = rawPerform.Mid(1);
		if (rawPerform.Left(3) == "msg")
			rawPerform = CString("PRIVMSG") + rawPerform.Mid(3);
		if( (rawPerform.Left(16)).CompareNoCase( "PRIVMSG nickserv"  )== 0){
			rawPerform = CString("ns") + rawPerform.Mid(16);
		}
		if( (rawPerform.Left(16)).CompareNoCase( "PRIVMSG chanserv" )== 0){
			rawPerform = CString("cs") + rawPerform.Mid(16);
		}
		if( !rawPerform.IsEmpty() )
			ircsocket->SendString( rawPerform );
	}
}

void CIrcMain::Connect(){
	CString ident;
	uint16 ident_int = 0;
	for( int i = 0; i < 16; i++){
		ident_int += theApp.glob_prefs->GetUserHash()[i] * theApp.glob_prefs->GetUserHash()[15-i];
	}
	ident.Format("e%u", ident_int);
	if( ident.GetLength() > 8 )
		ident.Truncate(8);
	ircsocket = new CIrcSocket(this);
	nick = (CString)theApp.glob_prefs->GetIRCNick();
	nick.Replace(".", "");
	nick.Replace(" ", "");
	nick.Replace(":", "");
	nick.Replace("/", "");
	nick.Replace("@", "");
	nick = nick.Left(20);
	version = "eMule" + theApp.m_strCurVersionLong + (CString)Irc_Version;
	user = "USER " + ident + " 8 * :" + version;
	ircsocket->Create();
	ircsocket->Connect();
}

void CIrcMain::Disconnect(bool isshuttingdown){
	ircsocket->Close();
	delete ircsocket;
	ircsocket = NULL;
	if( !isshuttingdown )
		m_pwndIRC->SetConnectStatus(false);
}

void CIrcMain::SetConnectStatus( bool connected ){
	m_pwndIRC->SetConnectStatus( connected );
}

int CIrcMain::SendString( CString send ){
	return ircsocket->SendString(send);
}