#ifndef CRYPTOPP_GFPCRYPT_H
#define CRYPTOPP_GFPCRYPT_H

/** \file
	Implementation of schemes based on DL over GF(p)
*/

#include "pubkey.h"
#include "modexppc.h"
#include "sha.h"
#include "algparam.h"
#include "asn.h"
#include "smartptr.h"
#include "hmac.h"

#include <limits.h>

NAMESPACE_BEGIN(CryptoPP)

//! .
class DL_GroupParameters_IntegerBased : public DL_GroupParameters<Integer>, public ASN1CryptoMaterial
{
	typedef DL_GroupParameters_IntegerBased ThisClass;
	
public:
	void Initialize(const DL_GroupParameters_IntegerBased &params)
		{Initialize(params.GetModulus(), params.GetSubgroupOrder(), params.GetSubgroupGenerator());}
	void Initialize(RandomNumberGenerator &rng, unsigned int pbits)
		{GenerateRandom(rng, MakeParameters("ModulusSize", (int)pbits));}
	void Initialize(const Integer &p, const Integer &g)
		{SetModulusAndSubgroupGenerator(p, g); SetSubgroupOrder(ComputeGroupOrder(p)/2);}
	void Initialize(const Integer &p, const Integer &q, const Integer &g)
		{SetModulusAndSubgroupGenerator(p, g); SetSubgroupOrder(q);}

	// ASN1Object interface
	void BERDecode(BufferedTransformation &bt);
	void DEREncode(BufferedTransformation &bt) const;

	// GeneratibleCryptoMaterial interface
	/*! parameters: (ModulusSize, SubgroupOrderSize (optional)) */
	void GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg);
	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;
	void AssignFrom(const NameValuePairs &source);
	
	// DL_GroupParameters
	const Integer & GetSubgroupOrder() const {return m_q;}
	Integer GetGroupOrder() const {return GetFieldType() == 1 ? GetModulus()-Integer::One() : GetModulus()+Integer::One();}
	bool ValidateGroup(RandomNumberGenerator &rng, unsigned int level) const;
	bool ValidateElement(unsigned int level, const Integer &element, const DL_FixedBasePrecomputation<Integer> *precomp) const;
	bool FastSubgroupCheckAvailable() const {return GetCofactor() == 2;}
	void EncodeElement(bool reversible, const Element &element, byte *encoded) const
		{element.Encode(encoded, GetModulus().ByteCount());}
	unsigned int GetEncodedElementSize(bool reversible) const {return GetModulus().ByteCount();}
	Integer DecodeElement(const byte *encoded, bool checkForGroupMembership) const;
	Integer ConvertElementToInteger(const Element &element) const
		{return element;}
	Integer GetMaxExponent() const;

	OID GetAlgorithmID() const;

	virtual const Integer & GetModulus() const =0;
	virtual void SetModulusAndSubgroupGenerator(const Integer &p, const Integer &g) =0;

	void SetSubgroupOrder(const Integer &q)
		{m_q = q; ParametersChanged();}

protected:
	Integer ComputeGroupOrder(const Integer &modulus) const
		{return modulus-(GetFieldType() == 1 ? 1 : -1);}

	// GF(p) = 1, GF(p^2) = 2
	virtual int GetFieldType() const =0;
	virtual unsigned int GetDefaultSubgroupOrderSize(unsigned int modulusSize) const;

private:
	Integer m_q;
};

//! .
template <class GROUP_PRECOMP, class BASE_PRECOMP = DL_FixedBasePrecomputationImpl<CPP_TYPENAME GROUP_PRECOMP::Element> >
class DL_GroupParameters_IntegerBasedImpl : public DL_GroupParametersImpl<GROUP_PRECOMP, BASE_PRECOMP, DL_GroupParameters_IntegerBased>
{
	typedef DL_GroupParameters_IntegerBasedImpl<GROUP_PRECOMP, BASE_PRECOMP> ThisClass;

public:
	typedef typename GROUP_PRECOMP::Element Element;

	// GeneratibleCryptoMaterial interface
	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
		{return GetValueHelper<DL_GroupParameters_IntegerBased>(this, name, valueType, pValue).Assignable();}

	void AssignFrom(const NameValuePairs &source)
		{AssignFromHelper<DL_GroupParameters_IntegerBased>(this, source);}

	// DL_GroupParameters
	const DL_FixedBasePrecomputation<Element> & GetBasePrecomputation() const {return m_gpc;}
	DL_FixedBasePrecomputation<Element> & AccessBasePrecomputation() {return m_gpc;}

	// IntegerGroupParameters
	const Integer & GetModulus() const {return m_groupPrecomputation.GetModulus();}
	const Integer & GetGenerator() const {return m_gpc.GetBase(GetGroupPrecomputation());}

	void SetModulusAndSubgroupGenerator(const Integer &p, const Integer &g)		// these have to be set together
		{m_groupPrecomputation.SetModulus(p); m_gpc.SetBase(GetGroupPrecomputation(), g); ParametersChanged();}

	// non-inherited
	bool operator==(const DL_GroupParameters_IntegerBasedImpl<GROUP_PRECOMP, BASE_PRECOMP> &rhs) const
		{return GetModulus() == rhs.GetModulus() && GetGenerator() == rhs.GetGenerator() && GetSubgroupOrder() == rhs.GetSubgroupOrder();}
	bool operator!=(const DL_GroupParameters_IntegerBasedImpl<GROUP_PRECOMP, BASE_PRECOMP> &rhs) const
		{return !operator==(rhs);}
};

//! .
class DL_GroupParameters_GFP : public DL_GroupParameters_IntegerBasedImpl<ModExpPrecomputation>
{
public:
	// DL_GroupParameters
	bool IsIdentity(const Integer &element) const {return element == Integer::One();}
	void SimultaneousExponentiate(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const;

	// NameValuePairs interface
	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
	{
		return GetValueHelper<DL_GroupParameters_IntegerBased>(this, name, valueType, pValue).Assignable();
	}

	// used by MQV
	Element MultiplyElements(const Element &a, const Element &b) const;
	Element CascadeExponentiate(const Element &element1, const Integer &exponent1, const Element &element2, const Integer &exponent2) const;

protected:
	int GetFieldType() const {return 1;}
};

//! .
class DL_GroupParameters_GFP_DefaultSafePrime : public DL_GroupParameters_GFP
{
public:
	typedef NoCofactorMultiplication DefaultCofactorOption;

protected:
	unsigned int GetDefaultSubgroupOrderSize(unsigned int modulusSize) const {return modulusSize-1;}
};

//! .
template <class T>
class DL_Algorithm_GDSA : public DL_ElgamalLikeSignatureAlgorithm<T>
{
public:
	static const char * StaticAlgorithmName() {return "DSA-1363";}

	void Sign(const DL_GroupParameters<T> &params, const Integer &x, const Integer &k, const Integer &e, Integer &r, Integer &s) const
	{
		const Integer &q = params.GetSubgroupOrder();
		r %= q;
		Integer kInv = k.InverseMod(q);
		s = (kInv * (x*r + e)) % q;
		assert(!!r && !!s);
	}

	bool Verify(const DL_GroupParameters<T> &params, const DL_PublicKey<T> &publicKey, const Integer &e, const Integer &r, const Integer &s) const
	{
		const Integer &q = params.GetSubgroupOrder();
		if (r>=q || r<1 || s>=q || s<1)
			return false;

		Integer w = s.InverseMod(q);
		Integer u1 = (e * w) % q;
		Integer u2 = (r * w) % q;
		// verify r == (g^u1 * y^u2 mod p) mod q
		return r == params.ConvertElementToInteger(publicKey.CascadeExponentiateBaseAndPublicElement(u1, u2)) % q;
	}
};

//! .
template <class T>
class DL_Algorithm_NR : public DL_ElgamalLikeSignatureAlgorithm<T>
{
public:
	static const char * StaticAlgorithmName() {return "NR";}

	Integer EncodeDigest(unsigned int modulusBits, const byte *digest, unsigned int digestLen) const
	{
		return NR_EncodeDigest(modulusBits, digest, digestLen);
	}

	void Sign(const DL_GroupParameters<T> &params, const Integer &x, const Integer &k, const Integer &e, Integer &r, Integer &s) const
	{
		const Integer &q = params.GetSubgroupOrder();
		r = (r + e) % q;
		s = (k - x*r) % q;
		assert(!!r);
	}

	bool Verify(const DL_GroupParameters<T> &params, const DL_PublicKey<T> &publicKey, const Integer &e, const Integer &r, const Integer &s) const
	{
		const Integer &q = params.GetSubgroupOrder();
		if (r>=q || r<1 || s>=q)
			return false;

		// check r == (m_g^s * m_y^r + m) mod m_q
		return r == (params.ConvertElementToInteger(publicKey.CascadeExponentiateBaseAndPublicElement(s, r)) + e) % q;
	}
};

/*! DSA public key format is defined in 7.3.3 of RFC 2459. The
	private key format is defined in 12.9 of PKCS #11 v2.10. */
template <class GP>
class DL_PublicKey_GFP : public DL_PublicKeyImpl<GP>
{
public:
	void Initialize(const DL_GroupParameters_IntegerBased &params, const Integer &y)
		{AccessGroupParameters().Initialize(params); SetPublicElement(y);}
	void Initialize(const Integer &p, const Integer &g, const Integer &y)
		{AccessGroupParameters().Initialize(p, g); SetPublicElement(y);}
	void Initialize(const Integer &p, const Integer &q, const Integer &g, const Integer &y)
		{AccessGroupParameters().Initialize(p, q, g); SetPublicElement(y);}

	// X509PublicKey
	void BERDecodeKey(BufferedTransformation &bt)
		{SetPublicElement(Integer(bt));}
	void DEREncodeKey(BufferedTransformation &bt) const
		{GetPublicElement().DEREncode(bt);}
};

//! .
template <class GP>
class DL_PrivateKey_GFP : public DL_PrivateKeyImpl<GP>
{
public:
	void Initialize(RandomNumberGenerator &rng, unsigned int modulusBits)
		{GenerateRandomWithKeySize(rng, modulusBits);}
	void Initialize(RandomNumberGenerator &rng, const Integer &p, const Integer &g)
		{GenerateRandom(rng, MakeParameters("Modulus", p)("SubgroupGenerator", g));}
	void Initialize(RandomNumberGenerator &rng, const Integer &p, const Integer &q, const Integer &g)
		{GenerateRandom(rng, MakeParameters("Modulus", p)("SubgroupOrder", q)("SubgroupGenerator", g));}
	void Initialize(const DL_GroupParameters_IntegerBased &params, const Integer &x)
		{AccessGroupParameters().Initialize(params); SetPrivateExponent(x);}
	void Initialize(const Integer &p, const Integer &g, const Integer &x)
		{AccessGroupParameters().Initialize(p, g); SetPrivateExponent(x);}
	void Initialize(const Integer &p, const Integer &q, const Integer &g, const Integer &x)
		{AccessGroupParameters().Initialize(p, q, g); SetPrivateExponent(x);}
};

//! .
struct DL_SignatureKeys_GFP
{
	typedef DL_GroupParameters_GFP GroupParameters;
	typedef DL_PublicKey_GFP<GroupParameters> PublicKey;
	typedef DL_PrivateKey_GFP<GroupParameters> PrivateKey;
};

//! .
struct DL_CryptoKeys_GFP
{
	typedef DL_GroupParameters_GFP_DefaultSafePrime GroupParameters;
	typedef DL_PublicKey_GFP<GroupParameters> PublicKey;
	typedef DL_PrivateKey_GFP<GroupParameters> PrivateKey;
};

//! provided for backwards compatibility, this class uses the old non-standard Crypto++ key format
template <class BASE>
class DL_PublicKey_GFP_OldFormat : public BASE
{
public:
	void BERDecode(BufferedTransformation &bt)
	{
		BERSequenceDecoder seq(bt);
			Integer v1(seq);
			Integer v2(seq);
			Integer v3(seq);

			if (seq.EndReached())
			{
				AccessGroupParameters().Initialize(v1, v1/2, v2);
				SetPublicElement(v3);
			}
			else
			{
				Integer v4(seq);
				AccessGroupParameters().Initialize(v1, v2, v3);
				SetPublicElement(v4);
			}

		seq.MessageEnd();
	}

	void DEREncode(BufferedTransformation &bt) const
	{
		DERSequenceEncoder seq(bt);
			GetGroupParameters().GetModulus().DEREncode(seq);
			if (GetGroupParameters().GetCofactor() != 2)
				GetGroupParameters().GetSubgroupOrder().DEREncode(seq);
			GetGroupParameters().GetGenerator().DEREncode(seq);
			GetPublicElement().DEREncode(seq);
		seq.MessageEnd();
	}
};

//! provided for backwards compatibility, this class uses the old non-standard Crypto++ key format
template <class BASE>
class DL_PrivateKey_GFP_OldFormat : public BASE
{
public:
	void BERDecode(BufferedTransformation &bt)
	{
		BERSequenceDecoder seq(bt);
			Integer v1(seq);
			Integer v2(seq);
			Integer v3(seq);
			Integer v4(seq);

			if (seq.EndReached())
			{
				AccessGroupParameters().Initialize(v1, v1/2, v2);
				SetPrivateExponent(v4 % (v1/2));	// some old keys may have x >= q
			}
			else
			{
				Integer v5(seq);
				AccessGroupParameters().Initialize(v1, v2, v3);
				SetPrivateExponent(v5);
			}

		seq.MessageEnd();
	}

	void DEREncode(BufferedTransformation &bt) const
	{
		DERSequenceEncoder seq(bt);
			GetGroupParameters().GetModulus().DEREncode(seq);
			if (GetGroupParameters().GetCofactor() != 2)
				GetGroupParameters().GetSubgroupOrder().DEREncode(seq);
			GetGroupParameters().GetGenerator().DEREncode(seq);
			GetGroupParameters().ExponentiateBase(GetPrivateExponent()).DEREncode(seq);
			GetPrivateExponent().DEREncode(seq);
		seq.MessageEnd();
	}
};

//! <a href="http://www.weidai.com/scan-mirror/sig.html#DSA-1363">DSA-1363</a>
template <class H>
struct GDSA : public DL_SS<
	DL_SignatureKeys_GFP, 
	DL_Algorithm_GDSA<Integer>, 
	DL_SignatureMessageEncodingMethod_DSA,
	H>
{
};

//! <a href="http://www.weidai.com/scan-mirror/sig.html#NR">NR</a>
template <class H>
struct NR : public DL_SS<
	DL_SignatureKeys_GFP, 
	DL_Algorithm_NR<Integer>, 
	DL_SignatureMessageEncodingMethod_NR,
	H>
{
};

//! .
class DL_GroupParameters_DSA : public DL_GroupParameters_GFP
{
public:
	/*! also checks that the lengths of p and q are allowed by the DSA standard */
	bool ValidateGroup(RandomNumberGenerator &rng, unsigned int level) const;
	/*! parameters: (ModulusSize), or (Modulus, SubgroupOrder, SubgroupGenerator) */
	/*! ModulusSize must be between 512 and 1024, and divisible by 64 */
	void GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg);
};

struct DSA;

//! .
struct DL_Keys_DSA
{
	typedef DL_PublicKey_GFP<DL_GroupParameters_DSA> PublicKey;
	typedef DL_PrivateKey_WithSignaturePairwiseConsistencyTest<DL_PrivateKey_GFP<DL_GroupParameters_DSA>, DSA> PrivateKey;
};

//! <a href="http://www.weidai.com/scan-mirror/sig.html#DSA">DSA</a>
struct DSA : public DL_SS<
	DL_Keys_DSA, 
	DL_Algorithm_GDSA<Integer>, 
	DL_SignatureMessageEncodingMethod_DSA,
	SHA, 
	DSA>
{
	static std::string StaticAlgorithmName() {return std::string("DSA");}

	//! Generate DSA primes according to NIST standard
	/*! Both seedLength and primeLength are in bits, but seedLength should
		be a multiple of 8.
		If useInputCounterValue == true, the counter parameter is taken as input, otherwise it's used for output
	*/
	static bool GeneratePrimes(const byte *seed, unsigned int seedLength, int &counter,
								Integer &p, unsigned int primeLength, Integer &q, bool useInputCounterValue = false);

	static bool IsValidPrimeLength(unsigned int pbits)
		{return pbits >= MIN_PRIME_LENGTH && pbits <= MAX_PRIME_LENGTH && pbits % PRIME_LENGTH_MULTIPLE == 0;}

	enum {
#if (DSA_1024_BIT_MODULUS_ONLY)
		MIN_PRIME_LENGTH = 1024,
#else
		MIN_PRIME_LENGTH = 512,
#endif
		MAX_PRIME_LENGTH = 1024, PRIME_LENGTH_MULTIPLE = 64};
};

//! .
template <class MAC, bool DHAES_MODE>
class DL_EncryptionAlgorithm_Xor : public DL_SymmetricEncryptionAlgorithm
{
public:
	unsigned int GetSymmetricKeyLength(unsigned int plainTextLength) const
		{return plainTextLength + MAC::DEFAULT_KEYLENGTH;}
	unsigned int GetSymmetricCiphertextLength(unsigned int plainTextLength) const
		{return plainTextLength + MAC::DIGESTSIZE;}
	unsigned int GetMaxSymmetricPlaintextLength(unsigned int cipherTextLength) const
		{return SaturatingSubtract(cipherTextLength, (unsigned int)MAC::DIGESTSIZE);}
	void SymmetricEncrypt(RandomNumberGenerator &rng, const byte *key, const byte *plainText, unsigned int plainTextLength, byte *cipherText) const
	{
		const byte *cipherKey, *macKey;
		if (DHAES_MODE)
		{
			macKey = key;
			cipherKey = key + MAC::DEFAULT_KEYLENGTH;
		}
		else
		{
			cipherKey = key;
			macKey = key + plainTextLength;
		}

		xorbuf(cipherText, plainText, cipherKey, plainTextLength);
		MAC mac(macKey);
		mac.Update(cipherText, plainTextLength);
		if (DHAES_MODE)
		{
			const byte L[8] = {0,0,0,0,0,0,0,0};
			mac.Update(L, 8);
		}
		mac.Final(cipherText + plainTextLength);
	}
	DecodingResult SymmetricDecrypt(const byte *key, const byte *cipherText, unsigned int cipherTextLength, byte *plainText) const
	{
		unsigned int plainTextLength = GetMaxSymmetricPlaintextLength(cipherTextLength);
		const byte *cipherKey, *macKey;
		if (DHAES_MODE)
		{
			macKey = key;
			cipherKey = key + MAC::DEFAULT_KEYLENGTH;
		}
		else
		{
			cipherKey = key;
			macKey = key + plainTextLength;
		}

		MAC mac(macKey);
		mac.Update(cipherText, plainTextLength);
		if (DHAES_MODE)
		{
			const byte L[8] = {0,0,0,0,0,0,0,0};
			mac.Update(L, 8);
		}
		if (!mac.Verify(cipherText + plainTextLength))
			return DecodingResult();

		xorbuf(plainText, cipherText, cipherKey, plainTextLength);
		return DecodingResult(plainTextLength);
	}
};

//! .
template <class T, bool DHAES_MODE, class KDF>
class DL_KeyDerivationAlgorithm_P1363 : public DL_KeyDerivationAlgorithm<T>
{
public:
	void Derive(const DL_GroupParameters<T> &params, byte *derivedKey, unsigned int derivedLength, const T &agreedElement, const T &ephemeralPublicKey) const
	{
		SecByteBlock agreedSecret;
		if (DHAES_MODE)
		{
			agreedSecret.New(params.GetEncodedElementSize(true) + params.GetEncodedElementSize(false));
			params.EncodeElement(true, ephemeralPublicKey, agreedSecret);
			params.EncodeElement(false, agreedElement, agreedSecret + params.GetEncodedElementSize(true));
		}
		else
		{
			agreedSecret.New(params.GetEncodedElementSize(false));
			params.EncodeElement(false, agreedElement, agreedSecret);
		}

		KDF::DeriveKey(derivedKey, derivedLength, agreedSecret, agreedSecret.size());
	}
};

//! Discrete Log Integrated Encryption Scheme, AKA <a href="http://www.weidai.com/scan-mirror/ca.html#DLIES">DLIES</a>
template <class COFACTOR_OPTION = NoCofactorMultiplication, bool DHAES_MODE = true>
struct DLIES
	: public DL_ES<
		DL_CryptoKeys_GFP,
		DL_KeyAgreementAlgorithm_DH<Integer, COFACTOR_OPTION>,
		DL_KeyDerivationAlgorithm_P1363<Integer, DHAES_MODE, P1363_KDF2<SHA1> >,
		DL_EncryptionAlgorithm_Xor<HMAC<SHA1>, DHAES_MODE>,
		DLIES<> >
{
	static std::string StaticAlgorithmName() {return "DLIES";}	// TODO: fix this after name is standardized
};

NAMESPACE_END

#endif
