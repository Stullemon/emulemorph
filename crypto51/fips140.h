#ifndef CRYPTOPP_FIPS140_H
#define CRYPTOPP_FIPS140_H

/*! \file
	FIPS 140 related functions and classes.
*/

#include "cryptlib.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! exception thrown when a crypto algorithm is used after a self test fails
class CRYPTOPP_DLL SelfTestFailure : public Exception
{
public:
	explicit SelfTestFailure(const std::string &s) : Exception(OTHER_ERROR, s) {}
};

//! returns whether FIPS 140-2 compliance features were enabled at compile time
CRYPTOPP_DLL bool FIPS_140_2_ComplianceEnabled();

//! enum values representing status of the power-up self test
enum PowerUpSelfTestStatus {POWER_UP_SELF_TEST_NOT_DONE, POWER_UP_SELF_TEST_FAILED, POWER_UP_SELF_TEST_PASSED};

//! perform the power-up self test, and set the self test status
CRYPTOPP_DLL void DoPowerUpSelfTest(const char *moduleFilename, const byte *expectedModuleMac);

//! perform the power-up self test using the filename of this DLL and the embedded module MAC
CRYPTOPP_DLL void DoDllPowerUpSelfTest();

//! set the power-up self test status to POWER_UP_SELF_TEST_FAILED
CRYPTOPP_DLL void SimulatePowerUpSelfTestFailure();

//! return the current power-up self test status
CRYPTOPP_DLL PowerUpSelfTestStatus CRYPTOPP_API GetPowerUpSelfTestStatus();

typedef PowerUpSelfTestStatus (CRYPTOPP_API * PGetPowerUpSelfTestStatus)();

CRYPTOPP_DLL const byte * CRYPTOPP_API GetActualMacAndLocation(unsigned int &macSize, unsigned int &fileLocation);

typedef const byte * (CRYPTOPP_API * PGetActualMacAndLocation)(unsigned int &macSize, unsigned int &fileLocation);

CRYPTOPP_DLL MessageAuthenticationCode * NewIntegrityCheckingMAC();

CRYPTOPP_DLL bool IntegrityCheckModule(const char *moduleFilename, const byte *expectedModuleMac, SecByteBlock *pActualMac = NULL, unsigned long *pMacFileLocation = NULL);

// this is used by Algorithm constructor to allow Algorithm objects to be constructed for the self test
bool PowerUpSelfTestInProgressOnThisThread();

void SetPowerUpSelfTestInProgressOnThisThread(bool inProgress);

void SignaturePairwiseConsistencyTest(const PK_Signer &signer, const PK_Verifier &verifier);
void EncryptionPairwiseConsistencyTest(const PK_Encryptor &encryptor, const PK_Decryptor &decryptor);

void SignaturePairwiseConsistencyTest_FIPS_140_Only(const PK_Signer &signer, const PK_Verifier &verifier);
void EncryptionPairwiseConsistencyTest_FIPS_140_Only(const PK_Encryptor &encryptor, const PK_Decryptor &decryptor);

NAMESPACE_END

#endif
