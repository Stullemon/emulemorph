//<<< eWombat [OnTheFly] Optimizer
// Miscellaneous routines
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef enum OPT_LEVEL
{
    OPTLEVEL_NONE,
    OPTLEVEL_MMX,
    OPTLEVEL_ATHLON,
	OPTLEVEL_ERROR
} eOptLevel;
//Callback for memcpy
//#define MEMCOPY	memcpy
#define MEMCOPY		_memcopy
typedef void* (*fnMemcpy)(void*, const void*, size_t);
extern fnMemcpy _memcopy;
__declspec(dllexport) void * memcpy_std(void *dest, const void *src, size_t n);
__declspec(dllexport) void * memcpy_amd(void *dest, const void *src, size_t n);
__declspec(dllexport) void * memcpy_mmx(void *dest, const void *src, size_t n);

//Callback for md4cpy
//#define MD4CPY	md4cpy
#define MD4COPY		_md4copy
typedef void* (*fnMd4cpy)(void*, const void*);
extern fnMd4cpy _md4copy;
__declspec(dllexport) void md4cpy_std(void* dst, const void* src);
__declspec(dllexport) void md4cpy_amd(void* dst, const void* src);
__declspec(dllexport) void md4cpy_mmx(void* dst, const void* src);

#ifdef __cplusplus
};
#endif
//>>>  eWombat [OnTheFly] Optimizer
