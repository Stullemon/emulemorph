#include "memcpy_amd.h"


//replacement for memcpy
__inline void *MEMCOPY(void* dst, const void *src, size_t size)
{
	return(::memcpy_optimized(dst,src,size));
}

//replacement for memset
__inline void *MEMSET(void* dst, int c, size_t size)
{
	return(::memset_optimized(dst,c,size));
}

__inline void MEMZERO(void* dst, size_t size)
{
	::memzero_optimized(dst, size);
}

typedef struct
{
	BYTE		hash[16];
} HashType;

// md4cpy -- replacement for memcpy(dst,src,16)
__inline void md4cpy(void* dst, const void* src)
{
	*reinterpret_cast<HashType*>(dst) = *reinterpret_cast<const HashType*>(src);
}
