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
//Morph Start - modified by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
#define CREDITFILE_VERSION_30_DYN		0x81	// Moonlight: Dynamic Transportable CreditStruct.
#define CREDITFILE_VERSION_30_SUQWTv2	0x80	// Moonlight: SUQWT CreditStruct v2.
#define CREDITFILE_VERSION_30_SUQWTv1	0x13	// Moonlight: SUQWT CreditStruct v1.
#define CREDITFILE_VERSION_30			0x12
//#define CREDITFILE_VERSION		0x12//original commented out
#define CREDITFILE_VERSION_29			0x11
#define CREDITFILE_VERSION				CREDITFILE_VERSION_30_SUQWTv2	// Define the current version number.
//Morph End - modified by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
#define COMPILE_DATE			__DATE__
#define COMPILE_TIME			__TIME__
#ifdef _DEBUG
#define EMULE_GUID				_T("EMULE-{4EADC6FC-516F-4b7c-9066-97D893649570}-DEBUG")
#else
#define EMULE_GUID				_T("EMULE-{4EADC6FC-516F-4b7c-9066-97D893649570}")
#endif

#define	SEC2MS(sec)		((sec)*1000)
#define	MIN2MS(min)		SEC2MS((min)*60)
#define	HR2MS(hr)		MIN2MS((hr)*60)
#define SEC(sec)		(sec)
#define	MIN2S(min)		((min)*60)
#define	HR2S(hr)		MIN2S((hr)*60)

// MOD Note: Do not change this part - Merkur
#define UDPSEARCHSPEED			SEC2MS(1)	//1 sec - if this value is too low you will miss sources
#define MAX_RESULTS				100		// max global search results
#define	MAX_MORE_SEARCH_REQ		5		// this gives a max. total search results of (1+5)*201 = 1206 or (1+5)*300 = 1800
#define MAX_CLIENTCONNECTIONTRY	2
#define CONNECTION_TIMEOUT		SEC2MS(80)	//40 secs - set his lower if you want less connections at once, set it higher if you have enough sockets (edonkey has its own timout too, so a very high value won't effect this)
#define	FILEREASKTIME			MIN2MS(29)	//29 mins
#define SERVERREASKTIME			MIN2MS(15)	//15 mins - don't set this too low, it wont speed up anything, but it could kill emule or your internetconnection
#define UDPSERVERREASKTIME		MIN2MS(30)	//30 mins
#define SOURCECLIENTREASKS		MIN2MS(40)	//40 mins
#define SOURCECLIENTREASKF		MIN2MS(5)	//5 mins
#define KADEMLIAASKTIME			SEC2MS(1)	//1 second
#define KADEMLIATOTALFILE		5		//Total files to search sources for.
#define KADEMLIAREASKTIME		HR2MS(1)	//1 hour
#define KADEMLIAPUBLISHTIME		SEC2MS(2)	//2 second
#define KADEMLIATOTALSTORESRC	2		//Total hashes to store.
#define KADEMLIATOTALSTOREKEY	1		//Total hashes to store.
#define KADEMLIAREPUBLISHTIMES	HR2S(5)		//5 hours
#define KADEMLIAREPUBLISHTIMEK	HR2S(10)	//10 hours
#define KADEMLIADISCONNECTDELAY	MIN2S(20)	//20 mins
#define	KADEMLIAMAXINDEX		5000		//Total index per key
#define KADEMLIAMAXSOUCEPERFILE	300			//Max number of sources per file in index.

#define ED2KREPUBLISHTIME		MIN2MS(1)	//1 min
#define MINCOMMONPENALTY		4
#define UDPSERVERSTATTIME		SEC2MS(5)	//5 secs
#define UDPSERVSTATREASKTIME	HR2S(4)		//4 hours
#define	UDPSERVERPORT			4665	//default udp port
#define UDPMAXQUEUETIME			SEC2MS(30)	//30 Seconds
#define RSAKEYSIZE				384		//384 bits
#define	MAX_SOURCES_FILE_SOFT	500
#define	MAX_SOURCES_FILE_UDP	50
#define SESSIONMAXTRANS			(9.3*1024*1024) // 9.3 Mbytes. "Try to send complete chunks" always sends this amount of data
#define SESSIONMAXTIME			HR2MS(1)	//1 hour
// MOD Note: end

#define CONFIGFOLDER			"config\\"
#define MAXCONPER5SEC			20	
#define MAXCON5WIN9X			10
#define	UPLOAD_CHECK_CLIENT_DR	2048
#define	UPLOAD_CLIENT_DATARATE	4000		// uploadspeed per client in bytes - you may want to adjust this if you have a slow connection or T1-T3 ;)
//#define	MAX_UP_CLIENTS_ALLOWED	100		// max. clients allowed regardless UPLOAD_CLIENT_DATARATE or any other factors. Don't set this too low, use DATARATE to adjust uploadspeed per client
#define	MIN_UP_CLIENTS_ALLOWED	1		// min. clients allowed to download regardless UPLOAD_CLIENT_DATARATE or any other factors. Don't set this too high
#define MINNUMBEROFTRICKLEUPLOADS 0
#define MINWAITBEFOREOPENANOTHERSLOTMS 1000
#define DOWNLOADTIMEOUT			SEC2MS(100)
#define CONSERVTIMEOUT			SEC2MS(25)	// agelimit for pending connection attempts
#define RARE_FILE				50
#define BADCLIENTBAN			4
#define	MIN_REQUESTTIME			MIN2MS(10) 
#define	MAX_PURGEQUEUETIME		HR2MS(1) 
#define PURGESOURCESWAPSTOP		MIN2MS(15)	// (15 mins), how long forbid swapping a source to a certain file (NNP,...)
#define CONNECTION_LATENCY		22050	// latency for responces
#define MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE   1000
#define MINWAIT_BEFORE_ULDISPLAY_WINDOWUPDATE   1000
#define MAXAVERAGETIME			SEC2MS(40) //millisecs
#define CLIENTBANTIME			HR2MS(2)	// 2h
#define TRACKED_CLEANUP_TIME	HR2MS(1)	// 1 hour
#define KEEPTRACK_TIME			HR2MS(2)	// 2h	//how long to keep track of clients which were once in the uploadqueue
#define LOCALSERVERREQUESTS		20000		// only one local src request during this timespan (WHERE IS THIS USED?)
#define DISKSPACERECHECKTIME	MIN2MS(15)	// SLUGFILLER: checkDiskspace

// you shouldn't change anything here if you are not really sure, or emule will probaly not work
//MOPRH START - Added by SiRoB, ZZ Upload System 20030824-2238
#define SESSIONAMOUNT			(10*1024*1024) // 10 Mbytes. "Try to send complete chunks" always sends this amount of data
//MOPRH END   - Added by SiRoB, ZZ Upload System 20030824-2238
#define	MAXFRAGSIZE				1300
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
#define OP_REQUESTFILENAME		0x58	// <HASH 16>	(more correctly file_name_request)
#define OP_REQFILENAMEANSWER	0x59	// <HASH 16><len 4><NAME len>
#define OP_CHANGE_SLOT			0x5B	// <HASH 16>
#define OP_QUEUERANK			0x5C	// <wert  4> (slot index of the request)
#define OP_ASKSHAREDDIRS        0x5D    // (null)
#define OP_ASKSHAREDFILESDIR    0x5E    // <len 2><Directory len>
#define OP_ASKSHAREDDIRSANS     0x5F    // <count 4>(<len 2><Directory len>)[count]
#define OP_ASKSHAREDFILESDIRANS 0x60    // <len 2><Directory len><count 4>(<HASH 16><ID 4><PORT 2><1 Tag_set>)[count]
#define OP_ASKSHAREDDENIEDANS   0x61    // (null)

// this 'identifier' is used for referencing shared part (incomplete) files with the OP_ASKSHAREDDIRS and related opcodes
// it was introduced with eDonkeyHybrid and is considered as part of the protocol.
#define OP_INCOMPLETE_SHARED_FILES "!Incomplete Files"

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
#define OP_MULTIPACKET			0x92
#define OP_MULTIPACKETANSWER	0x93

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
#define FT_FILENAME				 0x01
#define TAG_NAME				"\x01"
#define FT_FILESIZE				 0x02	// <int>
#define TAG_SIZE				"\x02"
#define FT_FILETYPE				 0x03	// <string>
#define TAG_TYPE				"\x03"
#define FT_FILEFORMAT			 0x04	// <string>
#define TAG_FORMAT				"\x04"
#define FT_LASTSEENCOMPLETE		 0x05
#define TAG_COLLECTION			"\x05"
#define	TAG_PART_PATH			"\x06"
#define	TAG_PART_HASH			"\x07"
#define FT_TRANSFERED			 0x08	// <int>
#define	TAG_COPIED				"\x08"
#define FT_GAPSTART				 0x09
#define	TAG_GAP_START			"\x09"
#define FT_GAPEND				 0x0A
#define	TAG_GAP_END				"\x0A"
#define	TAG_DESCRIPTION			"\x0B"
#define	TAG_PING				"\x0C"
#define	TAG_FAIL				"\x0D"
#define	TAG_PREFERENCE			"\x0E"
#define TAG_PORT				"\x0F"
#define TAG_IP_ADDRESS			"\x10"
#define TAG_VERSION				"\x11"
#define FT_PARTFILENAME			 0x12	// <string>
#define TAG_TEMPFILE			"\x12"
//#define FT_PRIORITY			 0x13	// Not used anymore
#define TAG_PRIORITY			"\x13"
#define FT_STATUS				 0x14
#define TAG_STATUS				"\x14"
#define FT_SOURCES				 0x15
#define TAG_AVAILABILITY		"\x15"
#define FT_PERMISSIONS			 0x16
#define TAG_QTIME				"\x16"
//#define FT_ULPRIORITY			 0x17	// Not used anymore
#define TAG_PARTS				"\x17"
#define FT_DLPRIORITY			 0x18	// Was 13
#define FT_ULPRIORITY			 0x19	// Was 17
#define FT_KADLASTPUBLISHKEY	 0x20	// <uint32>
#define FT_KADLASTPUBLISHSRC	 0x21	// <uint32>
#define	FT_FLAGS				 0x22
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
#define TAG_CLIENTLOWID			"\xF9"	// <uint32>
#define TAG_SERVERPORT			"\xFA"	// <uint16>
#define TAG_SERVERIP			"\xFB"	// <uint32>
#define TAG_SOURCEUPORT			"\xFC"	// <uint16>
#define TAG_SOURCEPORT			"\xFD"	// <uint16>
#define TAG_SOURCEIP			"\xFE"	// <uint32>
#define TAG_SOURCETYPE			"\xFF"	// <uint8>

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

//MORPH START - Added by SiRoB, ZZ Upload System
#define FT_POWERSHARE           "ZZUL_POWERSHARE"
//MORPH END - Added by SiRoB, ZZ Upload System

//MORPH START - Added by SiRoB, HIDEOS
#define FT_HIDEOS				"HIDEOS"
#define FT_SELECTIVE_CHUNK		"SELECT_CHUNK"
//MORPH END   - Added by SiRoB, HIDEOS

//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
#define FT_SHAREONLYTHENEED		"SHARE_ONLY_THE_NEED"
//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED

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

// emule tagnames
#define ET_COMPRESSION			0x20
#define ET_UDPPORT				0x21
#define ET_UDPVER				0x22
#define ET_SOURCEEXCHANGE		0x23
#define ET_COMMENTS				0x24
#define ET_EXTENDEDREQUEST		0x25
#define ET_COMPATIBLECLIENT		0x26
#define ET_FEATURES				0x27
#define ET_MOD_VERSION 			0x55

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
#define KADEMLIA_BUDDY_REQ		0x51	// <TCPPORT (sender) [2]>
#define KADEMLIA_BUDDY_CON		0x52	//
#define KADEMLIA_BUDDY_ACK		0x57	// <TCPPORT (sender) [2]>
#define KADEMLIA_FIREWALLED_RES	0x58	// <IP (sender) [4]>
#define KADEMLIA_FIREWALLED_ACK	0x59	// (null)

// KADEMLIA (parameter)
#define KADEMLIA_FIND_VALUE		0x02
#define KADEMLIA_STORE			0x04
#define KADEMLIA_FIND_NODE		0x0B
