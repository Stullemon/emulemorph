#pragma once

#ifndef WEBCACHE_H
#define WEBCACHE_H

#include <list>

#define WC_TAG_WEBCACHENAME	0x67		// multipacket tag for webcache name
//#define WC_TAG_WEBCACHEID	0x68		// multipacket tag for webcache ID, not needed anymore
#define WC_TAG_MASTERKEY	0x68		// Superlexx - encryption - multipacket tag for the master key
#define WC_TAG_VOODOO		0x69		// hello packet tag: magic number == 'WebC' for pre-1.1 (unsupported), 'ARC4' for 1.1
#define WC_TAG_FLAGS		0x6A		// webcache flags tag
//#define	WC_TAG_REQ_OHCBS	0x6B		// multipacket tag for OHCB request

#define WC_FLAGS_INFO_NEEDED	0x01
#define WC_FLAGS_UDP			0x02		// support for ohcbs over udp
#define WC_FLAGS_NO_OHCBS		0x04		// support for OP_DONT_SEND_OHCBS
#define	WC_FLAGS_MULTI_OHCBS	0x08		// support for multi-OHCB-packets

#define	WC_KEYLENGTH		(128/8)		// length of the cipher key (bytes)
#define WC_B64_KEYLENGTH	22			// length of base64-encoded key

#define WC_OHCB_PACKET_SIZE		50		// length of an OHCB packet
#define	WC_NR_OF_XPRESS_OHCBS	8		// number of OHCBs that should be sent ASAP
#define WC_MAX_OHCBS_IN_UDP_PACKET 10	// maximal number of OHCBs in a packet that is to be sent via UDP
#define WC_MULTI_OHCB_SEND_TIME	MIN2MS(10) // time to pass between regular sending of MultiOHCB packets
#define WC_SENDOHCBS_INTERVAL	SEC2MS(10) // time between scanning the known client list for next multi-OHCB-recipients
#define	WC_MAX_MFR_AGE			3600000	// MRFs expire after 1 hour

#define WC_SOCKETEXTRATIMEOUT	300	// 5 minutes extra timeout when the proxy waits until the whole 180kB block is transferred

class CUpDownClient; //JP proxy configuration test
extern CUpDownClient* ProxyTestClient; //JP proxy configuration test

class CWebCachedBlockList;
extern uint32 ProxyConnectionCount;
										
uint32 ResolveWebCacheName();
bool AllowProxyConnection();

// from Superlexx - CWebMule
CString	WebCacheIndex2Name(int index);
int	GetWebCacheIndex(CString webcachename);
int	AddWebCache(CString webcachename);

#ifdef _DEBUG
#define WCREASKMS			20*1000				// minimal DNS backward lookup interval, 20s for debugging
#else
#define WCREASKMS			24*3600*1000		// minimal DNS backward lookup interval, 24h for release builds
#endif //_DEBUG

// proxy autodetect
struct WCInfo_Struct{
   CString  identifier;
   CString  webcache;
   CString	port;
   CString	isp;
	CString	country;
	CString location;
   CString	blockLimit;
   CString	extraTimeout;
   CString	cachesLocal;
   CString	persistentconns;
	CString	comment;
	CString	active;
	CString trustLevel;
};

void AutodetectWebcache();
bool DetectWebCache(WCInfo_Struct* detectedWebcache, uint8 attempt = 1);
//bool DetectWebCache(WCInfo_Struct* detectedWebcache);
CString	ReverseDnsLookupForWebCache(DWORD dwIP);

void GenerateKey (byte* key);	// generates a key for encryption

// URL-compatible Base64 en/decoder
	BOOL WC_b64_Encode( const byte* source, int len, CStringA& destination );
	BOOL WC_b64_Decode( const CStringA& source, byte* destination, int dest_len );
// Base64 en/decoder end

//JP proxy configuration test
bool PingviaProxy(CString WebCacheName, uint16 WebCachePort);

#endif //WEBCACHE_H