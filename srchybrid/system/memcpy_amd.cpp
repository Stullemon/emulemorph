/******************************************************************************

 Copyright (c) 2001 Advanced Micro Devices, Inc.

 LIMITATION OF LIABILITY:  THE MATERIALS ARE PROVIDED *AS IS* WITHOUT ANY
 EXPRESS OR IMPLIED WARRANTY OF ANY KIND INCLUDING WARRANTIES OF MERCHANTABILITY,
 NONINFRINGEMENT OF THIRD-PARTY INTELLECTUAL PROPERTY, OR FITNESS FOR ANY
 PARTICULAR PURPOSE.  IN NO EVENT SHALL AMD OR ITS SUPPLIERS BE LIABLE FOR ANY
 DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF PROFITS,
 BUSINESS INTERRUPTION, LOSS OF INFORMATION) ARISING OUT OF THE USE OF OR
 INABILITY TO USE THE MATERIALS, EVEN IF AMD HAS BEEN ADVISED OF THE POSSIBILITY
 OF SUCH DAMAGES.  BECAUSE SOME JURISDICTIONS PROHIBIT THE EXCLUSION OR LIMITATION
 OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES, THE ABOVE LIMITATION MAY
 NOT APPLY TO YOU.

 AMD does not assume any responsibility for any errors which may appear in the
 Materials nor any responsibility to support or update the Materials.  AMD retains
 the right to make changes to its test specifications at any time, without notice.

 NO SUPPORT OBLIGATION: AMD is not obligated to furnish, support, or make any
 further information, software, technical information, know-how, or show-how
 available to you.

 So that all may benefit from your experience, please report  any  problems
 or  suggestions about this software to 3dsdk.support@amd.com

 AMD Developer Technologies, M/S 585
 Advanced Micro Devices, Inc.
 5900 E. Ben White Blvd.
 Austin, TX 78741
 3dsdk.support@amd.com
******************************************************************************/
#pragma once

#include "stdafx.h"
#include "memcpy_amd.h"

/*****************************************************************************
MEMCPY_AMD.CPP
******************************************************************************/

// Very optimized memcpy() routine for all AMD Athlon and Duron family.
// This code uses any of FOUR different basic copy methods, depending
// on the transfer size.
// NOTE:  Since this code uses MOVNTQ (also known as "Non-Temporal MOV" or
// "Streaming Store"), and also uses the software prefetchnta instructions,
// be sure you're running on Athlon/Duron or other recent CPU before calling!

#define TINY_BLOCK_COPY 64       // upper limit for movsd type copy
// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".

#define IN_CACHE_COPY 64 * 1024  // upper limit for movq/movq copy w/SW prefetch
// Next is a copy that uses the MMX registers to copy 8 bytes at a time,
// also using the "unrolled loop" optimization.   This code uses
// the software prefetch instruction to get the data into the cache.

#define UNCACHED_COPY 197 * 1024 // upper limit for movq/movntq w/SW prefetch
// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.
// USE 64 * 1024 FOR THIS VALUE IF YOU'RE ALWAYS FILLING A "CLEAN CACHE"

#define BLOCK_PREFETCH_COPY  infinity // no limit for movq/movntq w/block prefetch 
#define CACHEBLOCK 80h // number of 64-byte blocks (cache lines) for block prefetch
// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch.  The technique is great for
// getting maximum read bandwidth, especially in DDR memory systems.

// Inline assembly syntax for use with Visual C++

/////////////////////////////////////////////////////////////////////////////
// katsyonak: Added MMX & SSE optimized memcpy - October 8, 2003		  //
//																		 //
// katsyonak: Added AMD, MMX & SSE optimized memset - October 12, 2003  //
//																	   //
// katsyonak: Added FPU optimized memcpy & memset - November 23, 2003 //
///////////////////////////////////////////////////////////////////////

unsigned long CPU_Type = 0;
// 0 = CPU check not performed yet (Auto detect)
// 1 = FPU
// 2 = MMX
// 3 = MMX2 for AMD Athlon/Duron and above (might also work on MMX2 (KATMAI) Intel machines)
// 4 = SSE
// 5 = SSE2 (only for Pentium 4 detection, the optimization used is SSE)
unsigned long get_cpu_type()
{
  __asm
  {
	mov			eax,[CPU_Type]
	or			eax,eax
	jne			ret_eax
	;xor			eax,eax
	cpuid
	or			eax,eax
	mov			eax,1 ;FPU
	je			cpu_done
	xor			esi,esi
	cmp			ebx,68747541h ;Auth
	jne			not_amd
	cmp			edx,69746E65h ;enti
	jne			not_amd
	cmp			ecx,444D4163h ;cAMD
	jne			not_amd
	inc			esi
not_amd:
	;mov			eax,1
	cpuid
	mov			al,1 ;FPU
	bt			edx,23 ;MMX Feature Bit
	jnb			ret_al
	or			esi,esi
	je			check_sse
	and			ah,1111b
	cmp			ah,6 ;model 6 (K7) = Athlon, Duron
	jb			cpu_mmx
	mov			eax,80000000h
	cpuid
	cmp			eax,80000000h
	jbe			cpu_mmx
	mov			eax,80000001h
	cpuid
	bt			edx,31 ;AMD Feature Bit
	jnb			cpu_mmx
	mov			al,3 ;AMD
	jmp			ret_al
check_sse:
	bt			edx,25 ;SSE Feature Bit
	jb			cpu_sse
cpu_mmx:
	mov			al,2
	jmp			ret_al
cpu_sse:
	mov			al,4 ;SSE
	bt			edx,26 ;SSE2 Feature Bit
	adc			al,0
ret_al:
	movzx		eax,al
cpu_done:
	mov			[CPU_Type],eax
ret_eax:
  }
}

#if !defined(MMX) && !defined(AMD) && !defined(SSE)

unsigned long memcpyProc = 0;
unsigned long memsetProc = 0;

void * memcpy_optimized(void *dest, const void *src, size_t n)
{
  __asm
  {
	cld
proc_start:
	mov			ebx, [n]		; number of bytes to copy
	mov			edi, [dest]		; destination
	mov			esi, [src]		; source

	cmp			ebx, TINY_BLOCK_COPY
	jb			$memcpy_ic_2	; tiny? skip optimized copy

	mov			ecx,[memcpyProc]
	jecxz		$memcpy_detect
	jmp			ecx

$memcpy_detect:
	call		get_cpu_type
	mov			ecx,offset copy_sse
	cmp			al,3
	ja			addr_done
	mov			ecx,offset copy_amd
	je			addr_done
	mov			ecx,offset copy_mmx
	cmp			al,1
	ja			addr_done
	mov			ecx,offset copy_fpu
addr_done:
	mov			[memcpyProc],ecx
	jmp			proc_start

align 16
copy_fpu:
	mov			ecx, ebx		; number of bytes left to copy
	shr			ecx, 6			; get 64-byte block count
	jz			$memcpy_ic_2	; finish the last few bytes
align 16
$memcpy_fpu_ic_1:			; 64-byte block copies, in-cache copy
	fld			double ptr [esi]	; read 64 bits
	fstp		double ptr [edi]
	fld			double ptr [esi+8]
	fstp		double ptr [edi+8]
	fld			double ptr [esi+16]
	fstp		double ptr [edi+16]
	fld			double ptr [esi+24]
	fstp		double ptr [edi+24]
	fld			double ptr [esi+32]
	fstp		double ptr [edi+32]
	fld			double ptr [esi+40]
	fstp		double ptr [edi+40]
	fld			double ptr [esi+48]
	fstp		double ptr [edi+48]
	fld			double ptr [esi+56]
	fstp		double ptr [edi+56]
	add			esi, 64			; update source pointer
	add			edi, 64			; update destination pointer
	dec			ecx				; count down
	jnz			$memcpy_fpu_ic_1	; last 64-byte block?
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_last_few
	jmp			ecx				; jump to array of movsd's

align 16
copy_sse:
	mov			ecx, 16			; a trick that's faster than rep movsb...
	sub			ecx, edi		; align source to qword
	and			ecx, 1111b		; get the low bits
	sub			ebx, ecx		; update copy count
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_sse_align_done
	jmp			ecx				; jump to array of movsb's

align 4
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb

$memcpy_sse_align_done:		; destination is double quadword aligned
	mov			ecx, ebx		; number of bytes left to copy
	shr			ecx, 6			; get 64-byte block count
	jz			$memcpy_ic_2	; finish the last few bytes

	test		esi,1111b		; Is the source address aligned?
	je			$memcpy_sse_ic_1_a

// This is small block copy that uses the SSE registers to copy 16 bytes
// at a time.  It uses the "unrolled loop" optimization, and also uses
// the software prefetch instruction to get the data into the cache.
align 16
$memcpy_sse_ic_1:			; 64-byte block copies, in-cache copy

	prefetchnta	[esi + 320]		; start reading ahead

	movups		xmm0, [esi+0]		; read 128 bits
	movntps		[edi+0], xmm0		; write 128 bits
	movups		xmm1, [esi+16]
	movntps		[edi+16], xmm1
	movups		xmm2, [esi+32]
	movntps		[edi+32], xmm2
	movups		xmm3, [esi+48]
	movntps		[edi+48], xmm3

	add			esi, 64				; update source pointer
	add			edi, 64				; update destination pointer
	dec			ecx					; count down
	jnz			$memcpy_sse_ic_1	; last 64-byte block?

$memcpy_ic_2:
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_last_few
	jmp			ecx				; jump to array of movsd's

align 16
$memcpy_sse_ic_1_a:				; 64-byte block copies, in-cache copy

	prefetchnta	[esi + 320]		; start reading ahead

	movaps		xmm0, [esi+0]		; read 128 bits
	movntps		[edi+0], xmm0		; write 128 bits
	movaps		xmm1, [esi+16]
	movntps		[edi+16], xmm1
	movaps		xmm2, [esi+32]
	movntps		[edi+32], xmm2
	movaps		xmm3, [esi+48]
	movntps		[edi+48], xmm3

	add			esi, 64				; update source pointer
	add			edi, 64				; update destination pointer
	dec			ecx					; count down
	jnz			$memcpy_sse_ic_1_a	; last 64-byte block?
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_last_few
	jmp			ecx				; jump to array of movsd's

align 16
copy_amd:
	cmp			ebx, 32*1024			; don't align between 32k-64k because
	jbe			$memcpy_amd_do_align	;  it appears to be slower
	cmp			ebx, 64*1024
	jbe			$memcpy_amd_align_done
$memcpy_amd_do_align:
	mov			ecx, 8			; a trick that's faster than rep movsb...
	sub			ecx, edi		; align destination to qword
	and			ecx, 111b		; get the low bits
	sub			ebx, ecx		; update copy count
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_amd_align_done
	jmp			ecx				; jump to array of movsb's

align 4
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb

$memcpy_amd_align_done:		; destination is dword aligned
	mov			ecx, ebx		; number of bytes left to copy
	shr			ecx, 6			; get 64-byte block count
	jz			$memcpy_ic_2	; finish the last few bytes

	cmp			ecx, IN_CACHE_COPY/64	; too big 4 cache? use uncached copy
	jae			$memcpy_amd_uc_test

// This is small block copy that uses the MMX registers to copy 8 bytes
// at a time.  It uses the "unrolled loop" optimization, and also uses
// the software prefetch instruction to get the data into the cache.
align 16
$memcpy_amd_ic_1:			; 64-byte block copies, in-cache copy

	prefetchnta	[esi + (200*64/34+192)]		; start reading ahead

	movq		mm0, [esi+0]	; read 64 bits
	movq		mm1, [esi+8]
	movq		[edi+0], mm0	; write 64 bits
	movq		[edi+8], mm1	;    note:  the normal movq writes the
	movq		mm2, [esi+16]	;    data to cache; a cache line will be
	movq		mm3, [esi+24]	;    allocated as needed, to store the data
	movq		[edi+16], mm2
	movq		[edi+24], mm3
	movq		mm0, [esi+32]
	movq		mm1, [esi+40]
	movq		[edi+32], mm0
	movq		[edi+40], mm1
	movq		mm2, [esi+48]
	movq		mm3, [esi+56]
	movq		[edi+48], mm2
	movq		[edi+56], mm3

	add			esi, 64			; update source pointer
	add			edi, 64			; update destination pointer
	dec			ecx				; count down
	jnz			$memcpy_amd_ic_1	; last 64-byte block?
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_last_few
	jmp			ecx				; jump to array of movsd's

$memcpy_amd_uc_test:
	cmp			ecx, UNCACHED_COPY/64	; big enough? use block prefetch copy
	jae			$memcpy_amd_bp_1

$memcpy_amd_64_test:
	or			ecx, ecx		; tail end of block prefetch will jump here
	jz			$memcpy_ic_2	; no more 64-byte blocks left

// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.
align 16
$memcpy_amd_uc_1:				; 64-byte blocks, uncached copy

	prefetchnta	[esi + (200*64/34+192)]		; start reading ahead

	movq		mm0,[esi+0]		; read 64 bits
	add			edi,64			; update destination pointer
	movq		mm1,[esi+8]
	add			esi,64			; update source pointer
	movq		mm2,[esi-48]
	movntq		[edi-64], mm0	; write 64 bits, bypassing the cache
	movq		mm0,[esi-40]	;    note: movntq also prevents the CPU
	movntq		[edi-56], mm1	;    from READING the destination address
	movq		mm1,[esi-32]	;    into the cache, only to be over-written
	movntq		[edi-48], mm2	;    so that also helps performance
	movq		mm2,[esi-24]
	movntq		[edi-40], mm0
	movq		mm0,[esi-16]
	movntq		[edi-32], mm1
	movq		mm1,[esi-8]
	movntq		[edi-24], mm2
	movntq		[edi-16], mm0
	dec			ecx
	movntq		[edi-8], mm1
	jnz			$memcpy_amd_uc_1	; last 64-byte block?
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_last_few
	jmp			ecx				; jump to array of movsd's

// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch, in this case.
// The technique is great for getting maximum read bandwidth,
// especially in DDR memory systems.
$memcpy_amd_bp_1:			; large blocks, block prefetch copy

	cmp			ecx, CACHEBLOCK			; big enough to run another prefetch loop?
	jl			$memcpy_amd_64_test		; no, back to regular uncached copy

	mov			eax, CACHEBLOCK / 2		; block prefetch loop, unrolled 2X
	add			esi, CACHEBLOCK * 64	; move to the top of the block
align 16
$memcpy_amd_bp_2:
	mov			edx, [esi-64]		; grab one address per cache line
	mov			edx, [esi-128]		; grab one address per cache line
	sub			esi, 128			; go reverse order
	dec			eax					; count down the cache lines
	jnz			$memcpy_amd_bp_2	; keep grabbing more lines into cache

	mov			eax, CACHEBLOCK		; now that it's in cache, do the copy
align 16
$memcpy_amd_bp_3:
	movq		mm0, [esi   ]		; read 64 bits
	movq		mm1, [esi+ 8]
	movq		mm2, [esi+16]
	movq		mm3, [esi+24]
	movq		mm4, [esi+32]
	movq		mm5, [esi+40]
	movq		mm6, [esi+48]
	movq		mm7, [esi+56]
	add			esi, 64				; update source pointer
	movntq		[edi   ], mm0		; write 64 bits, bypassing cache
	movntq		[edi+ 8], mm1		;    note: movntq also prevents the CPU
	movntq		[edi+16], mm2		;    from READING the destination address 
	movntq		[edi+24], mm3		;    into the cache, only to be over-written,
	movntq		[edi+32], mm4		;    so that also helps performance
	movntq		[edi+40], mm5
	movntq		[edi+48], mm6
	movntq		[edi+56], mm7
	add			edi, 64				; update dest pointer
	dec			eax					; count down
	jnz			$memcpy_amd_bp_3	; keep copying
	sub			ecx, CACHEBLOCK		; update the 64-byte block count
	jmp			$memcpy_amd_bp_1	; keep processing chunks

align 16
copy_mmx:
	mov			ecx, 8			; a trick that's faster than rep movsb...
	sub			ecx, edi		; align destination to qword
	and			ecx, 111b		; get the low bits
	sub			ebx, ecx		; update copy count
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_mmx_align_done
	jmp			ecx				; jump to array of movsb's

align 4
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb

$memcpy_mmx_align_done:		; destination is dword aligned
	mov			ecx, ebx		; number of bytes left to copy
	shr			ecx, 6			; get 64-byte block count
	jz			$memcpy_ic_2	; finish the last few bytes

align 16
$memcpy_mmx_ic_1:
	movq		mm0, [esi+0]	; read 64 bits
	movq		mm1, [esi+8]
	movq		[edi+0], mm0	; write 64 bits
	movq		[edi+8], mm1
	movq		mm2, [esi+16]
	movq		mm3, [esi+24]
	movq		[edi+16], mm2
	movq		[edi+24], mm3
	movq		mm0, [esi+32]
	movq		mm1, [esi+40]
	movq		[edi+32], mm0
	movq		[edi+40], mm1
	movq		mm2, [esi+48]
	movq		mm3, [esi+56]
	movq		[edi+48], mm2
	movq		[edi+56], mm3

	add			esi, 64			; update source pointer
	add			edi, 64			; update destination pointer
	dec			ecx				; count down
	jnz			$memcpy_mmx_ic_1	; last 64-byte block?
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_last_few
	jmp			ecx				; jump to array of movsd's

// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".   Then it handles the last few bytes.
align 4
	movsd
	movsd			; perform last 1-15 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd			; perform last 1-7 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd

$memcpy_last_few:			; dword aligned from before movsd's
	mov			ecx, ebx	; has valid low 2 bits of the byte count
	and			ecx, 11b	; the last few cows must come home
	rep			movsb		; the last 1, 2, or 3 bytes

	cmp			[CPU_Type],2
	jb			$memcpy_exit
	emms				; clean up the MMX state
	je			$memcpy_exit
	sfence				; flush the write buffer
$memcpy_exit:
	mov			eax, [dest]	; ret value = destination pointer
    }
}

void * memset_optimized(void *dest, int c, size_t n)
{
  __asm
  {
	cld
proc_start:
	mov			ebx, [n]		; number of bytes to fill
	mov			edi, [dest]		; destination
	movzx		eax, [c]		; character
	mov			ah,  al
	push		ax
	push		ax
	pop			eax
	cmp			ebx, TINY_BLOCK_COPY
	jb			$memset_ic_2	; tiny? skip optimized fill

	mov			ecx,[memsetProc]
	jecxz		$memset_detect
	jmp			ecx

$memset_detect:
	call		get_cpu_type
	mov			ecx,offset fill_sse
	cmp			al,3
	ja			addr_done
	mov			ecx,offset fill_amd
	je			addr_done
	mov			ecx,offset fill_mmx
	cmp			al,1
	ja			addr_done
	mov			ecx,offset fill_fpu
addr_done:
	mov			[memsetProc],ecx
	jmp			proc_start

align 16
fill_fpu:
	mov			ecx, ebx		; number of bytes left to fill
	shr			ecx, 6			; get 64-byte block count
	jz			$memset_ic_2	; finish the last few bytes
	push		eax
	push		eax
	fld			double ptr [esp]
	
align 16
$memset_fpu_ic_1:
	fst			double ptr [edi+ 0]	; write 64 bits
	fst			double ptr [edi+ 8]
	fst			double ptr [edi+16]
	fst			double ptr [edi+24]
	fst			double ptr [edi+32]
	fst			double ptr [edi+40]
	fst			double ptr [edi+48]
	fst			double ptr [edi+56]

	add			edi, 64			; update destination pointer
	dec			ecx				; count down
	jnz			$memset_fpu_ic_1	; last 64-byte block?
	fstp		double ptr [esp]
	pop			eax
	pop			eax
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memset_last_few
	jmp			ecx				; jump to array of stosd's

align 16
fill_sse:
	mov			ecx, 16			; a trick that's faster than rep stosb...
	sub			ecx, edi		; align source to qword
	and			ecx, 1111b		; get the low bits
	sub			ebx, ecx		; update copy count
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memset_sse_align_done
	jmp			ecx				; jump to array of stosb's

align 4
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb

$memset_sse_align_done:		; destination is double quadword aligned
	mov			ecx, ebx		; number of bytes left to fill
	shr			ecx, 6			; get 64-byte block count
	jz			$memset_ic_2	; finish the last few bytes
	push		eax
	push		eax
	push		eax
	push		eax
	movups		xmm0,[esp]
	pop			eax
	pop			eax
	pop			eax
	pop			eax

align 16
$memset_sse_ic_1:

	movntps		[edi+ 0], xmm0		; write 128 bits
	movntps		[edi+16], xmm0
	movntps		[edi+32], xmm0
	movntps		[edi+48], xmm0

	add			edi, 64				; update destination pointer
	dec			ecx					; count down
	jnz			$memset_sse_ic_1	; last 64-byte block?

$memset_ic_2:
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memset_last_few
	jmp			ecx				; jump to array of stosd's

align 16
fill_amd:
	mov			ecx, 8			; a trick that's faster than rep stosb...
	sub			ecx, edi		; align destination to qword
	and			ecx, 111b		; get the low bits
	sub			ebx, ecx		; update fill count
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memset_amd_align_done
	jmp			ecx				; jump to array of stosb's

align 4
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb

$memset_amd_align_done:		; destination is dword aligned
	mov			ecx, ebx		; number of bytes left to fill
	shr			ecx, 6			; get 64-byte block count
	jz			$memset_ic_2	; finish the last few bytes

	push		eax
	push		eax
	movq		mm0,[esp]
	pop			eax
	pop			eax

align 16
$memset_amd_ic_1:

	movntq		[edi+ 0], mm0	; write 64 bits
	movntq		[edi+ 8], mm0
	movntq		[edi+16], mm0
	movntq		[edi+24], mm0
	movntq		[edi+32], mm0
	movntq		[edi+40], mm0
	movntq		[edi+48], mm0
	movntq		[edi+56], mm0

	add			edi, 64			; update destination pointer
	dec			ecx				; count down
	jnz			$memset_amd_ic_1	; last 64-byte block?
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memset_last_few
	jmp			ecx				; jump to array of stosd's

align 16
fill_mmx:
	mov			ecx, 8			; a trick that's faster than rep stosb...
	sub			ecx, edi		; align destination to qword
	and			ecx, 111b		; get the low bits
	sub			ebx, ecx		; update fill count
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memset_mmx_align_done
	jmp			ecx				; jump to array of stosb's

align 4
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb

$memset_mmx_align_done:		; destination is dword aligned
	mov			ecx, ebx		; number of bytes left to fill
	shr			ecx, 6			; get 64-byte block count
	jz			$memset_ic_2	; finish the last few bytes
	push		eax
	push		eax
	movq		mm0,[esp]
	pop			eax
	pop			eax

align 16
$memset_mmx_ic_1:
	movq		[edi+ 0], mm0	; write 64 bits
	movq		[edi+ 8], mm0
	movq		[edi+16], mm0
	movq		[edi+24], mm0
	movq		[edi+32], mm0
	movq		[edi+40], mm0
	movq		[edi+48], mm0
	movq		[edi+56], mm0

	add			edi, 64			; update destination pointer
	dec			ecx				; count down
	jnz			$memset_mmx_ic_1	; last 64-byte block?
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memset_last_few
	jmp			ecx				; jump to array of stosd's

align 4
	stosd
	stosd			; perform last 1-15 dword fills
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd			; perform last 1-7 dword fills
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd

$memset_last_few:		; dword aligned from before stosd's
	mov			ecx, ebx	; has valid low 2 bits of the byte count
	and			ecx, 11b	; the last few cows must come home
	rep			stosb		; the last 1, 2, or 3 bytes

	cmp			[CPU_Type],2
	jb			$memset_exit
	emms				; clean up the MMX state
	je			$memset_exit
	sfence				; flush the write buffer
$memset_exit:
	mov			eax, [dest]	; ret value = destination pointer
    }
}
#endif

#ifdef AMD
// MMX2 (KATMAI) / AMD Athlon & Duron
void * memcpy_optimized(void *dest, const void *src, size_t n)
{
  __asm
  {
	cld
	mov			ebx, [n]		; number of bytes to copy
	mov			edi, [dest]		; destination
	mov			esi, [src]		; source
	cmp			ebx, TINY_BLOCK_COPY
	jb			$memcpy_ic_2	; tiny? skip mmx/sse copy
	cmp			ebx, 32*1024			; don't align between 32k-64k because
	jbe			$memcpy_amd_do_align	;  it appears to be slower
	cmp			ebx, 64*1024
	jbe			$memcpy_amd_align_done
$memcpy_amd_do_align:
	mov			ecx, 8			; a trick that's faster than rep movsb...
	sub			ecx, edi		; align destination to qword
	and			ecx, 111b		; get the low bits
	sub			ebx, ecx		; update copy count
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_amd_align_done
	jmp			ecx				; jump to array of movsb's

align 4
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb

$memcpy_amd_align_done:		; destination is dword aligned
	mov			ecx, ebx		; number of bytes left to copy
	shr			ecx, 6			; get 64-byte block count
	jz			$memcpy_ic_2	; finish the last few bytes

	cmp			ecx, IN_CACHE_COPY/64	; too big 4 cache? use uncached copy
	jae			$memcpy_amd_uc_test

// This is small block copy that uses the MMX registers to copy 8 bytes
// at a time.  It uses the "unrolled loop" optimization, and also uses
// the software prefetch instruction to get the data into the cache.
align 16
$memcpy_amd_ic_1:			; 64-byte block copies, in-cache copy

	prefetchnta	[esi + (200*64/34+192)]		; start reading ahead

	movq		mm0, [esi+0]	; read 64 bits
	movq		mm1, [esi+8]
	movq		[edi+0], mm0	; write 64 bits
	movq		[edi+8], mm1	;    note:  the normal movq writes the
	movq		mm2, [esi+16]	;    data to cache; a cache line will be
	movq		mm3, [esi+24]	;    allocated as needed, to store the data
	movq		[edi+16], mm2
	movq		[edi+24], mm3
	movq		mm0, [esi+32]
	movq		mm1, [esi+40]
	movq		[edi+32], mm0
	movq		[edi+40], mm1
	movq		mm2, [esi+48]
	movq		mm3, [esi+56]
	movq		[edi+48], mm2
	movq		[edi+56], mm3

	add			esi, 64			; update source pointer
	add			edi, 64			; update destination pointer
	dec			ecx				; count down
	jnz			$memcpy_amd_ic_1	; last 64-byte block?
$memcpy_ic_2:
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_last_few
	jmp			ecx				; jump to array of movsd's

$memcpy_amd_uc_test:
	cmp			ecx, UNCACHED_COPY/64	; big enough? use block prefetch copy
	jae			$memcpy_amd_bp_1

$memcpy_amd_64_test:
	or			ecx, ecx		; tail end of block prefetch will jump here
	jz			$memcpy_ic_2	; no more 64-byte blocks left

// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.
align 16
$memcpy_amd_uc_1:				; 64-byte blocks, uncached copy

	prefetchnta	[esi + (200*64/34+192)]		; start reading ahead

	movq		mm0,[esi+0]		; read 64 bits
	add			edi,64			; update destination pointer
	movq		mm1,[esi+8]
	add			esi,64			; update source pointer
	movq		mm2,[esi-48]
	movntq		[edi-64], mm0	; write 64 bits, bypassing the cache
	movq		mm0,[esi-40]	;    note: movntq also prevents the CPU
	movntq		[edi-56], mm1	;    from READING the destination address
	movq		mm1,[esi-32]	;    into the cache, only to be over-written
	movntq		[edi-48], mm2	;    so that also helps performance
	movq		mm2,[esi-24]
	movntq		[edi-40], mm0
	movq		mm0,[esi-16]
	movntq		[edi-32], mm1
	movq		mm1,[esi-8]
	movntq		[edi-24], mm2
	movntq		[edi-16], mm0
	dec			ecx
	movntq		[edi-8], mm1
	jnz			$memcpy_amd_uc_1	; last 64-byte block?
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_last_few
	jmp			ecx				; jump to array of movsd's

// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch, in this case.
// The technique is great for getting maximum read bandwidth,
// especially in DDR memory systems.
$memcpy_amd_bp_1:			; large blocks, block prefetch copy

	cmp			ecx, CACHEBLOCK			; big enough to run another prefetch loop?
	jl			$memcpy_amd_64_test		; no, back to regular uncached copy

	mov			eax, CACHEBLOCK / 2		; block prefetch loop, unrolled 2X
	add			esi, CACHEBLOCK * 64	; move to the top of the block
align 16
$memcpy_amd_bp_2:
	mov			edx, [esi-64]		; grab one address per cache line
	mov			edx, [esi-128]		; grab one address per cache line
	sub			esi, 128			; go reverse order
	dec			eax					; count down the cache lines
	jnz			$memcpy_amd_bp_2	; keep grabbing more lines into cache

	mov			eax, CACHEBLOCK		; now that it's in cache, do the copy
align 16
$memcpy_amd_bp_3:
	movq		mm0, [esi   ]		; read 64 bits
	movq		mm1, [esi+ 8]
	movq		mm2, [esi+16]
	movq		mm3, [esi+24]
	movq		mm4, [esi+32]
	movq		mm5, [esi+40]
	movq		mm6, [esi+48]
	movq		mm7, [esi+56]
	add			esi, 64				; update source pointer
	movntq		[edi   ], mm0		; write 64 bits, bypassing cache
	movntq		[edi+ 8], mm1		;    note: movntq also prevents the CPU
	movntq		[edi+16], mm2		;    from READING the destination address 
	movntq		[edi+24], mm3		;    into the cache, only to be over-written,
	movntq		[edi+32], mm4		;    so that also helps performance
	movntq		[edi+40], mm5
	movntq		[edi+48], mm6
	movntq		[edi+56], mm7
	add			edi, 64				; update dest pointer
	dec			eax					; count down
	jnz			$memcpy_amd_bp_3	; keep copying
	sub			ecx, CACHEBLOCK		; update the 64-byte block count
	jmp			$memcpy_amd_bp_1	; keep processing chunks

// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".   Then it handles the last few bytes.
align 4
	movsd
	movsd			; perform last 1-15 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd			; perform last 1-7 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd

$memcpy_last_few:		; dword aligned from before movsd's
	mov			ecx, ebx	; has valid low 2 bits of the byte count
	and			ecx, 11b	; the last few cows must come home
	rep			movsb		; the last 1, 2, or 3 bytes

	emms				; clean up the MMX state
	sfence				; flush the write buffer
	mov			eax, [dest]	; ret value = destination pointer
    }
}

void * memset_optimized(void *dest, int c, size_t n)
{
  __asm
  {
	cld
	mov			ebx, [n]		; number of bytes to fill
	mov			edi, [dest]		; destination
	movzx		eax, [c]		; character
	mov			ah,al
	push		ax
	push		ax
	pop			eax
	cmp			ebx, TINY_BLOCK_COPY
	jb			$memset_ic_2	; tiny? skip optimized fill
	mov			ecx, 8			; a trick that's faster than rep stosb...
	sub			ecx, edi		; align destination to qword
	and			ecx, 111b		; get the low bits
	sub			ebx, ecx		; update fill count
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memset_amd_align_done
	jmp			ecx				; jump to array of stosb's

align 4
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb

$memset_amd_align_done:		; destination is dword aligned
	mov			ecx, ebx		; number of bytes left to fill
	shr			ecx, 6			; get 64-byte block count
	jz			$memset_ic_2	; finish the last few bytes
	push		eax
	push		eax
	movq		mm0,[esp]
	pop			eax
	pop			eax

align 16
$memset_amd_ic_1:

	movntq		[edi+ 0], mm0	; write 64 bits
	movntq		[edi+ 8], mm0
	movntq		[edi+16], mm0
	movntq		[edi+24], mm0
	movntq		[edi+32], mm0
	movntq		[edi+40], mm0
	movntq		[edi+48], mm0
	movntq		[edi+56], mm0

	add			edi, 64			; update destination pointer
	dec			ecx				; count down
	jnz			$memset_amd_ic_1	; last 64-byte block?
$memset_ic_2:
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memset_last_few
	jmp			ecx				; jump to array of stosd's

align 4
	stosd
	stosd			; perform last 1-15 dword fills
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd			; perform last 1-7 dword fills
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd

$memset_last_few:		; dword aligned from before stosd's
	mov			ecx, ebx	; has valid low 2 bits of the byte count
	and			ecx, 11b	; the last few cows must come home
	rep			stosb		; the last 1, 2, or 3 bytes

	emms				; clean up the MMX state
	sfence				; flush the write buffer
	mov			eax, [dest]	; ret value = destination pointer
    }
}
#endif AMD

#ifdef SSE
void * memcpy_optimized(void *dest, const void *src, size_t n)
{
  __asm
  {
	cld
	mov			ebx, [n]		; number of bytes to copy
	mov			edi, [dest]		; destination
	mov			esi, [src]		; source
	cmp			ebx, TINY_BLOCK_COPY
	jb			$memcpy_ic_2	; tiny? skip sse copy
	mov			ecx, 16			; a trick that's faster than rep movsb...
	sub			ecx, edi		; align source to qword
	and			ecx, 1111b		; get the low bits
	sub			ebx, ecx		; update copy count
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_sse_align_done
	jmp			ecx				; jump to array of movsb's

align 4
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb

$memcpy_sse_align_done:		; destination is double quadword aligned
	mov			ecx, ebx		; number of bytes left to copy
	shr			ecx, 6			; get 64-byte block count
	jz			$memcpy_ic_2	; finish the last few bytes

	test		esi,1111b		; Is source address aligned?
	je			$memcpy_sse_ic_1_a

align 16
$memcpy_sse_ic_1:			; 64-byte block copies, in-cache copy

	prefetchnta	[esi + 320]		; start reading ahead

	movups		xmm0, [esi+0]		; read 128 bits
	movntps		[edi+0], xmm0		; write 128 bits
	movups		xmm1, [esi+16]
	movntps		[edi+16], xmm1
	movups		xmm2, [esi+32]
	movntps		[edi+32], xmm2
	movups		xmm3, [esi+48]
	movntps		[edi+48], xmm3

	add			esi, 64				; update source pointer
	add			edi, 64				; update destination pointer
	dec			ecx					; count down
	jnz			$memcpy_sse_ic_1	; last 64-byte block?

$memcpy_ic_2:
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_last_few
	jmp			ecx				; jump to array of movsd's

align 16
$memcpy_sse_ic_1_a:				; 64-byte block copies, in-cache copy

	prefetchnta	[esi + 320]		; start reading ahead

	movaps		xmm0, [esi+0]		; read 128 bits
	movntps		[edi+0], xmm0		; write 128 bits
	movaps		xmm1, [esi+16]
	movntps		[edi+16], xmm1
	movaps		xmm2, [esi+32]
	movntps		[edi+32], xmm2
	movaps		xmm3, [esi+48]
	movntps		[edi+48], xmm3

	add			esi, 64				; update source pointer
	add			edi, 64				; update destination pointer
	dec			ecx					; count down
	jnz			$memcpy_sse_ic_1_a	; last 64-byte block?
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_last_few
	jmp			ecx				; jump to array of movsd's

align 4
	movsd
	movsd			; perform last 1-15 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd			; perform last 1-7 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd

$memcpy_last_few:		; dword aligned from before movsd's
	mov			ecx, ebx	; has valid low 2 bits of the byte count
	and			ecx, 11b	; the last few cows must come home
	rep			movsb		; the last 1, 2, or 3 bytes

	emms				; clean up the MMX state
	sfence				; flush the write buffer
	mov			eax, [dest]	; ret value = destination pointer
    }
}

void * memset_optimized(void *dest, int c, size_t n)
{
  __asm
  {
	cld
	mov			ebx, [n]		; number of bytes to fill
	mov			edi, [dest]		; destination
	movzx		eax, [c]		; character
	mov			ah,	 al
	push		ax
	push		ax
	pop			eax
	cmp			ebx, TINY_BLOCK_COPY
	jb			$memset_ic_2	; tiny? skip optimized fill
	mov			ecx, 16			; a trick that's faster than rep stosb...
	sub			ecx, edi		; align source to qword
	and			ecx, 1111b		; get the low bits
	sub			ebx, ecx		; update copy count
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memset_sse_align_done
	jmp			ecx				; jump to array of stosb's

align 4
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb

$memset_sse_align_done:		; destination is double quadword aligned
	mov			ecx, ebx		; number of bytes left to fill
	shr			ecx, 6			; get 64-byte block count
	jz			$memset_ic_2	; finish the last few bytes
	push		eax
	push		eax
	push		eax
	push		eax
	movups		xmm0,[esp]
	pop			eax
	pop			eax
	pop			eax
	pop			eax

align 16
$memset_sse_ic_1:

	movntps		[edi+ 0], xmm0		; write 128 bits
	movntps		[edi+16], xmm0
	movntps		[edi+32], xmm0
	movntps		[edi+48], xmm0

	add			edi, 64				; update destination pointer
	dec			ecx					; count down
	jnz			$memset_sse_ic_1	; last 64-byte block?

$memset_ic_2:
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memset_last_few
	jmp			ecx				; jump to array of stosd's

align 4
	stosd
	stosd			; perform last 1-15 dword fills
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd			; perform last 1-7 dword fills
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd

$memset_last_few:		; dword aligned from before stosd's
	mov			ecx, ebx	; has valid low 2 bits of the byte count
	and			ecx, 11b	; the last few cows must come home
	rep			stosb		; the last 1, 2, or 3 bytes

	emms				; clean up the MMX state
	sfence				; flush the write buffer
	mov			eax, [dest]	; ret value = destination pointer
    }
}
#endif SSE

#ifdef MMX
void * memcpy_optimized(void *dest, const void *src, size_t n)
{
  __asm
  {
	cld
	mov			ebx, [n]		; number of bytes to copy
	mov			edi, [dest]		; destination
	mov			esi, [src]		; source
	cmp			ebx, TINY_BLOCK_COPY
	jb			$memcpy_ic_2	; tiny? skip optimized copy
	mov			ecx, 8			; a trick that's faster than rep movsb...
	sub			ecx, edi		; align destination to qword
	and			ecx, 111b		; get the low bits
	sub			ebx, ecx		; update copy count
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_mmx_align_done
	jmp			ecx				; jump to array of movsb's

align 4
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb

$memcpy_mmx_align_done:		; destination is dword aligned
	mov			ecx, ebx		; number of bytes left to copy
	shr			ecx, 6			; get 64-byte block count
	jz			$memcpy_ic_2	; finish the last few bytes

align 16
$memcpy_mmx_ic_1:			; 64-byte block copies, in-cache copy
	movq		mm0, [esi+0]	; read 64 bits
	movq		mm1, [esi+8]
	movq		[edi+0], mm0	; write 64 bits
	movq		[edi+8], mm1
	movq		mm2, [esi+16]
	movq		mm3, [esi+24]
	movq		[edi+16], mm2
	movq		[edi+24], mm3
	movq		mm0, [esi+32]
	movq		mm1, [esi+40]
	movq		[edi+32], mm0
	movq		[edi+40], mm1
	movq		mm2, [esi+48]
	movq		mm3, [esi+56]
	movq		[edi+48], mm2
	movq		[edi+56], mm3

	add			esi, 64			; update source pointer
	add			edi, 64			; update destination pointer
	dec			ecx				; count down
	jnz			$memcpy_mmx_ic_1	; last 64-byte block?
$memcpy_ic_2:
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memcpy_last_few
	jmp			ecx				; jump to array of movsd's

align 4
	movsd
	movsd			; perform last 1-15 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd			; perform last 1-7 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd

$memcpy_last_few:		; dword aligned from before movsd's
	mov			ecx, ebx	; has valid low 2 bits of the byte count
	and			ecx, 11b	; the last few cows must come home
	rep			movsb		; the last 1, 2, or 3 bytes

	emms				; clean up the MMX state
	mov			eax, [dest]	; ret value = destination pointer
    }
}

void * memset_optimized(void *dest, int c, size_t n)
{
  __asm
  {
	cld
	mov			ebx, [n]		; number of bytes to fill
	mov			edi, [dest]		; destination
	movzx		eax, [c]		; character
	mov			ah,al
	push		ax
	push		ax
	pop			eax
	cmp			ebx, TINY_BLOCK_COPY
	jb			$memset_ic_2	; tiny? skip optimized copy
	mov			ecx, 8			; a trick that's faster than rep stosb...
	sub			ecx, edi		; align destination to qword
	and			ecx, 111b		; get the low bits
	sub			ebx, ecx		; update fill count
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memset_mmx_align_done
	jmp			ecx				; jump to array of stosb's

align 4
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb
	stosb

$memset_mmx_align_done:		; destination is dword aligned
	mov			ecx, ebx		; number of bytes left to fill
	shr			ecx, 6			; get 64-byte block count
	jz			$memset_ic_2	; finish the last few bytes
	push		eax
	push		eax
	movq		mm0,[esp]
	pop			eax
	pop			eax

align 16
$memset_mmx_ic_1:
	movq		[edi+ 0], mm0	; write 64 bits
	movq		[edi+ 8], mm0
	movq		[edi+16], mm0
	movq		[edi+24], mm0
	movq		[edi+32], mm0
	movq		[edi+40], mm0
	movq		[edi+48], mm0
	movq		[edi+56], mm0

	add			edi, 64			; update destination pointer
	dec			ecx				; count down
	jnz			$memset_mmx_ic_1	; last 64-byte block?
$memset_ic_2:
	mov			ecx, ebx		; has valid low 6 bits of the byte count
	shr			ecx, 2			; dword count
	and			ecx, 1111b		; only look at the "remainder" bits
	neg			ecx				; set up to jump into the array
	add			ecx, offset $memset_last_few
	jmp			ecx				; jump to array of stosd's

align 4
	stosd
	stosd			; perform last 1-15 dword fills
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd			; perform last 1-7 dword fills
	stosd
	stosd
	stosd
	stosd
	stosd
	stosd

$memset_last_few:		; dword aligned from before stosd's
	mov			ecx, ebx	; has valid low 2 bits of the byte count
	and			ecx, 11b	; the last few cows must come home
	rep			stosb		; the last 1, 2, or 3 bytes

	emms				; clean up the MMX state
	mov			eax, [dest]	; ret value = destination pointer
    }
}
#endif MMX
