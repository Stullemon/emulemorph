#ifndef CRYPTOPP_TEA_H
#define CRYPTOPP_TEA_H

/** \file
*/

#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

struct TEA_Info : public FixedBlockSize<8>, public FixedKeyLength<16>, public FixedRounds<32>
{
	enum {LOG_ROUNDS=5};
	static const char *StaticAlgorithmName() {return "TEA";}
};

/// <a href="http://www.weidai.com/scan-mirror/cs.html#TEA">TEA</a>
class TEA : public TEA_Info, public BlockCipherDocumentation
{
	class Base : public BlockCipherBaseTemplate<TEA_Info>
	{
	public:
		void UncheckedSetKey(CipherDir direction, const byte *userKey, unsigned int length);

	protected:
		static const word32 DELTA;
		FixedSizeSecBlock<word32, 4> k;
	};

	class Enc : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

	class Dec : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

public:
	typedef BlockCipherTemplate<ENCRYPTION, Enc> Encryption;
	typedef BlockCipherTemplate<DECRYPTION, Dec> Decryption;
};

typedef TEA::Encryption TEAEncryption;
typedef TEA::Decryption TEADecryption;

NAMESPACE_END

#endif
