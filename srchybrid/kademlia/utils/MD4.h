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

#pragma once

#include "../../stdafx.h"

#ifndef byte
typedef unsigned char byte;
#endif

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CUInt128;

#ifndef NOMD4MACROS

/** Constants for MD4Transform routine. */
#define MD4_S11  3
#define MD4_S12  7
#define MD4_S13 11
#define MD4_S14 19
#define MD4_S21  3
#define MD4_S22  5
#define MD4_S23  9
#define MD4_S24 13
#define MD4_S31  3
#define MD4_S32  9
#define MD4_S33 11
#define MD4_S34 15

/** Buffer padding. */
static const byte MD4_PADDING[64] = { 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/** ROTATE_LEFT rotates x left n bits. */
#define MD4_ROTATE_LEFT(x, n) ((x << n) | (x >> (32-n)))

/** F, G and H are basic MD4 functions. */
#define MD4_F(x, y, z)	( (x & y) | ((~x) & z) )
#define MD4_G(x, y, z)	( (x & y) | (x & z) | (y & z) )
#define MD4_H(x, y, z)	( x ^ y ^ z )

/** FF, GG and HH combine the basic MD4 functions. */
#define MD4_FF(a, b, c, d, x, s)	a += MD4_F(b, c, d) + x;              a = MD4_ROTATE_LEFT(a, s); 
#define MD4_GG(a, b, c, d, x, s)	a += MD4_G(b, c, d) + x + 0x5a827999; a = MD4_ROTATE_LEFT(a, s); 
#define MD4_HH(a, b, c, d, x, s)	a += MD4_H(b, c, d) + x + 0x6ed9eba1; a = MD4_ROTATE_LEFT(a, s); 

#endif //!NOMD4MACROS

/** MD4 context. */
struct MD4_CTX
{
	/** state (ABCD) */
	unsigned int state[4];
	/** number of bits, modulo 2^64 (lsb first) */
	unsigned int count[2];
	/** input buffer */
	byte buffer[64];
};


/**
 * This class implements the MD4 algorithm with convenient static methods.
 */
class CMD4
{
public:
	/////////////////////////////////////////////////////
	// Easy to use single-use static methods
	/////////////////////////////////////////////////////

	/**
	 * This will calculate the MD4 hash of the given data.
	 *
	 * @param data     [in]  The data to hash.
	 * @param inputLen [in]  The length of the data to hash.
	 * @param digest   [out] The MD4 hash of the data
	 */
	static void hash(const byte *data, unsigned int inputLen, CUInt128 *digest);

	/**
	 * This will calculate the MD4 hash of the given data.
	 *
	 * @param data     [in]  The data to hash.
	 * @param inputLen [in]  The length of the data to hash.
	 * @param digest   [out] The MD4 hash of the data in hex format.
	 */
	static void hashToString(const byte *data, unsigned int inputLen, CString *digest);

	/////////////////////////////////////////////////////
	// Need an instance to call repeatedly with blocks of data
	/////////////////////////////////////////////////////

	/**
	 * Create a new MD4 object ready to begin updating.
	 */
	CMD4(void);

	/**
	 * Process another block of data.
	 *
	 * @param data   [in] The data to process.
	 * @param length [in] The amount of data to process.
	 */
	void update(const byte *data, unsigned int length);

	/**
	 * This will return the MD4 hash of all previously processed data.      <p>
	 *
	 * Note that the MD4 class will be re-initialised after this call,
	 * the next call to update will begin a new MD4 hash process.
	 *
	 * @param digest [out] The MD4 hash of the data
	 */
	void getHash(CUInt128 *digest);


private:
	/////////////////////////////////////////////////////
	// Private internal stuff
	/////////////////////////////////////////////////////

	/** The conext for this object instance. */
	MD4_CTX context;

	/** MD4 initialization. Begins an MD4 operation, writing a new context. */
	static void init(MD4_CTX *context);

	/** MD4 block update operation. Continues an MD4 message-digest operation,
	 *  processing another message block, and updating the context. */
	static void update(MD4_CTX *context, const byte *input, unsigned int inputLen);

	/** MD4 basic transformation. Transforms state based on block. */
	static void transform (unsigned int *state, const byte *block);

	/** Encodes input (unsigned int) into output (byte). Assumes len is a multiple of 4. */
	static void encode(byte *output, const unsigned int *input, unsigned int len);

	/** Decodes input (byte) into output (unsigned int). Assumes len is a multiple of 4. */
	static void decode(unsigned int *output, const byte *input, unsigned int len);

	/** MD4 finalization. Ends an MD4 message-digest operation, writing the message digest and zeroizing the context. */
	static void final(MD4_CTX *context, CUInt128 *digest);
};

} // End namespace