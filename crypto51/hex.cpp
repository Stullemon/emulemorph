// hex.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "hex.h"

NAMESPACE_BEGIN(CryptoPP)

static const byte s_vecUpper[] = "0123456789ABCDEF";
static const byte s_vecLower[] = "0123456789abcdef";

void HexEncoder::IsolatedInitialize(const NameValuePairs &parameters)
{
	bool uppercase = parameters.GetValueWithDefault("Uppercase", true);
	m_filter->Initialize(CombinedNameValuePairs(
		parameters,
		MakeParameters("EncodingLookupArray", uppercase ? &s_vecUpper[0] : &s_vecLower[0])("Log2Base", 4)));
}

const int *HexDecoder::GetDecodingLookupArray()
{
	static bool s_initialized = false;
	static int s_array[256];

	if (!s_initialized)
	{
		InitializeDecodingLookupArray(s_array, s_vecUpper, 16, true);
		s_initialized = true;
	}
	return s_array;
}

NAMESPACE_END
