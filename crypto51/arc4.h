#ifndef CRYPTOPP_ARC4_H
#define CRYPTOPP_ARC4_H

#include "strciphr.h"

NAMESPACE_BEGIN(CryptoPP)

//! <a href="http://www.weidai.com/scan-mirror/cs.html#RC4">Alleged RC4</a>
/*! You can #ARC4 typedef rather than this class directly. */
class ARC4_Base : public VariableKeyLength<16, 1, 256>, public RandomNumberGenerator, public SymmetricCipher
{
public:
	~ARC4_Base();

	static const char *StaticAlgorithmName() {return "ARC4";}

    byte GenerateByte();
	void DiscardBytes(unsigned int n);

    void ProcessData(byte *outString, const byte *inString, unsigned int length);
	
	bool IsRandomAccess() const {return false;}
	bool IsSelfInverting() const {return true;}
	bool IsForwardTransformation() const {return true;}

	typedef SymmetricCipherFinalTemplate<ARC4_Base> Encryption;
	typedef SymmetricCipherFinalTemplate<ARC4_Base> Decryption;

protected:
	void UncheckedSetKey(const NameValuePairs &params, const byte *key, unsigned int length);
	virtual unsigned int GetDefaultDiscardBytes() const {return 0;}

    FixedSizeSecBlock<byte, 256> m_state;
    byte m_x, m_y;
};

//! .
typedef SymmetricCipherFinalTemplate<ARC4_Base> ARC4;

//! Modified ARC4: it discards the first 256 bytes of keystream which may be weaker than the rest
/*! You can #MARC4 typedef rather than this class directly. */
class MARC4_Base : public ARC4_Base
{
public:
	static const char *StaticAlgorithmName() {return "MARC4";}

	typedef SymmetricCipherFinalTemplate<MARC4_Base> Encryption;
	typedef SymmetricCipherFinalTemplate<MARC4_Base> Decryption;

protected:
	unsigned int GetDefaultDiscardBytes() const {return 256;}
};

//! .
typedef SymmetricCipherFinalTemplate<MARC4_Base> MARC4;

NAMESPACE_END

#endif
