/*
Copyright (C)2003 Barry Dunne (http://www.emule-project.net)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#include "stdafx.h"
#include "MD4.h"
//MORPH - Removed by SiRoB, Optimizer
//#include <memory.h>
#include <stdio.h>
#include "UInt128.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

/////////////////////////////////////////////////////
// Easy to use single-use static methods
/////////////////////////////////////////////////////

void CMD4::hash(const byte *data, unsigned int inputLen, CUInt128 *digest)
{
	CMD4 md4;
	md4.update(data, inputLen);
	md4.getHash(digest);
}

void CMD4::hashToString(const byte *data, unsigned int inputLen, CString *digest)
{
	CUInt128 temp;
	hash(data, inputLen, &temp);
	temp.toHexString(digest);
}

/////////////////////////////////////////////////////
// Need an instance to call repeatedly with blocks of data
/////////////////////////////////////////////////////

CMD4::CMD4(void)
{
	init(&context);
}

void CMD4::update(const byte *data, unsigned int length)
{
	update(&context, data, length);
}

void CMD4::getHash(CUInt128 *digest)
{
	final(&context, digest);
}

/////////////////////////////////////////////////////
// Private internal stuff
/////////////////////////////////////////////////////

void CMD4::init(MD4_CTX *context)
{
	context->count[0] = 0;
	context->count[1] = 0;
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
	for (int i=0; i<64; i++)
		context->buffer[i] = 0;
}

void CMD4::update(MD4_CTX *context, const byte *input, unsigned int inputLen)
{
	unsigned int i;
	unsigned int index;
	unsigned int partLen;

	/* Compute number of bytes mod 64 */
	index = ((context->count[0] >> 3) & 0x3F);

	/* Update number of bits */
	if ( (context->count[0] += (inputLen << 3)) < (inputLen << 3) )
		context->count[1]++;
	context->count[1] += (inputLen >> 29);

	partLen = 64 - index;

	/* Transform as many times as possible. */
	if (inputLen >= partLen)
	{
		memcpy(&context->buffer[index], input, partLen);
		transform(context->state, context->buffer);
		for (i = partLen; i+63 < inputLen; i += 64)
			transform(context->state, &input[i]);
		index = 0;
	}
	else
		i = 0;

	/* Buffer remaining input */
	memcpy(&context->buffer[index], &input[i], inputLen-i);
}

void CMD4::transform (unsigned int *state, const byte *block)
{
	unsigned int a = state[0];
	unsigned int b = state[1];
	unsigned int c = state[2];
	unsigned int d = state[3];
	unsigned int x[16];

	decode(x, block, 64);

	// Round 1
	MD4_FF(a, b, c, d, x[ 0], MD4_S11); /* 1 */
	MD4_FF(d, a, b, c, x[ 1], MD4_S12); /* 2 */
	MD4_FF(c, d, a, b, x[ 2], MD4_S13); /* 3 */
	MD4_FF(b, c, d, a, x[ 3], MD4_S14); /* 4 */
	MD4_FF(a, b, c, d, x[ 4], MD4_S11); /* 5 */
	MD4_FF(d, a, b, c, x[ 5], MD4_S12); /* 6 */
	MD4_FF(c, d, a, b, x[ 6], MD4_S13); /* 7 */
	MD4_FF(b, c, d, a, x[ 7], MD4_S14); /* 8 */
	MD4_FF(a, b, c, d, x[ 8], MD4_S11); /* 9 */
	MD4_FF(d, a, b, c, x[ 9], MD4_S12); /* 10 */
	MD4_FF(c, d, a, b, x[10], MD4_S13); /* 11 */
	MD4_FF(b, c, d, a, x[11], MD4_S14); /* 12 */
	MD4_FF(a, b, c, d, x[12], MD4_S11); /* 13 */
	MD4_FF(d, a, b, c, x[13], MD4_S12); /* 14 */
	MD4_FF(c, d, a, b, x[14], MD4_S13); /* 15 */
	MD4_FF(b, c, d, a, x[15], MD4_S14); /* 16 */

	// Round 2
	MD4_GG(a, b, c, d, x[ 0], MD4_S21); /* 17 */
	MD4_GG(d, a, b, c, x[ 4], MD4_S22); /* 18 */
	MD4_GG(c, d, a, b, x[ 8], MD4_S23); /* 19 */
	MD4_GG(b, c, d, a, x[12], MD4_S24); /* 20 */
	MD4_GG(a, b, c, d, x[ 1], MD4_S21); /* 21 */
	MD4_GG(d, a, b, c, x[ 5], MD4_S22); /* 22 */
	MD4_GG(c, d, a, b, x[ 9], MD4_S23); /* 23 */
	MD4_GG(b, c, d, a, x[13], MD4_S24); /* 24 */
	MD4_GG(a, b, c, d, x[ 2], MD4_S21); /* 25 */
	MD4_GG(d, a, b, c, x[ 6], MD4_S22); /* 26 */
	MD4_GG(c, d, a, b, x[10], MD4_S23); /* 27 */
	MD4_GG(b, c, d, a, x[14], MD4_S24); /* 28 */
	MD4_GG(a, b, c, d, x[ 3], MD4_S21); /* 29 */
	MD4_GG(d, a, b, c, x[ 7], MD4_S22); /* 30 */
	MD4_GG(c, d, a, b, x[11], MD4_S23); /* 31 */
	MD4_GG(b, c, d, a, x[15], MD4_S24); /* 32 */

	// Round 3
	MD4_HH(a, b, c, d, x[ 0], MD4_S31); /* 33 */
	MD4_HH(d, a, b, c, x[ 8], MD4_S32); /* 34 */
	MD4_HH(c, d, a, b, x[ 4], MD4_S33); /* 35 */
	MD4_HH(b, c, d, a, x[12], MD4_S34); /* 36 */
	MD4_HH(a, b, c, d, x[ 2], MD4_S31); /* 37 */
	MD4_HH(d, a, b, c, x[10], MD4_S32); /* 38 */
	MD4_HH(c, d, a, b, x[ 6], MD4_S33); /* 39 */
	MD4_HH(b, c, d, a, x[14], MD4_S34); /* 40 */
	MD4_HH(a, b, c, d, x[ 1], MD4_S31); /* 41 */
	MD4_HH(d, a, b, c, x[ 9], MD4_S32); /* 42 */
	MD4_HH(c, d, a, b, x[ 5], MD4_S33); /* 43 */
	MD4_HH(b, c, d, a, x[13], MD4_S34); /* 44 */
	MD4_HH(a, b, c, d, x[ 3], MD4_S31); /* 45 */
	MD4_HH(d, a, b, c, x[11], MD4_S32); /* 46 */
	MD4_HH(c, d, a, b, x[ 7], MD4_S33); /* 47 */
	MD4_HH(b, c, d, a, x[15], MD4_S34); /* 48 */

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	// Zeroize sensitive information.
	memset(x, 0, 16);
}

void CMD4::encode(byte *output, const unsigned int *input, unsigned int len)
{
	unsigned int i;
	unsigned int j;
	for (i=0, j=0; j<len; i++, j += 4)
	{
		output[j  ] = (byte)( input[i]        & 0xff);
		output[j+1] = (byte)((input[i] >>  8) & 0xff);
		output[j+2] = (byte)((input[i] >> 16) & 0xff);
		output[j+3] = (byte)((input[i] >> 24) & 0xff);
	}
}

void CMD4::decode(unsigned int *output, const byte *input, unsigned int len)
{
	unsigned int i;
	unsigned int j;
	for (i=0, j=0; j<len; i++, j+=4)
		output[i] = (((int)input[j])&0xFF) | ((((int)input[j+1])&0xFF) << 8) | ((((int)input[j+2])&0xFF) << 16) | ((((int)input[j+3])&0xFF) << 24);
}

void CMD4::final(MD4_CTX *context, CUInt128 *digest)
{
	byte bits[8];
	unsigned int index;
	unsigned int padLen;

	// Save number of bits
	encode(bits, context->count, 8);
	// Pad out to 56 mod 64.
	index = (unsigned int)((context->count[0] >> 3) & 0x3f);
	padLen = (index < 56) ? (56 - index) : (120 - index);
	update(context, MD4_PADDING, padLen);
	// Append length (before padding)
	update(context, bits, 8);
	// Store state in digest
	byte b[16];
	encode(b, context->state, 16);
	digest->setValueBE(b);

	// Zeroize sensitive information.
	init(context);
}