#include "memcpy_amd.h"

typedef struct
{
	BYTE		hash[16];
} HashType;

// md4cpy -- replacement for memcpy(dst,src,16)
__inline void md4cpy(void* dst, const void* src)
{
	*reinterpret_cast<HashType*>(dst) = *reinterpret_cast<const HashType*>(src);
}
