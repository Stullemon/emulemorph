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

#pragma once
// MOD Note: Do not change this part - Merkur
#define	EMULE_PROTOCOL			0x01
// MOD Note: end
#define	EDONKEYVERSION			0x3C
#define PREFFILE_VERSION		0x14	//<<-- last change: reduced .dat, by using .ini
#define PARTFILE_VERSION		0xe0
#define PARTFILE_SPLITTEDVERSION		0xe1
#define CREDITFILE_VERSION		0x12
#define CREDITFILE_VERSION_29	0x11
#define COMPILE_DATE			__DATE__
#define COMPILE_TIME			__TIME__
#ifdef _DEBUG
#define EMULE_GUID				"EMULE-{4EADC6FC-516F-4b7c-9066-97D893649570}-DEBUG"
#else
#define EMULE_GUID				"EMULE-{4EADC6FC-516F-4b7c-9066-97D893649570}"
#endif

#define	SEC2MS(sec)		((sec)*1000)
#define	MIN2MS(min)		SEC2MS((min)*60)

// MOD Note: Do not change this part - Merkur
#define UDPSEARCHSPEED			1000	//1 sec - if this value is too low you will miss sources
#define MAX_RESULTS				100		// max global search results
#define	MAX_MORE_SEARCH_REQ		5		// this gives a max. total search results of (1+5)*201 = 1206 or (1+5)*300 = 1800
#define MAX_CLIENTCONNECTIONTRY	2
//#define CONNECTION_TIMEOUT		40000	//40 secs - set his lower if you want less connections at once, set it higher if you have enough sockets (edonkey has its own timout too, so a very high value won't effect this)
#define CONNECTION_TIMEOUT		60000	// <<-- enkeyDEV(th1) -L2HAC-
#define	FILEREASKTIME			1500000	//25 mins
#define SERVERREASKTIME			900000  //15 mins - don't set this too low, it wont speed up anything, but it could kill emule or your internetconnection
#define UDPSERVERREASKTIME		1800000	//30 mins
#define SOURCECLIENTREASK		1080000	//18 mins
#define KADEMLIAASKTIME			1000	//10 second
#define KADEMLIATOTALFILE		3		//Total files to search sources for.
#define KADEMLIAREASKTIME		3600000 //1 hour
#define KADEMLIAPUBLISHTIME		2000	//2 second
#define KADEMLIATOTALSTORESRC	2		//Total hashes to store.
#define KADEMLIATOTALSTOREKEY	1		//Total hashes to store.
#define KADEMLIAREPUBLISHTIME	18000	//5 hours
#define KADEMLIAINDEXCLEAN		18000	//5 hours
#define ED2KREPUBLISHTIME		60000	//1 min
#define MINCOMMONPENALTY		9
#define UDPSERVERSTATTIME		5000	//5 secs
#define UDPSERVSTATREASKTIME	14400	//4 hours
#define	UDPSERVERPORT			4665	// default udp port
#define RSAKEYSIZE				384		//384 bits
// MOD Note: end

#define CONFIGFOLDER			"config\\"
#define MAXCONPER5SEC			20	
#define MAXCON5WIN9X			10
#define	UPLOAD_CHECK_CLIENT_DR	2048
//MORPH START - Added by Yun.SF3, ZZ Upload System 20030807-1911
//#define	UPLOAD_CLIENT_DATARATE	3072	// uploadspeed per client in bytes - you may want to adjust this if you have a slow connection or T1-T3 ;)
#define	UPLOAD_CLIENT_DATARATE	4000
#define MINNUMBEROFTRICKLEUPLOADS 0 /*Old 2*/
#define MINWAITBEFOREOPENANOTHERSLOTMS 3000
//MORPH END   - Added by Yun.SF3, ZZ Upload System 20030807-1911
#define	MAX_UP_CLIENTS_ALLOWED	100		// max. clients allowed regardless UPLOAD_CLIENT_DATARATE or any other factors. Don't set this too low, use DATARATE to adjust uploadspeed per client
//MOPRH START - Modified by SiRoB, ZZ Upload System 20030807-1911
//#define	MIN_UP_CLIENTS_ALLOWED	2		// min. clients allowed to download regardless UPLOAD_CLIENT_DATARATE or any other factors. Don't set this too high
#define	MIN_UP_CLIENTS_ALLOWED	1		// min. clients allowed to download regardless UPLOAD_CLIENT_DATARATE or any other factors. Don't set this too high
//MOPRH END   - Modified by SiRoB, ZZ Upload System 20030807-1911

#define DOWNLOADTIMEOUT			100000
#define CONSERVTIMEOUT			25000	// agelimit for pending connection attempts
#define RARE_FILE				25
#define BADCLIENTBAN			4
#define	MIN_REQUESTTIME			590000
#define	MAX_PURGEQUEUETIME		3600000 
#define PURGESOURCESWAPSTOP		900000	// (15 mins), how long forbid swapping a source to a certain file (NNP,...)
#define CONNECTION_LATENCY		22050	// latency for responces
#define	SOURCESSLOTS			100
//MORPH - Added by Yun.SF3, ZZ Upload System
#define MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE   3000
#define MINWAIT_BEFORE_ULDISPLAY_WINDOWUPDATE   3000
#define MINWAIT_BEFORE_CALCULATE_DL_RATE        1000
//MORPH - Added by Yun.SF3, ZZ Upload System
#define MAXAVERAGETIME			40000 //millisecs
#define CLIENTBANTIME			7200000 // 2h
#define TRACKED_CLEANUP_TIME	3600000 // 1 hour
#define KEEPTRACK_TIME			7200000 // 2h	//how long to keep track of clients which were once in the uploadqueue
#define LOCALSERVERREQUESTS		20000	// only one local src request during this timespan
#define DISKSPACERECHECKTIME	900000	// SLUGFILLER: checkDiskspace
//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
#define SESSIONAMOUNT (10*1024*1024) // PENDING: Change to 10 MBytes when done testing!
//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923

// START enkeyDEV(th1)
#define L2HAC_DEFAULT_EMULE		(FILEREASKTIME)
#define L2HAC_MIN_TIME			900000
#define L2HAC_MAX_TIME			3600000
#define L2HAC_CALLBACK_PRECEDE	(CONNECTION_TIMEOUT >> 1)
#define L2HAC_PREPARE_PRECEDE	(CONNECTION_TIMEOUT)
// END enkeyDEV(th1)

// you shouldn't change anything here if you are not really sure, or emule will probaly not work
//MOPRH START - Added by SiRoB, ZZ Upload System 20030824-2238
#define MINFRAGSIZE             2500/*512*/
//MOPRH END   - Added by SiRoB, ZZ Upload System 20030824-2238
#define	MAXFRAGSIZE				1300
#define MP_FORCE 10200//shadow#(onlydownloadcompletefiles) //EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
#define	PARTSIZE				9728000
#define EMBLOCKSIZE				184320
#define OP_EDONKEYHEADER		0xE3
#define OP_KADEMLIAHEADER		0xE4
#define OP_KADEMLIAPACKEDPROT	0xE5
#define OP_EDONKEYPROT			OP_EDONKEYHEADER
#define OP_PACKEDPROT			0xD4
#define OP_EMULEPROT			0xC5
#define OP_MLDONKEYPROT			0x00
#define	MET_HEADER				0x0E
	
#define UNLIMITED				0xFFFF

//Proxytypes deadlake
#define PROXYTYPE_NOPROXY 0
#define PROXYTYPE_SOCKS4 1
#define PROXYTYPE_SOCKS4A 2
#define PROXYTYPE_SOCKS5 3
#define PROXYTYPE_HTTP11 4

// client <-> server
#define OP_LOGINREQUEST			0x01	//<HASH 16><ID 4><PORT 2><1 Tag_set>
#define OP_REJECT				0x05	//(null)
#define OP_GETSERVERLIST		0x14	//(null)client->server
#define	OP_OFFERFILES			0x15	// <count 4>(<HASH 16><ID 4><PORT 2><1 Tag_set>)[count]
#define OP_SEARCHREQUEST		0x16	// <Query_Tree>
#define OP_DISCONNECT			0x18	// (not verified)
#define OP_GETSOURCES			0x19	// <HASH 16>
#define	OP_SEARCH_USER			0x1A	// <Query_Tree>
#define OP_CALLBACKREQUEST		0x1C	// <ID 4>
#define	OP_QUERY_CHATS			0x1D	// (deprecated not supported by server any longer)
#define OP_CHAT_MESSAGE        	0x1E    // (deprecated not supported by server any longer)
#define OP_JOIN_ROOM            0x1F    // (deprecated not supported by server any longer)
#define OP_QUERY_MORE_RESULT    0x21    // (null)
#define OP_SERVERLIST			0x32	// <count 1>(<IP 4><PORT 2>)[count] server->client
#define OP_SEARCHRESULT			0x33	// <count 4>(<HASH 16><ID 4><PORT 2><1 Tag_set>)[count]
#define	OP_SERVERSTATUS			0x34	// <USER 4><FILES 4>
#define OP_CALLBACKREQUESTED	0x35	// <IP 4><PORT 2>
#define OP_CALLBACK_FAIL		0x36	// (null notverified)
#define OP_SERVERMESSAGE		0x38	// <len 2><Message len>
#define OP_CHAT_ROOM_REQUEST    0x39    // (deprecated not supported by server any longer)
#define OP_CHAT_BROADCAST       0x3A    // (deprecated not supported by server any longer)
#define OP_CHAT_USER_JOIN       0x3B    // (deprecated not supported by server any longer)
#define OP_CHAT_USER_LEAVE      0x3C    // (deprecated not supported by server any longer)
#define OP_CHAT_USER            0x3D    // (deprecated not supported by server any longer)
#define OP_IDCHANGE				0x40	// <NEW_ID 4>
#define OP_SERVERIDENT		    0x41	// <HASH 16><IP 4><PORT 2>{1 TAG_SET}
#define OP_FOUNDSOURCES			0x42	// <HASH 16><count 1>(<ID 4><PORT 2>)[count]
#define OP_USERS_LIST           0x43    // <count 4>(<HASH 16><ID 4><PORT 2><1 Tag_set>)[count]

//client <-> UDP server
#define	OP_GLOBSEARCHREQ2		0x92	// <search_tree>
#define OP_GLOBSERVSTATREQ		0x96	// (null)
#define	OP_GLOBSERVSTATRES		0x97	// <USER 4><FILES 4>
#define OP_GLOBSEARCHREQ		0x98	// <search_tree>
#define OP_GLOBSEARCHRES		0x99	// 
#define OP_GLOBGETSOURCES		0x9A	// <HASH 16>
#define OP_GLOBFOUNDSOURCES		0x9B	//
#define OP_GLOBCALLBACKREQ		0x9C	// <IP 4><PORT 2><client_ID 4>
#define OP_INVALID_LOWID		0x9E	// <ID 4>
#define	OP_SERVER_LIST_REQ		0xA0	// <IP 4><PORT 2>
#define OP_SERVER_LIST_RES		0xA1	// <count 1> (<ip 4><port 2>)[count]
#define OP_SERVER_DESC_REQ		0xA2	// (null)
#define OP_SERVER_DESC_RES		0xA3	// <name_len 2><name name_len><desc_len 2 desc_en>
#define OP_SERVER_LIST_REQ2		0xA4	// (null)

// client <-> client
#define	OP_HELLO				0x01	// 0x10<HASH 16><ID 4><PORT 2><1 Tag_set>
#define OP_SENDINGPART			0x46	// <HASH 16><von 4><bis 4><Daten len:(von-bis)>
#define	OP_REQUESTPARTS			0x47	// <HASH 16><von[3] 4*3><bis[3] 4*3>
#define OP_FILEREQANSNOFIL		0x48	// <HASH 16>
#define OP_END_OF_DOWNLOAD     	0x49    // <HASH 16>
#define OP_ASKSHAREDFILES		0x4A	// (null)
#define OP_ASKSHAREDFILESANSWER 0x4B	// <count 4>(<HASH 16><ID 4><PORT 2><1 Tag_set>)[count]
#define	OP_HELLOANSWER			0x4C	// <HASH 16><ID 4><PORT 2><1 Tag_set><SERVER_IP 4><SERVER_PORT 2>
#define OP_CHANGE_CLIENT_ID 	0x4D	// <ID_old 4><ID_new 4>
#define	OP_MESSAGE				0x4E	// <len 2><Message len>
#define OP_SETREQFILEID			0x4F	// <HASH 16>
#define	OP_FILESTATUS			0x50	// <HASH 16><count 2><status(bit array) len:((count+7)/8)>
#define OP_HASHSETREQUEST		0x51	// <HASH 16>
#define OP_HASHSETANSWER		0x52	// <count 2><HASH[count] 16*count>
#define	OP_STARTUPLOADREQ		0x54	// <HASH 16>
#define	OP_ACCEPTUPLOADREQ		0x55	// (null)
#define	OP_CANCELTRANSFER		0x56	// (null)	
#define OP_OUTOFPARTREQS		0x57	// (null)
#define OP_FILEREQUEST			0x58	// <HASH 16>	(more correctly file_name_request)
#define OP_FILEREQANSWER		0x59	// <HASH 16><len 4><NAME len>
#define OP_CHANGE_SLOT			0x5B	// <HASH 16>
#define OP_QUEUERANK			0x5C	// <wert  4> (slot index of the request)
#define OP_ASKSHAREDDIRS        0x5D    // (null)
#define OP_ASKSHAREDFILESDIR    0x5E    // <len 2><Directory len>
#define OP_ASKSHAREDDIRSANS     0x5F    // <count 4>(<len 2><Directory len>)[count]
#define OP_ASKSHAREDFILESDIRANS 0x60    // <len 2><Directory len><count 4>(<HASH 16><ID 4><PORT 2><1 Tag_set>)[count]
#define OP_ASKSHAREDDENIEDANS   0x61    // (null)


// extened prot client <-> extened prot client
#define	OP_EMULEINFO			0x01	//
#define	OP_EMULEINFOANSWER		0x02	//
#define OP_COMPRESSEDPART		0x40	//
#define OP_QUEUERANKING			0x60	// <RANG 2>
#define OP_FILEDESC				0x61	// <len 2><NAME len>
#define OP_REQUESTSOURCES		0x81	// <HASH 16>
#define OP_ANSWERSOURCES		0x82	//
#define OP_PUBLICKEY			0x85	// <len 1><pubkey len>
#define OP_SIGNATURE			0x86	// v1: <len 1><signature len>  v2:<len 1><signature len><sigIPused 1>
#define OP_SECIDENTSTATE		0x87	// <state 1><rndchallenge 4>
#define OP_REQUESTPREVIEW		0x90	// <HASH 16>
#define OP_PREVIEWANSWER		0x91	// <HASH 16><frames 1>{frames * <len 4><frame len>}

// extened prot client <-> extened prot client UDP
#define OP_REASKFILEPING		0x90	// <HASH 16>
#define OP_REASKACK				0x91	// <RANG 2>
#define OP_FILENOTFOUND			0x92	// (null)
#define OP_QUEUEFULL			0x93	// (null)
	
// server.met
#define ST_SERVERNAME			0x01	// <string>
#define ST_DESCRIPTION			0x0B	// <string>
#define ST_PING					0x0C	// <int>
#define ST_PREFERENCE			0x0E	// <int>
#define ST_FAIL					0x0D	// <int>
#define	ST_DYNIP				0x85
//#define ST_LASTPING			0x86	// <int> No longer used.
#define ST_MAXUSERS				0x87
#define ST_SOFTFILES			0x88
#define ST_HARDFILES			0x89
#define ST_LASTPING				0x90	// <int>
#define	ST_VERSION				0x91	// <string>
#define	ST_UDPFLAGS				0x92	// <int>

//file tags
#define FT_FILENAME				0x01
#define TAG_NAME				"\x01"
#define FT_FILESIZE				0x02	// <int>
#define TAG_SIZE				"\x02"
#define FT_FILETYPE				0x03	// <string>
#define TAG_TYPE				"\x03"
#define FT_FILEFORMAT			0x04	// <string>
#define TAG_FORMAT				"\x04"
#define FT_LASTSEENCOMPLETE		0x05
#define TAG_COLLECTION			"\x05"
#define	TAG_PART_PATH			"\x06"
#define	TAG_PART_HASH			"\x07"
#define FT_TRANSFERED			0x08	// <int>
#define	TAG_COPIED				"\x08"
#define FT_GAPSTART				0x09
#define	TAG_GAP_START			"\x09"
#define FT_GAPEND				0x0A
#define	TAG_GAP_END				"\x0A"
#define	TAG_DESCRIPTION			"\x0B"
#define	TAG_PING				"\x0C"
#define	TAG_FAIL				"\x0D"
#define	TAG_PREFERENCE			"\x0E"
#define TAG_PORT				"\x0F"
#define TAG_IP_ADDRESS			"\x10"
#define TAG_VERSION				"\x11"
#define FT_PARTFILENAME			0x12	// <string>
#define TAG_TEMPFILE			"\x12"
//#define FT_PRIORITY			0x13	// Not used anymore
#define TAG_PRIORITY			"\x13"
#define FT_STATUS				0x14
#define TAG_STATUS				"\x14"
#define FT_SOURCES				0x15
#define TAG_AVAILABILITY		"\x15"
#define FT_PERMISSIONS			0x16
#define TAG_QTIME				"\x16"
//#define FT_ULPRIORITY			0x17	// Not used anymore
#define TAG_PARTS				"\x17"
#define FT_DLPRIORITY			0x18	// Was 13
#define FT_ULPRIORITY			0x19	// Was 17
#define FT_KADLASTPUBLISHKEY	 0x20	// <uint32>
#define FT_KADLASTPUBLISHSRC	 0x21	// <uint32>
#define	TAG_MEDIA_ARTIST		"\xD0"	// <string>
#define	 FT_MEDIA_ARTIST		 0xD0	// <string>
#define	TAG_MEDIA_ALBUM			"\xD1"	// <string>
#define	 FT_MEDIA_ALBUM			 0xD1	// <string>
#define	TAG_MEDIA_TITLE			"\xD2"	// <string>
#define	 FT_MEDIA_TITLE			 0xD2	// <string>
#define	TAG_MEDIA_LENGTH		"\xD3"	// <uint32> !!!
#define	 FT_MEDIA_LENGTH		 0xD3	// <uint32> !!!
#define	TAG_MEDIA_BITRATE		"\xD4"	// <uint32>
#define	 FT_MEDIA_BITRATE		 0xD4	// <uint32>
#define	TAG_MEDIA_CODEC			"\xD5"	// <string>
#define	 FT_MEDIA_CODEC			 0xD5	// <string>
#define TAG_SERVERPORT			"\xFA"
#define TAG_SERVERIP			"\xFB"
#define TAG_SOURCEUPORT			"\xFC"
#define TAG_SOURCEPORT			"\xFD"
#define TAG_SOURCEIP			"\xFE"
#define TAG_SOURCETYPE			"\xFF"

//MORPH START - Added by SiRoB, ZZ Upload System
#define FT_POWERSHARE           "ZZUL_POWERSHARE"
//MORPH END - Added by SiRoB, ZZ Upload System

// additional media meta data tags from eDonkeyHybrid (note also the uppercase/lowercase)
#define	FT_ED2K_MEDIA_ARTIST	"Artist"	// <string>
#define	FT_ED2K_MEDIA_ALBUM		"Album"		// <string>
#define	FT_ED2K_MEDIA_TITLE		"Title"		// <string>
#define	FT_ED2K_MEDIA_LENGTH	"length"	// <string> !!!
#define	FT_ED2K_MEDIA_BITRATE	"bitrate"	// <uint32>
#define	FT_ED2K_MEDIA_CODEC		"codec"		// <string>
#define TAG_NSENT				"# Sent"
#define TAG_ONIP				"ip"
#define TAG_ONPORT				"port"

// statistic
#define FT_ATTRANSFERED			0x50
#define FT_ATREQUESTED			0x51
#define FT_ATACCEPTED			0x52
#define FT_CATEGORY				0x53
#define	FT_ATTRANSFEREDHI		0x54
// khaos::categorymod+
#define FT_CATRESUMEORDER		0x55
// khaos::categorymod-
// -khaos--+++>
#define FT_COMPRESSIONBYTES		0x56
#define FT_CORRUPTIONBYTES		0x57
// <-----khaos-
// khaos::categorymod+
#define FT_CATFILEGROUP			0x58
// khaos::categorymod-
// khaos::accuratetimerem+
#define	FT_SECONDSACTIVE		0x80
#define	FT_INITIALBYTES			0x81
#define FT_A4AFON				0x82
#define FT_A4AFOFF				0x83
// khaos::accuratetimerem-

//MORPH - Added by IceCream, SLUGFILLER: Spreadbars
#define FT_SPREADSTART			0x70
#define FT_SPREADEND			0x71
#define FT_SPREADCOUNT			0x72
//MORPH - Added by IceCream, SLUGFILLER: Spreadbars
#define FT_LASTUSED				0x95	// <uint32> // EastShare - Added by TAHO, .met file control

#define CT_NAME					0x01
#define CT_VERSION				0x11
#define	CT_PORT					0x0f
#define	CT_EMULE_RESERVED1		0xf0
#define	CT_EMULE_RESERVED2		0xf1
#define	CT_EMULE_RESERVED3		0xf2
#define	CT_EMULE_RESERVED4		0xf3
#define	CT_EMULE_RESERVED5		0xf4
#define	CT_EMULE_RESERVED6		0xf5
#define	CT_EMULE_RESERVED7		0xf6
#define	CT_EMULE_RESERVED8		0xf7
#define	CT_EMULE_RESERVED9		0xf8
#define	CT_EMULE_UDPPORTS		0xf9
#define	CT_EMULE_MISCOPTIONS1	0xfa
#define	CT_EMULE_VERSION		0xfb
#define CT_EMULE_RESERVED10		0xfc
#define CT_EMULE_RESERVED11		0xfd
#define CT_EMULE_RESERVED12		0xfe
#define CT_EMULE_RESERVED13		0xff

#define MP_LIST_REQUESTED_FILES		17015 //MORPH START - Added by Yun.SF3, List Requested Files

//MORPH START - Added by SiRoB, About Popup Open File Folder entry
#define	MP_OPENFILEFOLDER			10099
//MORPH END - Added by SiRoB, About Popup Open File Folder entry
//MORPH START - Added by IceCream, copy feedback feature
#define	MP_COPYFEEDBACK			10100
#define MP_COPYFEEDBACK_US		10101
//MORPH END   - Added by IceCream, copy feedback feature

#define MP_MESSAGE				10102
#define MP_DETAIL				10103
#define MP_ADDFRIEND			10104
#define MP_REMOVEFRIEND			10105
#define MP_SHOWLIST				10106
#define MP_FRIENDSLOT			10107
#define MP_CANCEL				10201
#define MP_STOP					10202
#define MP_PAUSE				10203
#define MP_RESUME				10204
#define	MP_CLEARCOMPLETED		10205
#define	MP_OPEN					10206
#define	MP_PREVIEW				10207

#define MP_CMT					10208

#define MP_HM_CON				10209
#define MP_HM_SRVR				10210
#define MP_HM_TRANSFER			10211
#define MP_HM_SEARCH			10212
#define MP_HM_FILES				10213
#define MP_HM_MSGS				10214
#define MP_HM_IRC				10215
#define MP_HM_STATS				10216
#define MP_HM_PREFS				10217
#define MP_HM_OPENINC			10218
#define MP_HM_EXIT				10219
#define MP_ALL_A4AF_TO_THIS		10220
#define MP_ALL_A4AF_TO_OTHER	10221
#define MP_ALL_A4AF_AUTO		10222
#define MP_SWAP_A4AF_TO_THIS	10223
#define MP_SWAP_A4AF_TO_OTHER	10224
#define MP_META_DATA			10225
#define MP_BOOT					10226
#define MP_HM_CONVERTPF			10227

#define MP_HM_LINK1				10230
#define MP_HM_LINK2				10231
#define MP_HM_LINK3				10232
#define MP_HM_SCHEDONOFF		10233

#define MP_SELECTTOOLBARBITMAPDIR 10234
#define MP_SELECTTOOLBARBITMAP	10235
#define MP_NOTEXTLABELS			10236
#define MP_TEXTLABELS			10237
#define MP_TEXTLABELSONRIGHT	10238
#define	MP_CUSTOMIZETOOLBAR		10239
#define	MP_SELECT_SKIN_FILE		10240
#define	MP_SELECT_SKIN_DIR		10241

//MORPH START - Added by SiRoB, ZZ Upload System, Kademlia 40f26
#define MP_POWERSHARE_ON        10242
#define MP_POWERSHARE_OFF       10243
#define MP_POWERSHARE_AUTO      10244
//MORPH END - Added by SiRoB, ZZ Upload System, Kademlia 40f26

#define Irc_Version				"(SMIRCv00.66)"
#define Irc_Op					10240
#define Irc_DeOp				10241
#define Irc_Voice				10242
#define Irc_DeVoice				10243
#define Irc_HalfOp				10244
#define Irc_DeHalfOp			10245
#define Irc_Kick				10246
#define Irc_Slap				10247
#define Irc_Join				10248
#define Irc_Close				10249
#define Irc_Priv				10250
#define Irc_AddFriend			10251
#define	Irc_SendLink			10252
#define Irc_SetSendLink			10253
#define	Irc_Owner				10254
#define Irc_DeOwner				10255
#define	Irc_Protect				10256
#define Irc_DeProtect			10257

#define MP_PRIOVERYLOW			10300
#define MP_PRIOLOW				10301
#define MP_PRIONORMAL			10302
#define MP_PRIOHIGH				10303
#define MP_PRIOVERYHIGH			10304
#define MP_PRIOAUTO				10317
#define MP_GETED2KLINK			10305
#define MP_GETHTMLED2KLINK		10306
#define	MP_GETSOURCEED2KLINK	10299
#define MP_METINFO				10307
#define MP_PERMALL				10308
#define MP_PERMFRIENDS			10309
#define MP_PERMNONE				10310
#define MP_CONNECTTO			10311
#define MP_REMOVE				10312
#define MP_REMOVEALL			10313
#define MP_REMOVESELECTED		10314
#define MP_UNBAN				10315
#define MP_ADDTOSTATIC			10316
#define MP_CLCOMMAND			10317
#define MP_REMOVEFROMSTATIC		10318
#define MP_VIEWFILECOMMENTS		10319
#define MP_VERSIONCHECK			10320
#define MP_CAT_ADD				10321
#define MP_CAT_EDIT				10322
#define MP_CAT_REMOVE			10323

#define MPG_DELETE				10325
#define	MP_COPYSELECTED			10326
#define	MP_SELECTALL			10327
#define	MP_AUTOSCROLL			10328
//#define MP_RESUMENEXT			10329
#define MPG_ALTENTER			10330
#define MPG_F2					10331

// khaos::categorymod+
#define MP_CAT_SHOWHIDEPAUSED	10335
#define MP_CAT_SETRESUMEORDER	10336
#define	MP_CAT_ORDERAUTOINC		10337
#define MP_CAT_ORDERSTEPTHRU	10338
#define MP_CAT_ORDERALLSAME		10339
#define MP_CAT_RESUMENEXT		10340
#define	MP_CAT_PAUSELAST		10341
#define MP_CAT_STOPLAST			10342
#define MP_CAT_MERGE			10343
#define MP_SETFILEGROUP			10344

// For the downloadlist menu
#define MP_FORCEA4AF			10350
#define	MP_FORCEA4AFONFLAG		10351
#define MP_FORCEA4AFOFFFLAG		10352

// 10353 to 10355 reserved for A4AF menu items.
#define MP_CAT_A4AF			10353
// khaos::categorymod-

#define MP_GETHOSTNAMESOURCEED2KLINK	10361	// itsonlyme: hostnameSource

// quick-speed changer
#define MP_QS_U10				10501
#define MP_QS_U20				10502
#define MP_QS_U30				10503
#define MP_QS_U40				10504
#define MP_QS_U50				10505
#define MP_QS_U60				10506
#define MP_QS_U70				10507
#define MP_QS_U80				10508
#define MP_QS_U90				10509
#define MP_QS_U100				10510
#define MP_QS_UPC				10511
#define MP_QS_UP10				10512
#define MP_QS_UPL				10513
#define MP_QS_D10				10521
#define MP_QS_D20				10522
#define MP_QS_D30				10523
#define MP_QS_D40				10524
#define MP_QS_D50				10525
#define MP_QS_D60				10526
#define MP_QS_D70				10527
#define MP_QS_D80				10528
#define MP_QS_D90				10529
#define MP_QS_D100				10530
#define MP_QS_DC				10531
#define MP_QS_DL				10532
#define MP_QS_PA				10533
#define MP_QS_UA				10534

#define MP_WEBURL				10600
// reserve some for weburls!

#define MP_ASSIGNCAT			10700
// reserve some for categories!
#define MP_SCHACTIONS			10800
// reserve some for schedules
#define MP_CAT_SET0				10900
// reserve some for change all-cats (about 20)
#define MP_TOOLBARBITMAP		10950
// reserve max 50
#define	MP_SKIN_PROFILE			11000
// reserve max 50

// emule tagnames
#define ET_COMPRESSION			0x20
#define ET_UDPPORT				0x21
#define ET_UDPVER				0x22
#define ET_SOURCEEXCHANGE		0x23
#define ET_COMMENTS				0x24
#define ET_EXTENDEDREQUEST		0x25
#define ET_COMPATIBLECLIENT		0x26
#define ET_FEATURES				0x27
//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
#define ET_MOD_VERSION 			0x55 // Maella -Support for tag ET_MOD_VERSION 0x55-
//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-

#define ET_L2HAC				0x3E //<<-- enkeyDEV(th1) -L2HAC-

// emuleapp <-> emuleapp
#define OP_ED2KLINK				12000
#define OP_CLCOMMAND			12001
	

// GUI-Protocol TCP (ed2k_gui + java_gui)
#define CO_SERVER_LIST			0xAA	// C-G 170 //      filelist: server list
#define CO_FRIEND_LIST			0xAB	// C-G 171 //      filelist: friend list
#define CO_SHARED_DIRS			0xAC	// C-G 172 //      w16: num directories	//      string[]: directory name
#define CO_SHARED_FILES			0xAD	// C-G 173 //	   filelist: shared files
#define CO_GAP_DETAILS			0xAE	// C-G 174 <HASH 16><count 2>(<GAP_START 4><GAP_END 4><GAP_STAT 2>)[count]
#define CO_CLIENT_STATS			0xAF	// C-G 175
// float: free space on temp drive in MB
// float: free space in incoming in MB
// float: space needed by downloads in MB
// w32:   client ID
// w16:   no. of connections used currently
// w16:   no. of people on queue
#define CO_STATUS_MSG			0xB4	// C-G 180	//      string: message
#define CO_ERROR_MSG			0xB5	// C-G 181	//      string: error message
#define CO_CONNECTED_TO			0xB6	// C-G 182	//      string: server name
#define CO_DISCONNECTED			0xB7	// C-G 183
#define CO_SET_SERVER_STATS		0xB8	// C-G 184	//      w32: files w32: users
#define CO_EXTENDING_SEARCH		0xB9	// C-G 185	//      string: server name
#define CO_SEARCH_RESULT		0xBA	// C-G 186	//      meta: result meta
#define CO_NEW_SEARCH_RESULTS	0xBB	// C-G 187	//      filelist: result list	//      w8: more or not (0 or 1)
#define CO_NEW_DOWNLOAD			0xBC	// C-G 188	// meta: download file w8: initial preference (???) i4string: temp file name
#define CO_REMOVE_DOWNLOAD		0xBD	// C-G 189	//      hash: fileID
#define CO_NEW_UPLOAD			0xBE    // C-G 190	// string: file name	meta: upload user
#define CO_REMOVE_UPLOAD		0xBF    // C-G 191	//      hash: uploader/user ID
#define CO_NEW_UPLOAD_SLOT		0xC0    // C-G 192	// w32: slot ID string: uploader name
#define CO_REMOVE_UPLOAD_SLOT	0xC1    // C-G 193	//      w32: slot ID
#define CO_USER_FILES			0xC2    // C-G 194	//      filelist: users filelist
#define CO_HASHING				0xC3    // C-G 195	//      string: filename
#define CO_FRIEND_LIST_UPDATE	0xC4    // C-G 196     	// the friend list needs to be updated
#define CO_DOWNLOAD_STATUS		0xC5	// C-G 197	<cnt 2>(<ID 2><STAT 1><SPEED kb/sec float 4><TRANSFERED 4><AVAIL% 1><SOUCES 1>)[cnt]
#define CO_UPLOAD_STATUS		0xC6	// C-G 198	<cnt 2>(<ID 2><SPEED kb/sec float 4>)[cnt]
#define CO_OPTIONS				0xC7	// C-G 199     	// options follow
/* w16     clientversion
float   max down
float   max up
w16     doorport
w16     max connections
string  client name
string  temp dir
string  incoming dir
w8      autoConnect
w8      autoServRemove
w8      primsgallow
w8      savecorrupt
w8      verifyCancel
w16     adminDoorPort
w32     core build date
float   line down speed */
#define CO_CONNECT              0xC8	// G-C
#define CO_DISCONNECT           0xC9	// G-C
#define CO_SEARCH               0xCA	// G-C
#define CO_EXSEARCH             0xCB	// G-C
#define CO_MORESEARCH           0xCC	// G-C
#define CO_SEARCH_USER          0xCD	// G-C
#define CO_EXSEARCH_USER        0xCE	// G-C
#define CO_DOWNLOAD             0xCF	// G-C
#define CO_PAUSE_DOWNLOAD       0xD0	// G-C		<HASH 16>
#define CO_RESUME_DOWNLOAD      0xD1	// G-C		<HASH 16>
#define CO_CANCEL_DOWNLOAD      0xD2	// G-C		<HASH 16>
#define CO_SETPRI_DOWNLOAD      0xD3	// G-C		<HASH 16><PRI 1>
#define CO_VIEW_FRIEND_FILES    0xD4	// G-C
#define CO_GET_SERVERLIST       0xD5	// G-C		(null)
#define CO_GET_FRIENDLIST       0xD6	// G-C		(null)
#define CO_GET_SHARE_DIRS       0xD7	// G-C		(null)
#define CO_SET_SHARE_DIRS       0xD8	// G-C		(null)
#define CO_START_DL_STATUS      0xD9	// G-C		(null)
#define CO_STOP_DL_STATUS       0xDA	// G-C		(null)
#define CO_START_UL_STATUS      0xDB	// G-C		(null)
#define CO_STOP_UL_STATUS       0xDC	// G-C		(null)
#define CO_DELETE_SERVER        0xDD	// G-C		<IP 4><PORT 2>
#define CO_ADD_SERVER           0xDE	// G-C		<IP 4><PORT 2>
#define CO_SETPRI_SERVER        0xDF	// G-C		<IP 4><PORT 2><PRI 1>
#define CO_GET_SHARE_FILES      0xE0	// G-C
#define CO_GET_OPTIONS          0xE1	// G-C
#define CO_REQ_NEW_DOWNLOAD     0xE2	// G-C	226	<HASH 16><IP 4><PORT 4><Tag_set {min size+name}>
#define CO_GET_GAP_DETAILS		0xE3	// G-C	227	<HASH 16><count 2>
#define CO_GET_CLIENT_STATS		0xE4	// G-C 	228	<FREE_TMP_MB float 4><FREE_IN_MB float 4><NEED MB float 4><ID 4><conn 2><queue 2>

// KADEMLIA (opcodes) (udp)
#define KADEMLIA_BOOTSTRAP_REQ	0x00	// <PEER (sender) [25]>
#define KADEMLIA_BOOTSTRAP_RES	0x08	// <CNT [2]> <PEER [25]>*(CNT)
#define KADEMLIA_HELLO_REQ	 	0x10	// <PEER (sender) [25]>
#define KADEMLIA_HELLO_RES     	0x18	// <PEER (reciever) [25]>
#define KADEMLIA_REQ		   	0x20	// <TYPE [1]> <HASH (target) [16]> <HASH (reciever) 16>
#define KADEMLIA_RES			0x28	// <HASH (target) [16]> <CNT> <PEER [25]>*(CNT)
#define KADEMLIA_SEARCH_REQ		0x30	// <HASH (key) [16]> <ext 0/1 [1]> <SEARCH_TREE>[ext]
#define KADEMLIA_SEARCH_RES		0x38	// <HASH (key) [16]> <CNT1 [2]> (<HASH (answer) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
#define KADEMLIA_PUBLISH_REQ	0x40	// <HASH (key) [16]> <CNT1 [2]> (<HASH (target) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
#define KADEMLIA_PUBLISH_RES	0x48	// <HASH (key) [16]>
#define KADEMLIA_FIREWALLED_REQ	0x50	// <TCPPORT (sender) [2]>
#define KADEMLIA_FIREWALLED_RES	0x58	// <IP (sender) [4]>
#define KADEMLIA_FIREWALLED_ACK	0x59	// (null)

// KADEMLIA (parameter)
#define KADEMLIA_FIND_VALUE		0x02
#define KADEMLIA_STORE			0x04
#define KADEMLIA_FIND_NODE		0x0B

#define MP_DELFILE			10333 // eastshare added by linekin, TBH delete shared file


