#ifndef CRYPTOPP_SERPENT_H
#define CRYPTOPP_SERPENT_H

/** \file
*/

#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

struct Serpent_Info : public FixedBlockSize<16>, public VariableKeyLength<16, 1, 32>, public FixedRounds<32>
{
	static const char *StaticAlgorithmName() {return "Serpent";}
};

/// <a href="http://www.weidai.com/scan-mirror/cs.html#Serpent">Serpent</a>
class Serpent : public Serpent_Info, public BlockCipherDocumentation
{
	class Base : public BlockCipherBaseTemplate<Serpent_Info>
	{
	public:
		void UncheckedSetKey(CipherDir direction, const byte *userKey, unsigned int length);

	protected:
		FixedSizeSecBlock<word32, 140> m_key;
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

typedef Serpent::Encryption SerpentEncryption;
typedef Serpent::Decryption SerpentDecryption;

NAMESPACE_END

#endif
