#ifndef CRYPTOPP_IDEA_H
#define CRYPTOPP_IDEA_H

/** \file
*/

#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

struct IDEA_Info : public FixedBlockSize<8>, public FixedKeyLength<16>, public FixedRounds<8>
{
	static const char *StaticAlgorithmName() {return "IDEA";}
};

/// <a href="http://www.weidai.com/scan-mirror/cs.html#IDEA">IDEA</a>
class IDEA : public IDEA_Info, public BlockCipherDocumentation
{
	class Base : public BlockCipherBaseTemplate<IDEA_Info>
	{
	public:
		unsigned int GetAlignment() const {return 2;}
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;

		void UncheckedSetKey(CipherDir direction, const byte *userKey, unsigned int length);

	private:
		void EnKey(const byte *);
		void DeKey();
		FixedSizeSecBlock<word, 6*ROUNDS+4> m_key;

	#ifdef IDEA_LARGECACHE
		static inline void LookupMUL(word &a, word b);
		void LookupKeyLogs();
		static void BuildLogTables();
		static bool tablesBuilt;
		static word16 log[0x10000], antilog[0x10000];
	#endif
	};

public:
	typedef BlockCipherTemplate<ENCRYPTION, Base> Encryption;
	typedef BlockCipherTemplate<DECRYPTION, Base> Decryption;
};

typedef IDEA::Encryption IDEAEncryption;
typedef IDEA::Decryption IDEADecryption;

NAMESPACE_END

#endif
