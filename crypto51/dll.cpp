// dll.cpp - written and placed in the public domain by Wei Dai

#define CRYPTOPP_MANUALLY_INSTANTIATE_TEMPLATES
#define CRYPTOPP_DEFAULT_NO_DLL

#include "dll.h"
#pragma warning(default: 4660)

#ifdef CRYPTOPP_WIN32_AVAILABLE
#include <windows.h>
#endif

#include "iterhash.cpp"
#include "strciphr.cpp"
#include "algebra.cpp"
#include "eprecomp.cpp"
#include "eccrypto.cpp"

#ifndef CRYPTOPP_IMPORTS

NAMESPACE_BEGIN(CryptoPP)

#ifdef __MWERKS__
// CodeWarrior 8 workaround: explicit instantiations have to appear after member function definitions
CRYPTOPP_DLL_TEMPLATE_CLASS DL_GroupParameters_EC<ECP>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_GroupParameters_EC<EC2N>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_FixedBasePrecomputationImpl<Integer>;
CRYPTOPP_STATIC_TEMPLATE_CLASS IteratedHashBase<word64, HashTransformation>;
CRYPTOPP_DLL_TEMPLATE_CLASS IteratedHashBase<word32, HashTransformation>;
CRYPTOPP_STATIC_TEMPLATE_CLASS IteratedHashBase<word32, MessageAuthenticationCode>;
CRYPTOPP_DLL_TEMPLATE_CLASS CFB_CipherTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> >;
CRYPTOPP_DLL_TEMPLATE_CLASS CFB_EncryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> >;
CRYPTOPP_DLL_TEMPLATE_CLASS CFB_DecryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> >;
CRYPTOPP_DLL_TEMPLATE_CLASS AdditiveCipherTemplate<>;
CRYPTOPP_DLL_TEMPLATE_CLASS AdditiveCipherTemplate<AbstractPolicyHolder<AdditiveCipherAbstractPolicy, OFB_ModePolicy> >;
CRYPTOPP_DLL_TEMPLATE_CLASS AdditiveCipherTemplate<AbstractPolicyHolder<AdditiveCipherAbstractPolicy, CTR_ModePolicy> >;
CRYPTOPP_DLL_TEMPLATE_CLASS AbstractEuclideanDomain<Integer>;
#endif

template<> const byte PKCS_DigestDecoration<SHA>::decoration[] = {0x30,0x21,0x30,0x09,0x06,0x05,0x2B,0x0E,0x03,0x02,0x1A,0x05,0x00,0x04,0x14};
template<> const unsigned int PKCS_DigestDecoration<SHA>::length = sizeof(PKCS_DigestDecoration<SHA>::decoration);

NAMESPACE_END

#endif

#ifdef CRYPTOPP_EXPORTS

USING_NAMESPACE(CryptoPP)

#if !(defined(_MSC_VER) && (_MSC_VER < 1300))
using std::set_new_handler;
#endif

static PNew s_pNew = NULL;
static PDelete s_pDelete = NULL;

static void * CRYPTOPP_CDECL New (size_t size)
{
	void *p;
	while (!(p = malloc(size)))
		CallNewHandler();

	return p;
}

static void SetNewAndDeleteFunctionPointers()
{
	void *p = NULL;
	HMODULE hModule = NULL;
	MEMORY_BASIC_INFORMATION mbi;

	while (true)
	{
		VirtualQuery(p, &mbi, sizeof(mbi));

		if (p >= (char *)mbi.BaseAddress + mbi.RegionSize)
			break;

		p = (char *)mbi.BaseAddress + mbi.RegionSize;

		if (!mbi.AllocationBase || mbi.AllocationBase == hModule)
			continue;

		hModule = HMODULE(mbi.AllocationBase);

		PGetNewAndDelete pGetNewAndDelete = (PGetNewAndDelete)GetProcAddress(hModule, "GetNewAndDeleteForCryptoPP");
		if (pGetNewAndDelete)
		{
			pGetNewAndDelete(s_pNew, s_pDelete);
			return;
		}

		PSetNewAndDelete pSetNewAndDelete = (PSetNewAndDelete)GetProcAddress(hModule, "SetNewAndDeleteFromCryptoPP");
		if (pSetNewAndDelete)
		{
			s_pNew = &New;
			s_pDelete = &free;
			pSetNewAndDelete(s_pNew, s_pDelete, &set_new_handler);
			return;
		}
	}

	hModule = GetModuleHandle("msvcrtd");
	if (!hModule)
		hModule = GetModuleHandle("msvcrt");
	if (hModule)
	{
		s_pNew = (PNew)GetProcAddress(hModule, "??2@YAPAXI@Z");		// operator new
		s_pDelete = (PDelete)GetProcAddress(hModule, "??3@YAXPAX@Z");	// operator delete
		return;
	}

	OutputDebugString("Crypto++ was not able to obtain new and delete function pointers.\n");
	throw 0;
}

void * CRYPTOPP_CDECL operator new (size_t size)
{
	if (!s_pNew)
		SetNewAndDeleteFunctionPointers();

	return s_pNew(size);
}

void CRYPTOPP_CDECL operator delete (void * p)
{
	s_pDelete(p);
}

void * CRYPTOPP_CDECL operator new [] (size_t size)
{
	return operator new (size);
}

void CRYPTOPP_CDECL operator delete [] (void * p)
{
	operator delete (p);
}

#endif	// #ifdef CRYPTOPP_EXPORTS
