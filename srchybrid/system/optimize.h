#include "memcpy_amd.h"//Morph - added by AndCycle, eMulePlus CPU optimize

//Morph Start - added by AndCycle, eMulePlus CPU optimize

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

// md4cpy -- replacement for memcpy(dst,src,16)
__inline void md4cpy(void* dst, const void* src)
{
	MEMCOPY(dst,src,16);
}
//Morph End - added by AndCycle, eMulePlus CPU optimize