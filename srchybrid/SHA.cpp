/*
 ---------------------------------------------------------------------------
 Copyright (c) 2002, Dr Brian Gladman <brg@gladman.me.uk>, Worcester, UK.
 All rights reserved.

 LICENSE TERMS

 The free distribution and use of this software in both source and binary 
 form is allowed (with or without changes) provided that:

   1. distributions of this source code include the above copyright 
      notice, this list of conditions and the following disclaimer;

   2. distributions in binary form include the above copyright
      notice, this list of conditions and the following disclaimer
      in the documentation and/or other associated materials;

   3. the copyright holder's name is not used to endorse products 
      built using this software without specific written permission. 

 ALTERNATIVELY, provided that this notice is retained in full, this product
 may be distributed under the terms of the GNU General Public License (GPL),
 in which case the provisions of the GPL apply INSTEAD OF those given above.
 
 DISCLAIMER

 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness 
 and/or fitness for purpose.
 ---------------------------------------------------------------------------
 Issue Date: 30/11/2002

 This is a byte oriented version of SHA1 that operates on arrays of bytes
 stored in memory. It runs at 22 cycles per byte on a Pentium P4 processor
*/

#include "StdAfx.h"
#include "SHA.h"


CSHA::CSHA()
{
	Reset();
}

CSHA::~CSHA()
{
}

/*
    To obtain the highest speed on processors with 32-bit words, this code 
    needs to determine the order in which bytes are packed into such words.
    The following block of code is an attempt to capture the most obvious 
    ways in which various environemnts specify their endian definitions. 
    It may well fail, in which case the definitions will need to be set by 
    editing at the points marked **** EDIT HERE IF NECESSARY **** below.
*/
#define SHA_LITTLE_ENDIAN   1234 /* byte 0 is least significant (i386) */
#define SHA_BIG_ENDIAN      4321 /* byte 0 is most significant (mc68k) */
#define PLATFORM_BYTE_ORDER	SHA_LITTLE_ENDIAN

#define rotl32(x,n) (((x) << n) | ((x) >> (32 - n)))

#if (PLATFORM_BYTE_ORDER == SHA_BIG_ENDIAN)
#define swap_b32(x) (x)
#elif defined(bswap_32)
#define swap_b32(x) bswap_32(x)
#else
#define swap_b32(x) ((rotl32((x), 8) & 0x00ff00ff) | (rotl32((x), 24) & 0xff00ff00))
#endif

#define SHA1_MASK   (SHA1_BLOCK_SIZE - 1)

/* reverse byte order in 32-bit words   */

#define ch(x,y,z)       (((x) & (y)) ^ (~(x) & (z)))
#define parity(x,y,z)   ((x) ^ (y) ^ (z))
#define maj(x,y,z)      (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

/* A normal version as set out in the FIPS. This version uses   */
/* partial loop unrolling and is optimised for the Pentium 4    */

#define rnd(f,k)    \
    t = a; a = rotl32(a,5) + f(b,c,d) + e + k + w[i]; \
    e = d; d = c; c = rotl32(b, 30); b = t

void CSHA::Compile()
{
	DWORD    w[80], i, a, b, c, d, e, t;

    /* note that words are compiled from the buffer into 32-bit */
    /* words in big-endian order so an order reversal is needed */
    /* here on little endian machines                           */
    for(i = 0; i < SHA1_BLOCK_SIZE / 4; ++i)
        w[i] = swap_b32(m_nBuffer[i]);

    for(i = SHA1_BLOCK_SIZE / 4; i < 80; ++i)
        w[i] = rotl32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

    a = m_nHash[0];
    b = m_nHash[1];
    c = m_nHash[2];
    d = m_nHash[3];
    e = m_nHash[4];

    for(i = 0; i < 20; ++i)
    {
        rnd(ch, 0x5a827999);    
    }

    for(i = 20; i < 40; ++i)
    {
        rnd(parity, 0x6ed9eba1);
    }

    for(i = 40; i < 60; ++i)
    {
        rnd(maj, 0x8f1bbcdc);
    }

    for(i = 60; i < 80; ++i)
    {
        rnd(parity, 0xca62c1d6);
    }

    m_nHash[0] += a; 
    m_nHash[1] += b; 
    m_nHash[2] += c; 
    m_nHash[3] += d; 
    m_nHash[4] += e;
}

void CSHA::Reset()
{
    m_nCount[0] = m_nCount[1] = 0;
    m_nHash[0] = 0x67452301;
    m_nHash[1] = 0xefcdab89;
    m_nHash[2] = 0x98badcfe;
    m_nHash[3] = 0x10325476;
    m_nHash[4] = 0xc3d2e1f0;
}

void CSHA::GetHash(CAICHHash& Hash)
{
    /* extract the hash value as bytes in case the hash buffer is   */
    /* misaligned for 32-bit words*/

	ASSERT( Hash.GetHashSize() == 20 );
    for(int i = 0; i < SHA1_DIGEST_SIZE; ++i)
        Hash.GetRawHash()[i] = (unsigned char)(m_nHash[i >> 2] >> 8 * (~i & 3));
}

/* SHA1 hash data in an array of bytes into hash buffer and call the        */
/* hash_compile function as required.                                       */

void CSHA::Add(LPCVOID pData, DWORD nLength)
{
	const unsigned char* data = (const unsigned char*)pData;
	
	DWORD pos = (DWORD)(m_nCount[0] & SHA1_MASK), 
             space = SHA1_BLOCK_SIZE - pos;
    const unsigned char *sp = data;

    if((m_nCount[0] += nLength) < nLength)
        ++(m_nCount[1]);

    while(nLength >= space)     /* tranfer whole blocks while possible  */
    {
        memcpy(((unsigned char*)m_nBuffer) + pos, sp, space);
        sp += space; nLength -= space; space = SHA1_BLOCK_SIZE; pos = 0; 
        Compile();
    }

    memcpy(((unsigned char*)m_nBuffer) + pos, sp, nLength);
}

/* SHA1 final padding and digest calculation  */

#if (PLATFORM_BYTE_ORDER == SHA_LITTLE_ENDIAN)
static DWORD  mask[4] = 
	{   0x00000000, 0x000000ff, 0x0000ffff, 0x00ffffff };
static DWORD  bits[4] = 
	{   0x00000080, 0x00008000, 0x00800000, 0x80000000 };
#else
static DWORD  mask[4] = 
	{   0x00000000, 0xff000000, 0xffff0000, 0xffffff00 };
static DWORD  bits[4] = 
	{   0x80000000, 0x00800000, 0x00008000, 0x00000080 };
#endif

void CSHA::Finish(CAICHHash& Hash)
{
	DWORD    i = (DWORD)(m_nCount[0] & SHA1_MASK);

    /* mask out the rest of any partial 32-bit word and then set    */
    /* the next byte to 0x80. On big-endian machines any bytes in   */
    /* the buffer will be at the top end of 32 bit words, on little */
    /* endian machines they will be at the bottom. Hence the AND    */
    /* and OR masks above are reversed for little endian systems    */
	/* Note that we can always add the first padding byte at this	*/
	/* because the buffer always contains at least one empty slot	*/ 
    m_nBuffer[i >> 2] = (m_nBuffer[i >> 2] & mask[i & 3]) | bits[i & 3];

    /* we need 9 or more empty positions, one for the padding byte  */
    /* (above) and eight for the length count.  If there is not     */
    /* enough space pad and empty the buffer                        */
    if(i > SHA1_BLOCK_SIZE - 9)
    {
        if(i < 60) m_nBuffer[15] = 0;
        Compile();
        i = 0;
    }
    else    /* compute a word index for the empty buffer positions  */
        i = (i >> 2) + 1;

    while(i < 14) /* and zero pad all but last two positions      */ 
        m_nBuffer[i++] = 0;
    
    /* assemble the eight byte counter in in big-endian format		*/
    m_nBuffer[14] = swap_b32((m_nCount[1] << 3) | (m_nCount[0] >> 29));
    m_nBuffer[15] = swap_b32(m_nCount[0] << 3);

    Compile();
	GetHash(Hash);
}