// integer.cpp - written and placed in the public domain by Wei Dai
// contains public domain code contributed by Alister Lee and Leonard Janke

#include "pch.h"
#include "integer.h"
#include "modarith.h"
#include "nbtheory.h"
#include "asn.h"
#include "oids.h"
#include "words.h"
#include "algparam.h"
#include "pubkey.h"		// for P1363_KDF2
#include "sha.h"

#include <iostream>

#ifdef SSE2_INTRINSICS_AVAILABLE
#include <emmintrin.h>
#endif

#include "algebra.cpp"
#include "eprecomp.cpp"

NAMESPACE_BEGIN(CryptoPP)

bool FunctionAssignIntToInteger(const std::type_info &valueType, void *pInteger, const void *pInt)
{
	if (valueType != typeid(Integer))
		return false;
	*reinterpret_cast<Integer *>(pInteger) = *reinterpret_cast<const int *>(pInt);
	return true;
}

static int DummyAssignIntToInteger = (AssignIntToInteger = FunctionAssignIntToInteger, 0);

#ifdef SSE2_INTRINSICS_AVAILABLE
template <class T>
 CPP_TYPENAME AllocatorBase<T>::pointer AlignedAllocator<T>::allocate(size_type n, const void *)
  {
  	if (n < 4)
  		return new T[n];

	else
		return (T *)_mm_malloc(sizeof(T)*n, 16);

}

template <class T>
void AlignedAllocator<T>::deallocate(void *p, size_type n)
{
	memset(p, 0, n*sizeof(T));
	if (n < 4)
		delete [] p;
	else
		_mm_free(p);
}

template class AlignedAllocator<word>;
#endif

#define MAKE_DWORD(lowWord, highWord) ((dword(highWord)<<WORD_BITS) | (lowWord))

static int Compare(const word *A, const word *B, unsigned int N)
{
	while (N--)
		if (A[N] > B[N])
			return 1;
		else if (A[N] < B[N])
			return -1;

	return 0;
}

static word Increment(word *A, unsigned int N, word B=1)
{
	assert(N);
	word t = A[0];
	A[0] = t+B;
	if (A[0] >= t)
		return 0;
	for (unsigned i=1; i<N; i++)
		if (++A[i])
			return 0;
	return 1;
}

static word Decrement(word *A, unsigned int N, word B=1)
{
	assert(N);
	word t = A[0];
	A[0] = t-B;
	if (A[0] <= t)
		return 0;
	for (unsigned i=1; i<N; i++)
		if (A[i]--)
			return 0;
	return 1;
}

static void TwosComplement(word *A, unsigned int N)
{
	Decrement(A, N);
	for (unsigned i=0; i<N; i++)
		A[i] = ~A[i];
}

static word LinearMultiply(word *C, const word *A, word B, unsigned int N)
{
	word carry=0;
	for(unsigned i=0; i<N; i++)
	{
		dword p = (dword)A[i] * B + carry;
		C[i] = LOW_WORD(p);
		carry = HIGH_WORD(p);
	}
	return carry;
}

static void AtomicInverseModPower2(word *C, word A0, word A1)
{
	assert(A0%2==1);

	dword A=MAKE_DWORD(A0, A1), R=A0%8;

	for (unsigned i=3; i<2*WORD_BITS; i*=2)
		R = R*(2-R*A);

	assert(R*A==1);

	C[0] = LOW_WORD(R);
	C[1] = HIGH_WORD(R);
}

// ********************************************************

class Portable
{
public:
	static word Add(word *C, const word *A, const word *B, unsigned int N);
	static word Subtract(word *C, const word *A, const word *B, unsigned int N);

	static inline void Multiply2(word *C, const word *A, const word *B);
	static inline word Multiply2Add(word *C, const word *A, const word *B);
	static void Multiply4(word *C, const word *A, const word *B);
	static void Multiply8(word *C, const word *A, const word *B);
	static inline unsigned int MultiplyRecursionLimit() {return 8;}

	static inline void Multiply2Bottom(word *C, const word *A, const word *B);
	static void Multiply4Bottom(word *C, const word *A, const word *B);
	static void Multiply8Bottom(word *C, const word *A, const word *B);
	static inline unsigned int MultiplyBottomRecursionLimit() {return 8;}

	static void Square2(word *R, const word *A);
	static void Square4(word *R, const word *A);
	static void Square8(word *R, const word *A) {assert(false);}
	static inline unsigned int SquareRecursionLimit() {return 4;}
};

word Portable::Add(word *C, const word *A, const word *B, unsigned int N)
{
	assert (N%2 == 0);

#ifdef IS_LITTLE_ENDIAN
	if (sizeof(dword) == sizeof(size_t))	// dword is only register size
	{
		dword carry = 0;
		N >>= 1;
		for (unsigned int i = 0; i < N; i++)
		{
			dword a = ((const dword *)A)[i] + carry;
			dword c = a + ((const dword *)B)[i];
			((dword *)C)[i] = c;
			carry = (a < carry) | (c < a);
		}
		return (word)carry;
	}
	else
#endif
	{
		word carry = 0;
		for (unsigned int i = 0; i < N; i+=2)
		{
			dword u = (dword) carry + A[i] + B[i];
			C[i] = LOW_WORD(u);
			u = (dword) HIGH_WORD(u) + A[i+1] + B[i+1];
			C[i+1] = LOW_WORD(u);
			carry = HIGH_WORD(u);
		}
		return carry;
	}
}

word Portable::Subtract(word *C, const word *A, const word *B, unsigned int N)
{
	assert (N%2 == 0);

#ifdef IS_LITTLE_ENDIAN
	if (sizeof(dword) == sizeof(size_t))	// dword is only register size
	{
		dword borrow = 0;
		N >>= 1;
		for (unsigned int i = 0; i < N; i++)
		{
			dword a = ((const dword *)A)[i];
			dword b = a - borrow;
			dword c = b - ((const dword *)B)[i];
			((dword *)C)[i] = c;
			borrow = (b > a) | (c > b);
		}
		return (word)borrow;
	}
	else
#endif
	{
		word borrow=0;
		for (unsigned i = 0; i < N; i+=2)
		{
			dword u = (dword) A[i] - B[i] - borrow;
			C[i] = LOW_WORD(u);
			u = (dword) A[i+1] - B[i+1] - (word)(0-HIGH_WORD(u));
			C[i+1] = LOW_WORD(u);
			borrow = 0-HIGH_WORD(u);
		}
		return borrow;
	}
}

void Portable::Multiply2(word *C, const word *A, const word *B)
{
/*
	word s;
	dword d;

	if (A1 >= A0)
		if (B0 >= B1)
		{
			s = 0;
			d = (dword)(A1-A0)*(B0-B1);
		}
		else
		{
			s = (A1-A0);
			d = (dword)s*(word)(B0-B1);
		}
	else
		if (B0 > B1)
		{
			s = (B0-B1);
			d = (word)(A1-A0)*(dword)s;
		}
		else
		{
			s = 0;
			d = (dword)(A0-A1)*(B1-B0);
		}
*/
	// this segment is the branchless equivalent of above
	word D[4] = {A[1]-A[0], A[0]-A[1], B[0]-B[1], B[1]-B[0]};
	unsigned int ai = A[1] < A[0];
	unsigned int bi = B[0] < B[1];
	unsigned int di = ai & bi;
	dword d = (dword)D[di]*D[di+2];
	D[1] = D[3] = 0;
	unsigned int si = ai + !bi;
	word s = D[si];

	dword A0B0 = (dword)A[0]*B[0];
	C[0] = LOW_WORD(A0B0);

	dword A1B1 = (dword)A[1]*B[1];
	dword t = (dword) HIGH_WORD(A0B0) + LOW_WORD(A0B0) + LOW_WORD(d) + LOW_WORD(A1B1);
	C[1] = LOW_WORD(t);

	t = A1B1 + HIGH_WORD(t) + HIGH_WORD(A0B0) + HIGH_WORD(d) + HIGH_WORD(A1B1) - s;
	C[2] = LOW_WORD(t);
	C[3] = HIGH_WORD(t);
}

inline void Portable::Multiply2Bottom(word *C, const word *A, const word *B)
{
#ifdef IS_LITTLE_ENDIAN
	if (sizeof(dword) == sizeof(size_t))
	{
		dword a = *(const dword *)A, b = *(const dword *)B;
		((dword *)C)[0] = a*b;
	}
	else
#endif
	{
		dword t = (dword)A[0]*B[0];
		C[0] = LOW_WORD(t);
		C[1] = HIGH_WORD(t) + A[0]*B[1] + A[1]*B[0];
	}
}

word Portable::Multiply2Add(word *C, const word *A, const word *B)
{
	word D[4] = {A[1]-A[0], A[0]-A[1], B[0]-B[1], B[1]-B[0]};
	unsigned int ai = A[1] < A[0];
	unsigned int bi = B[0] < B[1];
	unsigned int di = ai & bi;
	dword d = (dword)D[di]*D[di+2];
	D[1] = D[3] = 0;
	unsigned int si = ai + !bi;
	word s = D[si];

	dword A0B0 = (dword)A[0]*B[0];
	dword t = A0B0 + C[0];
	C[0] = LOW_WORD(t);

	dword A1B1 = (dword)A[1]*B[1];
	t = (dword) HIGH_WORD(t) + LOW_WORD(A0B0) + LOW_WORD(d) + LOW_WORD(A1B1) + C[1];
	C[1] = LOW_WORD(t);

	t = (dword) HIGH_WORD(t) + LOW_WORD(A1B1) + HIGH_WORD(A0B0) + HIGH_WORD(d) + HIGH_WORD(A1B1) - s + C[2];
	C[2] = LOW_WORD(t);

	t = (dword) HIGH_WORD(t) + HIGH_WORD(A1B1) + C[3];
	C[3] = LOW_WORD(t);
	return HIGH_WORD(t);
}

#define MulAcc(x, y)								\
	p = (dword)A[x] * B[y] + c; 					\
	c = LOW_WORD(p);								\
	p = (dword)d + HIGH_WORD(p);					\
	d = LOW_WORD(p);								\
	e += HIGH_WORD(p);

#define SaveMulAcc(s, x, y) 						\
	R[s] = c;										\
	p = (dword)A[x] * B[y] + d; 					\
	c = LOW_WORD(p);								\
	p = (dword)e + HIGH_WORD(p);					\
	d = LOW_WORD(p);								\
	e = HIGH_WORD(p);

#define SquAcc(x, y)								\
	q = (dword)A[x] * A[y];	\
	p = q + c; 					\
	c = LOW_WORD(p);								\
	p = (dword)d + HIGH_WORD(p);					\
	d = LOW_WORD(p);								\
	e += HIGH_WORD(p);			\
	p = q + c; 					\
	c = LOW_WORD(p);								\
	p = (dword)d + HIGH_WORD(p);					\
	d = LOW_WORD(p);								\
	e += HIGH_WORD(p);

#define SaveSquAcc(s, x, y) 						\
	R[s] = c;										\
	q = (dword)A[x] * A[y];	\
	p = q + d; 					\
	c = LOW_WORD(p);								\
	p = (dword)e + HIGH_WORD(p);					\
	d = LOW_WORD(p);								\
	e = HIGH_WORD(p);			\
	p = q + c; 					\
	c = LOW_WORD(p);								\
	p = (dword)d + HIGH_WORD(p);					\
	d = LOW_WORD(p);								\
	e += HIGH_WORD(p);

void Portable::Multiply4(word *R, const word *A, const word *B)
{
	dword p;
	word c, d, e;

	p = (dword)A[0] * B[0];
	R[0] = LOW_WORD(p);
	c = HIGH_WORD(p);
	d = e = 0;

	MulAcc(0, 1);
	MulAcc(1, 0);

	SaveMulAcc(1, 2, 0);
	MulAcc(1, 1);
	MulAcc(0, 2);

	SaveMulAcc(2, 0, 3);
	MulAcc(1, 2);
	MulAcc(2, 1);
	MulAcc(3, 0);

	SaveMulAcc(3, 3, 1);
	MulAcc(2, 2);
	MulAcc(1, 3);

	SaveMulAcc(4, 2, 3);
	MulAcc(3, 2);

	R[5] = c;
	p = (dword)A[3] * B[3] + d;
	R[6] = LOW_WORD(p);
	R[7] = e + HIGH_WORD(p);
}

void Portable::Square2(word *R, const word *A)
{
	dword p, q;
	word c, d, e;

	p = (dword)A[0] * A[0];
	R[0] = LOW_WORD(p);
	c = HIGH_WORD(p);
	d = e = 0;

	SquAcc(0, 1);

	R[1] = c;
	p = (dword)A[1] * A[1] + d;
	R[2] = LOW_WORD(p);
	R[3] = e + HIGH_WORD(p);
}

void Portable::Square4(word *R, const word *A)
{
	const word *B = A;
	dword p, q;
	word c, d, e;

	p = (dword)A[0] * A[0];
	R[0] = LOW_WORD(p);
	c = HIGH_WORD(p);
	d = e = 0;

	SquAcc(0, 1);

	SaveSquAcc(1, 2, 0);
	MulAcc(1, 1);

	SaveSquAcc(2, 0, 3);
	SquAcc(1, 2);

	SaveSquAcc(3, 3, 1);
	MulAcc(2, 2);

	SaveSquAcc(4, 2, 3);

	R[5] = c;
	p = (dword)A[3] * A[3] + d;
	R[6] = LOW_WORD(p);
	R[7] = e + HIGH_WORD(p);
}

void Portable::Multiply8(word *R, const word *A, const word *B)
{
	dword p;
	word c, d, e;

	p = (dword)A[0] * B[0];
	R[0] = LOW_WORD(p);
	c = HIGH_WORD(p);
	d = e = 0;

	MulAcc(0, 1);
	MulAcc(1, 0);

	SaveMulAcc(1, 2, 0);
	MulAcc(1, 1);
	MulAcc(0, 2);

	SaveMulAcc(2, 0, 3);
	MulAcc(1, 2);
	MulAcc(2, 1);
	MulAcc(3, 0);

	SaveMulAcc(3, 0, 4);
	MulAcc(1, 3);
	MulAcc(2, 2);
	MulAcc(3, 1);
	MulAcc(4, 0);

	SaveMulAcc(4, 0, 5);
	MulAcc(1, 4);
	MulAcc(2, 3);
	MulAcc(3, 2);
	MulAcc(4, 1);
	MulAcc(5, 0);

	SaveMulAcc(5, 0, 6);
	MulAcc(1, 5);
	MulAcc(2, 4);
	MulAcc(3, 3);
	MulAcc(4, 2);
	MulAcc(5, 1);
	MulAcc(6, 0);

	SaveMulAcc(6, 0, 7);
	MulAcc(1, 6);
	MulAcc(2, 5);
	MulAcc(3, 4);
	MulAcc(4, 3);
	MulAcc(5, 2);
	MulAcc(6, 1);
	MulAcc(7, 0);

	SaveMulAcc(7, 1, 7);
	MulAcc(2, 6);
	MulAcc(3, 5);
	MulAcc(4, 4);
	MulAcc(5, 3);
	MulAcc(6, 2);
	MulAcc(7, 1);

	SaveMulAcc(8, 2, 7);
	MulAcc(3, 6);
	MulAcc(4, 5);
	MulAcc(5, 4);
	MulAcc(6, 3);
	MulAcc(7, 2);

	SaveMulAcc(9, 3, 7);
	MulAcc(4, 6);
	MulAcc(5, 5);
	MulAcc(6, 4);
	MulAcc(7, 3);

	SaveMulAcc(10, 4, 7);
	MulAcc(5, 6);
	MulAcc(6, 5);
	MulAcc(7, 4);

	SaveMulAcc(11, 5, 7);
	MulAcc(6, 6);
	MulAcc(7, 5);

	SaveMulAcc(12, 6, 7);
	MulAcc(7, 6);

	R[13] = c;
	p = (dword)A[7] * B[7] + d;
	R[14] = LOW_WORD(p);
	R[15] = e + HIGH_WORD(p);
}

void Portable::Multiply4Bottom(word *R, const word *A, const word *B)
{
	dword p;
	word c, d, e;

	p = (dword)A[0] * B[0];
	R[0] = LOW_WORD(p);
	c = HIGH_WORD(p);
	d = e = 0;

	MulAcc(0, 1);
	MulAcc(1, 0);

	SaveMulAcc(1, 2, 0);
	MulAcc(1, 1);
	MulAcc(0, 2);

	R[2] = c;
	R[3] = d + A[0] * B[3] + A[1] * B[2] + A[2] * B[1] + A[3] * B[0];
}

void Portable::Multiply8Bottom(word *R, const word *A, const word *B)
{
	dword p;
	word c, d, e;

	p = (dword)A[0] * B[0];
	R[0] = LOW_WORD(p);
	c = HIGH_WORD(p);
	d = e = 0;

	MulAcc(0, 1);
	MulAcc(1, 0);

	SaveMulAcc(1, 2, 0);
	MulAcc(1, 1);
	MulAcc(0, 2);

	SaveMulAcc(2, 0, 3);
	MulAcc(1, 2);
	MulAcc(2, 1);
	MulAcc(3, 0);

	SaveMulAcc(3, 0, 4);
	MulAcc(1, 3);
	MulAcc(2, 2);
	MulAcc(3, 1);
	MulAcc(4, 0);

	SaveMulAcc(4, 0, 5);
	MulAcc(1, 4);
	MulAcc(2, 3);
	MulAcc(3, 2);
	MulAcc(4, 1);
	MulAcc(5, 0);

	SaveMulAcc(5, 0, 6);
	MulAcc(1, 5);
	MulAcc(2, 4);
	MulAcc(3, 3);
	MulAcc(4, 2);
	MulAcc(5, 1);
	MulAcc(6, 0);

	R[6] = c;
	R[7] = d + A[0] * B[7] + A[1] * B[6] + A[2] * B[5] + A[3] * B[4] +
				A[4] * B[3] + A[5] * B[2] + A[6] * B[1] + A[7] * B[0];
}

#undef MulAcc
#undef SaveMulAcc
#undef SquAcc
#undef SaveSquAcc

// CodeWarrior defines _MSC_VER
#if defined(_MSC_VER) && !defined(__MWERKS__) && defined(_M_IX86) && (_M_IX86<=700)

class PentiumOptimized : public Portable
{
public:
	static word __fastcall Add(word *C, const word *A, const word *B, unsigned int N);
	static word __fastcall Subtract(word *C, const word *A, const word *B, unsigned int N);
	static inline void Square4(word *R, const word *A)
	{
		// VC60 workaround: MSVC 6.0 has an optimization bug that makes
		// (dword)A*B where either A or B has been cast to a dword before
		// very expensive. Revisit this function when this
		// bug is fixed.
		Multiply4(R, A, A);
	}
};

typedef PentiumOptimized LowLevel;

__declspec(naked) word __fastcall PentiumOptimized::Add(word *C, const word *A, const word *B, unsigned int N)
{
	__asm
	{
		push ebp
		push ebx
		push esi
		push edi

		mov esi, [esp+24]	; N
		mov ebx, [esp+20]	; B

		// now: ebx = B, ecx = C, edx = A, esi = N

		sub ecx, edx	// hold the distance between C & A so we can add this to A to get C
		xor eax, eax	// clear eax

		sub eax, esi	// eax is a negative index from end of B
		lea ebx, [ebx+4*esi]	// ebx is end of B

		sar eax, 1		// unit of eax is now dwords; this also clears the carry flag
		jz	loopend		// if no dwords then nothing to do

loopstart:
		mov    esi,[edx]			// load lower word of A
		mov    ebp,[edx+4]			// load higher word of A

		mov    edi,[ebx+8*eax]		// load lower word of B
		lea    edx,[edx+8]			// advance A and C

		adc    esi,edi				// add lower words
		mov    edi,[ebx+8*eax+4]	// load higher word of B

		adc    ebp,edi				// add higher words
		inc    eax					// advance B

		mov    [edx+ecx-8],esi		// store lower word result
		mov    [edx+ecx-4],ebp		// store higher word result

		jnz    loopstart			// loop until eax overflows and becomes zero

loopend:
		adc eax, 0		// store carry into eax (return result register)
		pop edi
		pop esi
		pop ebx
		pop ebp
		ret 8
	}
}

__declspec(naked) word __fastcall PentiumOptimized::Subtract(word *C, const word *A, const word *B, unsigned int N)
{
	__asm
	{
		push ebp
		push ebx
		push esi
		push edi

		mov esi, [esp+24]	; N
		mov ebx, [esp+20]	; B

		sub ecx, edx
		xor eax, eax

		sub eax, esi
		lea ebx, [ebx+4*esi]

		sar eax, 1
		jz	loopend

loopstart:
		mov    esi,[edx]
		mov    ebp,[edx+4]

		mov    edi,[ebx+8*eax]
		lea    edx,[edx+8]

		sbb    esi,edi
		mov    edi,[ebx+8*eax+4]

		sbb    ebp,edi
		inc    eax

		mov    [edx+ecx-8],esi
		mov    [edx+ecx-4],ebp

		jnz    loopstart

loopend:
		adc eax, 0
		pop edi
		pop esi
		pop ebx
		pop ebp
		ret 8
	}
}

#ifdef SSE2_INTRINSICS_AVAILABLE

static bool GetSSE2Capability()
{
	word32 b;

	__asm
	{
		mov		eax, 1
		cpuid
		mov		b, edx
	}

	return (b & (1 << 26)) != 0;
}

bool g_sse2DetectionDone = false, g_sse2Detected, g_sse2Enabled = true;

static inline bool HasSSE2()
{
	if (g_sse2Enabled && !g_sse2DetectionDone)
	{
		g_sse2Detected = GetSSE2Capability();
		g_sse2DetectionDone = true;
	}
	return g_sse2Enabled && g_sse2Detected;
}

class P4Optimized : public PentiumOptimized
{
public:
	static word __fastcall Add(word *C, const word *A, const word *B, unsigned int N);
	static word __fastcall Subtract(word *C, const word *A, const word *B, unsigned int N);
	static void Multiply4(word *C, const word *A, const word *B);
	static void Multiply8(word *C, const word *A, const word *B);
	static inline void Square4(word *R, const word *A)
	{
		Multiply4(R, A, A);
	}
	static void Multiply8Bottom(word *C, const word *A, const word *B);
};

static void __fastcall P4_Mul(__m128i *C, const __m128i *A, const __m128i *B)
{
	__m128i a3210 = _mm_load_si128(A);
	__m128i b3210 = _mm_load_si128(B);

	__m128i sum;

	__m128i z = _mm_setzero_si128();
	__m128i a2b2_a0b0 = _mm_mul_epu32(a3210, b3210);
	C[0] = a2b2_a0b0;

	__m128i a3120 = _mm_shuffle_epi32(a3210, _MM_SHUFFLE(3, 1, 2, 0));
	__m128i b3021 = _mm_shuffle_epi32(b3210, _MM_SHUFFLE(3, 0, 2, 1));
	__m128i a1b0_a0b1 = _mm_mul_epu32(a3120, b3021);
	__m128i a1b0 = _mm_unpackhi_epi32(a1b0_a0b1, z);
	__m128i a0b1 = _mm_unpacklo_epi32(a1b0_a0b1, z);
	C[1] = _mm_add_epi64(a1b0, a0b1);

	__m128i a31 = _mm_srli_epi64(a3210, 32);
	__m128i b31 = _mm_srli_epi64(b3210, 32);
	__m128i a3b3_a1b1 = _mm_mul_epu32(a31, b31);
	C[6] = a3b3_a1b1;

	__m128i a1b1 = _mm_unpacklo_epi32(a3b3_a1b1, z);
	__m128i b3012 = _mm_shuffle_epi32(b3210, _MM_SHUFFLE(3, 0, 1, 2));
	__m128i a2b0_a0b2 = _mm_mul_epu32(a3210, b3012);
	__m128i a0b2 = _mm_unpacklo_epi32(a2b0_a0b2, z);
	__m128i a2b0 = _mm_unpackhi_epi32(a2b0_a0b2, z);
	sum = _mm_add_epi64(a1b1, a0b2);
	C[2] = _mm_add_epi64(sum, a2b0);

	__m128i a2301 = _mm_shuffle_epi32(a3210, _MM_SHUFFLE(2, 3, 0, 1));
	__m128i b2103 = _mm_shuffle_epi32(b3210, _MM_SHUFFLE(2, 1, 0, 3));
	__m128i a3b0_a1b2 = _mm_mul_epu32(a2301, b3012);
	__m128i a2b1_a0b3 = _mm_mul_epu32(a3210, b2103);
	__m128i a3b0 = _mm_unpackhi_epi32(a3b0_a1b2, z);
	__m128i a1b2 = _mm_unpacklo_epi32(a3b0_a1b2, z);
	__m128i a2b1 = _mm_unpackhi_epi32(a2b1_a0b3, z);
	__m128i a0b3 = _mm_unpacklo_epi32(a2b1_a0b3, z);
	__m128i sum1 = _mm_add_epi64(a3b0, a1b2);
	sum = _mm_add_epi64(a2b1, a0b3);
	C[3] = _mm_add_epi64(sum, sum1);

	__m128i	a3b1_a1b3 = _mm_mul_epu32(a2301, b2103);
	__m128i a2b2 = _mm_unpackhi_epi32(a2b2_a0b0, z);
	__m128i a3b1 = _mm_unpackhi_epi32(a3b1_a1b3, z);
	__m128i a1b3 = _mm_unpacklo_epi32(a3b1_a1b3, z);
	sum = _mm_add_epi64(a2b2, a3b1);
	C[4] = _mm_add_epi64(sum, a1b3);

	__m128i a1302 = _mm_shuffle_epi32(a3210, _MM_SHUFFLE(1, 3, 0, 2));
	__m128i b1203 = _mm_shuffle_epi32(b3210, _MM_SHUFFLE(1, 2, 0, 3));
	__m128i a3b2_a2b3 = _mm_mul_epu32(a1302, b1203);
	__m128i a3b2 = _mm_unpackhi_epi32(a3b2_a2b3, z);
	__m128i a2b3 = _mm_unpacklo_epi32(a3b2_a2b3, z);
	C[5] = _mm_add_epi64(a3b2, a2b3);
}

void P4Optimized::Multiply4(word *C, const word *A, const word *B)
{
	__m128i temp[7];
	const word *w = (word *)temp;
	const __m64 *mw = (__m64 *)w;

	P4_Mul(temp, (__m128i *)A, (__m128i *)B);

	C[0] = w[0];

	__m64 s1, s2;

	__m64 w1 = _m_from_int(w[1]);
	__m64 w4 = mw[2];
	__m64 w6 = mw[3];
	__m64 w8 = mw[4];
	__m64 w10 = mw[5];
	__m64 w12 = mw[6];
	__m64 w14 = mw[7];
	__m64 w16 = mw[8];
	__m64 w18 = mw[9];
	__m64 w20 = mw[10];
	__m64 w22 = mw[11];
	__m64 w26 = _m_from_int(w[26]);

	s1 = _mm_add_si64(w1, w4);
	C[1] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s2 = _mm_add_si64(w6, w8);
	s1 = _mm_add_si64(s1, s2);
	C[2] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s2 = _mm_add_si64(w10, w12);
	s1 = _mm_add_si64(s1, s2);
	C[3] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s2 = _mm_add_si64(w14, w16);
	s1 = _mm_add_si64(s1, s2);
	C[4] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s2 = _mm_add_si64(w18, w20);
	s1 = _mm_add_si64(s1, s2);
	C[5] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s2 = _mm_add_si64(w22, w26);
	s1 = _mm_add_si64(s1, s2);
	C[6] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	C[7] = _m_to_int(s1) + w[27];
	_mm_empty();
}

void P4Optimized::Multiply8(word *C, const word *A, const word *B)
{
	__m128i temp[28];
	const word *w = (word *)temp;
	const __m64 *mw = (__m64 *)w;
	const word *x = (word *)temp+7*4;
	const __m64 *mx = (__m64 *)x;
	const word *y = (word *)temp+7*4*2;
	const __m64 *my = (__m64 *)y;
	const word *z = (word *)temp+7*4*3;
	const __m64 *mz = (__m64 *)z;

	P4_Mul(temp, (__m128i *)A, (__m128i *)B);

	P4_Mul(temp+7, (__m128i *)A+1, (__m128i *)B);

	P4_Mul(temp+14, (__m128i *)A, (__m128i *)B+1);

	P4_Mul(temp+21, (__m128i *)A+1, (__m128i *)B+1);

	C[0] = w[0];

	__m64 s1, s2, s3, s4;

	__m64 w1 = _m_from_int(w[1]);
	__m64 w4 = mw[2];
	__m64 w6 = mw[3];
	__m64 w8 = mw[4];
	__m64 w10 = mw[5];
	__m64 w12 = mw[6];
	__m64 w14 = mw[7];
	__m64 w16 = mw[8];
	__m64 w18 = mw[9];
	__m64 w20 = mw[10];
	__m64 w22 = mw[11];
	__m64 w26 = _m_from_int(w[26]);
	__m64 w27 = _m_from_int(w[27]);

	__m64 x0 = _m_from_int(x[0]);
	__m64 x1 = _m_from_int(x[1]);
	__m64 x4 = mx[2];
	__m64 x6 = mx[3];
	__m64 x8 = mx[4];
	__m64 x10 = mx[5];
	__m64 x12 = mx[6];
	__m64 x14 = mx[7];
	__m64 x16 = mx[8];
	__m64 x18 = mx[9];
	__m64 x20 = mx[10];
	__m64 x22 = mx[11];
	__m64 x26 = _m_from_int(x[26]);
	__m64 x27 = _m_from_int(x[27]);

	__m64 y0 = _m_from_int(y[0]);
	__m64 y1 = _m_from_int(y[1]);
	__m64 y4 = my[2];
	__m64 y6 = my[3];
	__m64 y8 = my[4];
	__m64 y10 = my[5];
	__m64 y12 = my[6];
	__m64 y14 = my[7];
	__m64 y16 = my[8];
	__m64 y18 = my[9];
	__m64 y20 = my[10];
	__m64 y22 = my[11];
	__m64 y26 = _m_from_int(y[26]);
	__m64 y27 = _m_from_int(y[27]);

	__m64 z0 = _m_from_int(z[0]);
	__m64 z1 = _m_from_int(z[1]);
	__m64 z4 = mz[2];
	__m64 z6 = mz[3];
	__m64 z8 = mz[4];
	__m64 z10 = mz[5];
	__m64 z12 = mz[6];
	__m64 z14 = mz[7];
	__m64 z16 = mz[8];
	__m64 z18 = mz[9];
	__m64 z20 = mz[10];
	__m64 z22 = mz[11];
	__m64 z26 = _m_from_int(z[26]);

	s1 = _mm_add_si64(w1, w4);
	C[1] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s2 = _mm_add_si64(w6, w8);
	s1 = _mm_add_si64(s1, s2);
	C[2] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s2 = _mm_add_si64(w10, w12);
	s1 = _mm_add_si64(s1, s2);
	C[3] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(x0, y0);
	s2 = _mm_add_si64(w14, w16);
	s1 = _mm_add_si64(s1, s3);
	s1 = _mm_add_si64(s1, s2);
	C[4] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(x1, y1);
	s4 = _mm_add_si64(x4, y4);
	s1 = _mm_add_si64(s1, w18);
	s3 = _mm_add_si64(s3, s4);
	s1 = _mm_add_si64(s1, w20);
	s1 = _mm_add_si64(s1, s3);
	C[5] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(x6, y6);
	s4 = _mm_add_si64(x8, y8);
	s1 = _mm_add_si64(s1, w22);
	s3 = _mm_add_si64(s3, s4);
	s1 = _mm_add_si64(s1, w26);
	s1 = _mm_add_si64(s1, s3);
	C[6] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(x10, y10);
	s4 = _mm_add_si64(x12, y12);
	s1 = _mm_add_si64(s1, w27);
	s3 = _mm_add_si64(s3, s4);
	s1 = _mm_add_si64(s1, s3);
	C[7] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(x14, y14);
	s4 = _mm_add_si64(x16, y16);
	s1 = _mm_add_si64(s1, z0);
	s3 = _mm_add_si64(s3, s4);
	s1 = _mm_add_si64(s1, s3);
	C[8] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(x18, y18);
	s4 = _mm_add_si64(x20, y20);
	s1 = _mm_add_si64(s1, z1);
	s3 = _mm_add_si64(s3, s4);
	s1 = _mm_add_si64(s1, z4);
	s1 = _mm_add_si64(s1, s3);
	C[9] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(x22, y22);
	s4 = _mm_add_si64(x26, y26);
	s1 = _mm_add_si64(s1, z6);
	s3 = _mm_add_si64(s3, s4);
	s1 = _mm_add_si64(s1, z8);
	s1 = _mm_add_si64(s1, s3);
	C[10] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(x27, y27);
	s1 = _mm_add_si64(s1, z10);
	s1 = _mm_add_si64(s1, z12);
	s1 = _mm_add_si64(s1, s3);
	C[11] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(z14, z16);
	s1 = _mm_add_si64(s1, s3);
	C[12] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(z18, z20);
	s1 = _mm_add_si64(s1, s3);
	C[13] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(z22, z26);
	s1 = _mm_add_si64(s1, s3);
	C[14] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	C[15] = z[27] + _m_to_int(s1);
	_mm_empty();
}

void P4Optimized::Multiply8Bottom(word *C, const word *A, const word *B)
{
	__m128i temp[21];
	const word *w = (word *)temp;
	const __m64 *mw = (__m64 *)w;
	const word *x = (word *)temp+7*4;
	const __m64 *mx = (__m64 *)x;
	const word *y = (word *)temp+7*4*2;
	const __m64 *my = (__m64 *)y;

	P4_Mul(temp, (__m128i *)A, (__m128i *)B);

	P4_Mul(temp+7, (__m128i *)A+1, (__m128i *)B);

	P4_Mul(temp+14, (__m128i *)A, (__m128i *)B+1);

	C[0] = w[0];

	__m64 s1, s2, s3, s4;

	__m64 w1 = _m_from_int(w[1]);
	__m64 w4 = mw[2];
	__m64 w6 = mw[3];
	__m64 w8 = mw[4];
	__m64 w10 = mw[5];
	__m64 w12 = mw[6];
	__m64 w14 = mw[7];
	__m64 w16 = mw[8];
	__m64 w18 = mw[9];
	__m64 w20 = mw[10];
	__m64 w22 = mw[11];
	__m64 w26 = _m_from_int(w[26]);

	__m64 x0 = _m_from_int(x[0]);
	__m64 x1 = _m_from_int(x[1]);
	__m64 x4 = mx[2];
	__m64 x6 = mx[3];
	__m64 x8 = mx[4];

	__m64 y0 = _m_from_int(y[0]);
	__m64 y1 = _m_from_int(y[1]);
	__m64 y4 = my[2];
	__m64 y6 = my[3];
	__m64 y8 = my[4];

	s1 = _mm_add_si64(w1, w4);
	C[1] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s2 = _mm_add_si64(w6, w8);
	s1 = _mm_add_si64(s1, s2);
	C[2] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s2 = _mm_add_si64(w10, w12);
	s1 = _mm_add_si64(s1, s2);
	C[3] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(x0, y0);
	s2 = _mm_add_si64(w14, w16);
	s1 = _mm_add_si64(s1, s3);
	s1 = _mm_add_si64(s1, s2);
	C[4] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(x1, y1);
	s4 = _mm_add_si64(x4, y4);
	s1 = _mm_add_si64(s1, w18);
	s3 = _mm_add_si64(s3, s4);
	s1 = _mm_add_si64(s1, w20);
	s1 = _mm_add_si64(s1, s3);
	C[5] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	s3 = _mm_add_si64(x6, y6);
	s4 = _mm_add_si64(x8, y8);
	s1 = _mm_add_si64(s1, w22);
	s3 = _mm_add_si64(s3, s4);
	s1 = _mm_add_si64(s1, w26);
	s1 = _mm_add_si64(s1, s3);
	C[6] = _m_to_int(s1);
	s1 = _m_psrlqi(s1, 32);

	C[7] = _m_to_int(s1) + w[27] + x[10] + y[10] + x[12] + y[12];
	_mm_empty();
}

__declspec(naked) word __fastcall P4Optimized::Add(word *C, const word *A, const word *B, unsigned int N)
{
	__asm
	{
		sub		esp, 16
		xor		eax, eax
		mov		[esp], edi
		mov		[esp+4], esi
		mov		[esp+8], ebx
		mov		[esp+12], ebp

		mov		ebx, [esp+20]	// B
		mov		esi, [esp+24]	// N

		// now: ebx = B, ecx = C, edx = A, esi = N

		neg		esi
		jz		loopend		// if no dwords then nothing to do

		mov		edi, [edx]
		mov		ebp, [ebx]

loopstart:
		add		edi, eax
		jc		carry1

		xor		eax, eax

carry1continue:
		add		edi, ebp
		mov		ebp, 1
		mov		[ecx], edi
		mov		edi, [edx+4]
		cmovc	eax, ebp
		mov		ebp, [ebx+4]
		lea		ebx, [ebx+8]
		add		edi, eax
		jc		carry2

		xor		eax, eax

carry2continue:
		add		edi, ebp
		mov		ebp, 1
		cmovc	eax, ebp
		mov		[ecx+4], edi
		add		ecx, 8
		mov		edi, [edx+8]
		add		edx, 8
		add		esi, 2
		mov		ebp, [ebx]
		jnz		loopstart

loopend:
		mov		edi, [esp]
		mov		esi, [esp+4]
		mov		ebx, [esp+8]
		mov		ebp, [esp+12]
		add		esp, 16
		ret		8

carry1:
		mov		eax, 1
		jmp		carry1continue

carry2:
		mov		eax, 1
		jmp		carry2continue
	}
}

__declspec(naked) word __fastcall P4Optimized::Subtract(word *C, const word *A, const word *B, unsigned int N)
{
	__asm
	{
		sub		esp, 16
		xor		eax, eax
		mov		[esp], edi
		mov		[esp+4], esi
		mov		[esp+8], ebx
		mov		[esp+12], ebp

		mov		ebx, [esp+20]	// B
		mov		esi, [esp+24]	// N

		// now: ebx = B, ecx = C, edx = A, esi = N

		neg		esi
		jz		loopend		// if no dwords then nothing to do

		mov		edi, [edx]
		mov		ebp, [ebx]

loopstart:
		sub		edi, eax
		jc		carry1

		xor		eax, eax

carry1continue:
		sub		edi, ebp
		mov		ebp, 1
		mov		[ecx], edi
		mov		edi, [edx+4]
		cmovc	eax, ebp
		mov		ebp, [ebx+4]
		lea		ebx, [ebx+8]
		sub		edi, eax
		jc		carry2

		xor		eax, eax

carry2continue:
		sub		edi, ebp
		mov		ebp, 1
		cmovc	eax, ebp
		mov		[ecx+4], edi
		add		ecx, 8
		mov		edi, [edx+8]
		add		edx, 8
		add		esi, 2
		mov		ebp, [ebx]
		jnz		loopstart

loopend:
		mov		edi, [esp]
		mov		esi, [esp+4]
		mov		ebx, [esp+8]
		mov		ebp, [esp+12]
		add		esp, 16
		ret		8

carry1:
		mov		eax, 1
		jmp		carry1continue

carry2:
		mov		eax, 1
		jmp		carry2continue
	}
}

#endif	// #ifdef SSE2_INTRINSICS_AVAILABLE

#elif defined(__GNUC__) && defined(__i386__)

class PentiumOptimized : public Portable
{
public:
#ifndef __pic__		// -fpic uses up a register, leaving too few for the asm code
	static word Add(word *C, const word *A, const word *B, unsigned int N);
	static word Subtract(word *C, const word *A, const word *B, unsigned int N);
#endif
	static void Square4(word *R, const word *A);
	static void Multiply4(word *C, const word *A, const word *B);
	static void Multiply8(word *C, const word *A, const word *B);
};

typedef PentiumOptimized LowLevel;

// Add and Subtract assembly code originally contributed by Alister Lee

#ifndef __pic__
__attribute__((regparm(3))) word PentiumOptimized::Add(word *C, const word *A, const word *B, unsigned int N)
{
	assert (N%2 == 0);

	register word carry, temp;

	__asm__ __volatile__(
			"push %%ebp;"
			"sub %3, %2;"
			"xor %0, %0;"
			"sub %4, %0;"
			"lea (%1,%4,4), %1;"
			"sar $1, %0;"
			"jz 1f;"

		"0:;"
			"mov 0(%3), %4;"
			"mov 4(%3), %%ebp;"
			"mov (%1,%0,8), %5;"
			"lea 8(%3), %3;"
			"adc %5, %4;"
			"mov 4(%1,%0,8), %5;"
			"adc %5, %%ebp;"
			"inc %0;"
			"mov %4, -8(%3, %2);"
			"mov %%ebp, -4(%3, %2);"
			"jnz 0b;"

		"1:;"
			"adc $0, %0;"
			"pop %%ebp;"

		: "=aSD" (carry), "+r" (B), "+r" (C), "+r" (A), "+r" (N), "=r" (temp)
		: : "cc", "memory");

	return carry;
}

__attribute__((regparm(3))) word PentiumOptimized::Subtract(word *C, const word *A, const word *B, unsigned int N)
{
	assert (N%2 == 0);

	register word carry, temp;

	__asm__ __volatile__(
			"push %%ebp;"
			"sub %3, %2;"
			"xor %0, %0;"
			"sub %4, %0;"
			"lea (%1,%4,4), %1;"
			"sar $1, %0;"
			"jz 1f;"

		"0:;"
			"mov 0(%3), %4;"
			"mov 4(%3), %%ebp;"
			"mov (%1,%0,8), %5;"
			"lea 8(%3), %3;"
			"sbb %5, %4;"
			"mov 4(%1,%0,8), %5;"
			"sbb %5, %%ebp;"
			"inc %0;"
			"mov %4, -8(%3, %2);"
			"mov %%ebp, -4(%3, %2);"
			"jnz 0b;"

		"1:;"
			"adc $0, %0;"
			"pop %%ebp;"

		: "=aSD" (carry), "+r" (B), "+r" (C), "+r" (A), "+r" (N), "=r" (temp)
		: : "cc", "memory");

	return carry;
}
#endif	// __pic__

// Comba square and multiply assembly code originally contributed by Leonard Janke

#define SqrStartup \
  "push %%ebp\n\t" \
  "push %%esi\n\t" \
  "push %%ebx\n\t" \
  "xor %%ebp, %%ebp\n\t" \
  "xor %%ebx, %%ebx\n\t" \
  "xor %%ecx, %%ecx\n\t" 

#define SqrShiftCarry \
  "mov %%ebx, %%ebp\n\t" \
  "mov %%ecx, %%ebx\n\t" \
  "xor %%ecx, %%ecx\n\t"

#define SqrAccumulate(i,j) \
  "mov 4*"#j"(%%esi), %%eax\n\t" \
  "mull 4*"#i"(%%esi)\n\t" \
  "add %%eax, %%ebp\n\t" \
  "adc %%edx, %%ebx\n\t" \
  "adc %%ch, %%cl\n\t" \
  "add %%eax, %%ebp\n\t" \
  "adc %%edx, %%ebx\n\t" \
  "adc %%ch, %%cl\n\t"

#define SqrAccumulateCentre(i) \
  "mov 4*"#i"(%%esi), %%eax\n\t" \
  "mull 4*"#i"(%%esi)\n\t" \
  "add %%eax, %%ebp\n\t" \
  "adc %%edx, %%ebx\n\t" \
  "adc %%ch, %%cl\n\t" 

#define SqrStoreDigit(X)  \
  "mov %%ebp, 4*"#X"(%%edi)\n\t" \

#define SqrLastDiagonal(digits) \
  "mov 4*("#digits"-1)(%%esi), %%eax\n\t" \
  "mull 4*("#digits"-1)(%%esi)\n\t" \
  "add %%eax, %%ebp\n\t" \
  "adc %%edx, %%ebx\n\t" \
  "mov %%ebp, 4*(2*"#digits"-2)(%%edi)\n\t" \
  "mov %%ebx, 4*(2*"#digits"-1)(%%edi)\n\t" 

#define SqrCleanup \
  "pop %%ebx\n\t" \
  "pop %%esi\n\t" \
  "pop %%ebp\n\t" 

void PentiumOptimized::Square4(word* Y, const word* X)
{
	__asm__ __volatile__(
		SqrStartup

		SqrAccumulateCentre(0)
		SqrStoreDigit(0)
		SqrShiftCarry

		SqrAccumulate(1,0)
		SqrStoreDigit(1)
		SqrShiftCarry

		SqrAccumulate(2,0)
		SqrAccumulateCentre(1)
		SqrStoreDigit(2)
		SqrShiftCarry

		SqrAccumulate(3,0)
		SqrAccumulate(2,1)
		SqrStoreDigit(3)
		SqrShiftCarry

		SqrAccumulate(3,1)
		SqrAccumulateCentre(2)
		SqrStoreDigit(4)
		SqrShiftCarry

		SqrAccumulate(3,2)
		SqrStoreDigit(5)
		SqrShiftCarry

		SqrLastDiagonal(4)

		SqrCleanup

		:
		: "D" (Y), "S" (X)
		: "eax",  "ecx", "edx", "ebp",   "memory"
	);
}

#define MulStartup \
  "push %%ebp\n\t" \
  "push %%esi\n\t" \
  "push %%ebx\n\t" \
  "push %%edi\n\t" \
  "mov %%eax, %%ebx \n\t" \
  "xor %%ebp, %%ebp\n\t" \
  "xor %%edi, %%edi\n\t" \
  "xor %%ecx, %%ecx\n\t" 

#define MulShiftCarry \
  "mov %%edx, %%ebp\n\t" \
  "mov %%ecx, %%edi\n\t" \
  "xor %%ecx, %%ecx\n\t"

#define MulAccumulate(i,j) \
  "mov 4*"#j"(%%ebx), %%eax\n\t" \
  "mull 4*"#i"(%%esi)\n\t" \
  "add %%eax, %%ebp\n\t" \
  "adc %%edx, %%edi\n\t" \
  "adc %%ch, %%cl\n\t"

#define MulStoreDigit(X)  \
  "mov %%edi, %%edx \n\t" \
  "mov (%%esp), %%edi \n\t" \
  "mov %%ebp, 4*"#X"(%%edi)\n\t" \
  "mov %%edi, (%%esp)\n\t" 

#define MulLastDiagonal(digits) \
  "mov 4*("#digits"-1)(%%ebx), %%eax\n\t" \
  "mull 4*("#digits"-1)(%%esi)\n\t" \
  "add %%eax, %%ebp\n\t" \
  "adc %%edi, %%edx\n\t" \
  "mov (%%esp), %%edi\n\t" \
  "mov %%ebp, 4*(2*"#digits"-2)(%%edi)\n\t" \
  "mov %%edx, 4*(2*"#digits"-1)(%%edi)\n\t" 

#define MulCleanup \
  "pop %%edi\n\t" \
  "pop %%ebx\n\t" \
  "pop %%esi\n\t" \
  "pop %%ebp\n\t" 

void PentiumOptimized::Multiply4(word* Z, const word* X, const word* Y)
{
	__asm__ __volatile__(
		MulStartup
		MulAccumulate(0,0)
		MulStoreDigit(0)
		MulShiftCarry

		MulAccumulate(1,0)
		MulAccumulate(0,1)
		MulStoreDigit(1)
		MulShiftCarry

		MulAccumulate(2,0)
		MulAccumulate(1,1)
		MulAccumulate(0,2)
		MulStoreDigit(2)
		MulShiftCarry

		MulAccumulate(3,0)
		MulAccumulate(2,1)
		MulAccumulate(1,2)
		MulAccumulate(0,3)
		MulStoreDigit(3)
		MulShiftCarry

		MulAccumulate(3,1)
		MulAccumulate(2,2)
		MulAccumulate(1,3)
		MulStoreDigit(4)
		MulShiftCarry

		MulAccumulate(3,2)
		MulAccumulate(2,3)
		MulStoreDigit(5)
		MulShiftCarry

		MulLastDiagonal(4)

		MulCleanup

		: 
		: "D" (Z), "S" (X), "a" (Y)
		: "%ecx", "%edx",  "memory"
	);
}

void PentiumOptimized::Multiply8(word* Z, const word* X, const word* Y)
{
	__asm__ __volatile__(
		MulStartup
		MulAccumulate(0,0)
		MulStoreDigit(0)
		MulShiftCarry

		MulAccumulate(1,0)
		MulAccumulate(0,1)
		MulStoreDigit(1)
		MulShiftCarry

		MulAccumulate(2,0)
		MulAccumulate(1,1)
		MulAccumulate(0,2)
		MulStoreDigit(2)
		MulShiftCarry

		MulAccumulate(3,0)
		MulAccumulate(2,1)
		MulAccumulate(1,2)
		MulAccumulate(0,3)
		MulStoreDigit(3)
		MulShiftCarry

		MulAccumulate(4,0)
		MulAccumulate(3,1)
		MulAccumulate(2,2)
		MulAccumulate(1,3)
		MulAccumulate(0,4)
		MulStoreDigit(4)
		MulShiftCarry

		MulAccumulate(5,0)
		MulAccumulate(4,1)
		MulAccumulate(3,2)
		MulAccumulate(2,3)
		MulAccumulate(1,4)
		MulAccumulate(0,5)
		MulStoreDigit(5)
		MulShiftCarry

		MulAccumulate(6,0)
		MulAccumulate(5,1)
		MulAccumulate(4,2)
		MulAccumulate(3,3)
		MulAccumulate(2,4)
		MulAccumulate(1,5)
		MulAccumulate(0,6)
		MulStoreDigit(6)
		MulShiftCarry

		MulAccumulate(7,0)
		MulAccumulate(6,1)
		MulAccumulate(5,2)
		MulAccumulate(4,3)
		MulAccumulate(3,4)
		MulAccumulate(2,5)
		MulAccumulate(1,6)
		MulAccumulate(0,7)
		MulStoreDigit(7)
		MulShiftCarry

		MulAccumulate(7,1)
		MulAccumulate(6,2)
		MulAccumulate(5,3)
		MulAccumulate(4,4)
		MulAccumulate(3,5)
		MulAccumulate(2,6)
		MulAccumulate(1,7)
		MulStoreDigit(8)
		MulShiftCarry

		MulAccumulate(7,2)
		MulAccumulate(6,3)
		MulAccumulate(5,4)
		MulAccumulate(4,5)
		MulAccumulate(3,6)
		MulAccumulate(2,7)
		MulStoreDigit(9)
		MulShiftCarry

		MulAccumulate(7,3)
		MulAccumulate(6,4)
		MulAccumulate(5,5)
		MulAccumulate(4,6)
		MulAccumulate(3,7)
		MulStoreDigit(10)
		MulShiftCarry

		MulAccumulate(7,4)
		MulAccumulate(6,5)
		MulAccumulate(5,6)
		MulAccumulate(4,7)
		MulStoreDigit(11)
		MulShiftCarry

		MulAccumulate(7,5)
		MulAccumulate(6,6)
		MulAccumulate(5,7)
		MulStoreDigit(12)
		MulShiftCarry

		MulAccumulate(7,6)
		MulAccumulate(6,7)
		MulStoreDigit(13)
		MulShiftCarry

		MulLastDiagonal(8)

		MulCleanup

		: 
		: "D" (Z), "S" (X), "a" (Y)
		: "%ecx", "%edx",  "memory"
	);
}

#elif defined(__GNUC__) && defined(__alpha__)

class AlphaOptimized : public Portable
{
public:
	static inline void Multiply2(word *C, const word *A, const word *B);
	static inline word Multiply2Add(word *C, const word *A, const word *B);
	static inline void Multiply4(word *C, const word *A, const word *B);
	static inline unsigned int MultiplyRecursionLimit() {return 4;}

	static inline void Multiply4Bottom(word *C, const word *A, const word *B);
	static inline unsigned int MultiplyBottomRecursionLimit() {return 4;}

	static inline void Square4(word *R, const word *A)
	{
		Multiply4(R, A, A);
	}
};

typedef AlphaOptimized LowLevel;

inline void AlphaOptimized::Multiply2(word *C, const word *A, const word *B)
{
	register dword c, a = *(const dword *)A, b = *(const dword *)B;
	((dword *)C)[0] = a*b;
	__asm__("umulh %1,%2,%0" : "=r" (c) : "r" (a), "r" (b));
	((dword *)C)[1] = c;
}

inline word AlphaOptimized::Multiply2Add(word *C, const word *A, const word *B)
{
	register dword c, d, e, a = *(const dword *)A, b = *(const dword *)B;
	c = ((dword *)C)[0];
	d = a*b + c;
	__asm__("umulh %1,%2,%0" : "=r" (e) : "r" (a), "r" (b));
	((dword *)C)[0] = d;
	d = (d < c);
	c = ((dword *)C)[1] + d;
	d = (c < d);
	c += e;
	((dword *)C)[1] = c;
	d |= (c < e);
	return d;
}

inline void AlphaOptimized::Multiply4(word *R, const word *A, const word *B)
{
	Multiply2(R, A, B);
	Multiply2(R+4, A+2, B+2);
	word carry = Multiply2Add(R+2, A+0, B+2);
	carry += Multiply2Add(R+2, A+2, B+0);
	Increment(R+6, 2, carry);
}

static inline void Multiply2BottomAdd(word *C, const word *A, const word *B)
{
	register dword a = *(const dword *)A, b = *(const dword *)B;
	((dword *)C)[0] = a*b + ((dword *)C)[0];
}

inline void AlphaOptimized::Multiply4Bottom(word *R, const word *A, const word *B)
{
	Multiply2(R, A, B);
	Multiply2BottomAdd(R+2, A+0, B+2);
	Multiply2BottomAdd(R+2, A+2, B+0);
}

#else	// no processor specific code available

typedef Portable LowLevel;

#endif

// ********************************************************

#define A0		A
#define A1		(A+N2)
#define B0		B
#define B1		(B+N2)

#define T0		T
#define T1		(T+N2)
#define T2		(T+N)
#define T3		(T+N+N2)

#define R0		R
#define R1		(R+N2)
#define R2		(R+N)
#define R3		(R+N+N2)

//VC60 workaround: compiler bug triggered without the extra dummy parameters

// R[2*N] - result = A*B
// T[2*N] - temporary work space
// A[N] --- multiplier
// B[N] --- multiplicant

template <class P>
void DoRecursiveMultiply(word *R, word *T, const word *A, const word *B, unsigned int N, const P *dummy=NULL);

template <class P>
inline void RecursiveMultiply(word *R, word *T, const word *A, const word *B, unsigned int N, const P *dummy=NULL)
{
	assert(N>=2 && N%2==0);

	if (P::MultiplyRecursionLimit() >= 8 && N==8)
		P::Multiply8(R, A, B);
	else if (P::MultiplyRecursionLimit() >= 4 && N==4)
		P::Multiply4(R, A, B);
	else if (N==2)
		P::Multiply2(R, A, B);
	else
		DoRecursiveMultiply<P>(R, T, A, B, N, NULL);	// VC60 workaround: needs this NULL
}

template <class P>
void DoRecursiveMultiply(word *R, word *T, const word *A, const word *B, unsigned int N, const P *dummy)
{
	const unsigned int N2 = N/2;
	int carry;

	int aComp = Compare(A0, A1, N2);
	int bComp = Compare(B0, B1, N2);

	switch (2*aComp + aComp + bComp)
	{
	case -4:
		P::Subtract(R0, A1, A0, N2);
		P::Subtract(R1, B0, B1, N2);
		RecursiveMultiply<P>(T0, T2, R0, R1, N2);
		P::Subtract(T1, T1, R0, N2);
		carry = -1;
		break;
	case -2:
		P::Subtract(R0, A1, A0, N2);
		P::Subtract(R1, B0, B1, N2);
		RecursiveMultiply<P>(T0, T2, R0, R1, N2);
		carry = 0;
		break;
	case 2:
		P::Subtract(R0, A0, A1, N2);
		P::Subtract(R1, B1, B0, N2);
		RecursiveMultiply<P>(T0, T2, R0, R1, N2);
		carry = 0;
		break;
	case 4:
		P::Subtract(R0, A1, A0, N2);
		P::Subtract(R1, B0, B1, N2);
		RecursiveMultiply<P>(T0, T2, R0, R1, N2);
		P::Subtract(T1, T1, R1, N2);
		carry = -1;
		break;
	default:
		SetWords(T0, 0, N);
		carry = 0;
	}

	RecursiveMultiply<P>(R0, T2, A0, B0, N2);
	RecursiveMultiply<P>(R2, T2, A1, B1, N2);

	// now T[01] holds (A1-A0)*(B0-B1), R[01] holds A0*B0, R[23] holds A1*B1

	carry += P::Add(T0, T0, R0, N);
	carry += P::Add(T0, T0, R2, N);
	carry += P::Add(R1, R1, T0, N);

	assert (carry >= 0 && carry <= 2);
	Increment(R3, N2, carry);
}

// R[2*N] - result = A*A
// T[2*N] - temporary work space
// A[N] --- number to be squared

template <class P>
void DoRecursiveSquare(word *R, word *T, const word *A, unsigned int N, const P *dummy=NULL);

template <class P>
inline void RecursiveSquare(word *R, word *T, const word *A, unsigned int N, const P *dummy=NULL)
{
	assert(N && N%2==0);
	if (P::SquareRecursionLimit() >= 8 && N==8)
		P::Square8(R, A);
	if (P::SquareRecursionLimit() >= 4 && N==4)
		P::Square4(R, A);
	else if (N==2)
		P::Square2(R, A);
	else
		DoRecursiveSquare<P>(R, T, A, N, NULL);	// VC60 workaround: needs this NULL
}

template <class P>
void DoRecursiveSquare(word *R, word *T, const word *A, unsigned int N, const P *dummy)
{
	const unsigned int N2 = N/2;

	RecursiveSquare<P>(R0, T2, A0, N2);
	RecursiveSquare<P>(R2, T2, A1, N2);
	RecursiveMultiply<P>(T0, T2, A0, A1, N2);

	word carry = P::Add(R1, R1, T0, N);
	carry += P::Add(R1, R1, T0, N);
	Increment(R3, N2, carry);
}

// R[N] - bottom half of A*B
// T[N] - temporary work space
// A[N] - multiplier
// B[N] - multiplicant

template <class P>
void DoRecursiveMultiplyBottom(word *R, word *T, const word *A, const word *B, unsigned int N, const P *dummy=NULL);

template <class P>
inline void RecursiveMultiplyBottom(word *R, word *T, const word *A, const word *B, unsigned int N, const P *dummy=NULL)
{
	assert(N>=2 && N%2==0);
	if (P::MultiplyBottomRecursionLimit() >= 8 && N==8)
		P::Multiply8Bottom(R, A, B);
	else if (P::MultiplyBottomRecursionLimit() >= 4 && N==4)
		P::Multiply4Bottom(R, A, B);
	else if (N==2)
		P::Multiply2Bottom(R, A, B);
	else
		DoRecursiveMultiplyBottom<P>(R, T, A, B, N, NULL);
}

template <class P>
void DoRecursiveMultiplyBottom(word *R, word *T, const word *A, const word *B, unsigned int N, const P *dummy)
{
	const unsigned int N2 = N/2;

	RecursiveMultiply<P>(R, T, A0, B0, N2);
	RecursiveMultiplyBottom<P>(T0, T1, A1, B0, N2);
	P::Add(R1, R1, T0, N2);
	RecursiveMultiplyBottom<P>(T0, T1, A0, B1, N2);
	P::Add(R1, R1, T0, N2);
}

// R[N] --- upper half of A*B
// T[2*N] - temporary work space
// L[N] --- lower half of A*B
// A[N] --- multiplier
// B[N] --- multiplicant

template <class P>
void RecursiveMultiplyTop(word *R, word *T, const word *L, const word *A, const word *B, unsigned int N, const P *dummy=NULL)
{
	assert(N>=2 && N%2==0);

	if (N==4)
	{
		P::Multiply4(T, A, B);
		((dword *)R)[0] = ((dword *)T)[2];
		((dword *)R)[1] = ((dword *)T)[3];
	}
	else if (N==2)
	{
		P::Multiply2(T, A, B);
		((dword *)R)[0] = ((dword *)T)[1];
	}
	else
	{
		const unsigned int N2 = N/2;
		int carry;

		int aComp = Compare(A0, A1, N2);
		int bComp = Compare(B0, B1, N2);

		switch (2*aComp + aComp + bComp)
		{
		case -4:
			P::Subtract(R0, A1, A0, N2);
			P::Subtract(R1, B0, B1, N2);
			RecursiveMultiply<P>(T0, T2, R0, R1, N2);
			P::Subtract(T1, T1, R0, N2);
			carry = -1;
			break;
		case -2:
			P::Subtract(R0, A1, A0, N2);
			P::Subtract(R1, B0, B1, N2);
			RecursiveMultiply<P>(T0, T2, R0, R1, N2);
			carry = 0;
			break;
		case 2:
			P::Subtract(R0, A0, A1, N2);
			P::Subtract(R1, B1, B0, N2);
			RecursiveMultiply<P>(T0, T2, R0, R1, N2);
			carry = 0;
			break;
		case 4:
			P::Subtract(R0, A1, A0, N2);
			P::Subtract(R1, B0, B1, N2);
			RecursiveMultiply<P>(T0, T2, R0, R1, N2);
			P::Subtract(T1, T1, R1, N2);
			carry = -1;
			break;
		default:
			SetWords(T0, 0, N);
			carry = 0;
		}

		RecursiveMultiply<P>(T2, R0, A1, B1, N2);

		// now T[01] holds (A1-A0)*(B0-B1), T[23] holds A1*B1

		word c2 = P::Subtract(R0, L+N2, L, N2);
		c2 += P::Subtract(R0, R0, T0, N2);
		word t = (Compare(R0, T2, N2) == -1);

		carry += t;
		carry += Increment(R0, N2, c2+t);
		carry += P::Add(R0, R0, T1, N2);
		carry += P::Add(R0, R0, T3, N2);
		assert (carry >= 0 && carry <= 2);

		CopyWords(R1, T3, N2);
		Increment(R1, N2, carry);
	}
}

inline word Add(word *C, const word *A, const word *B, unsigned int N)
{
	return LowLevel::Add(C, A, B, N);
}

inline word Subtract(word *C, const word *A, const word *B, unsigned int N)
{
	return LowLevel::Subtract(C, A, B, N);
}

inline void Multiply(word *R, word *T, const word *A, const word *B, unsigned int N)
{
#ifdef SSE2_INTRINSICS_AVAILABLE
	if (HasSSE2())
		RecursiveMultiply<P4Optimized>(R, T, A, B, N);
	else
#endif
		RecursiveMultiply<LowLevel>(R, T, A, B, N);
}

inline void Square(word *R, word *T, const word *A, unsigned int N)
{
#ifdef SSE2_INTRINSICS_AVAILABLE
	if (HasSSE2())
		RecursiveSquare<P4Optimized>(R, T, A, N);
	else
#endif
		RecursiveSquare<LowLevel>(R, T, A, N);
}

inline void MultiplyBottom(word *R, word *T, const word *A, const word *B, unsigned int N)
{
#ifdef SSE2_INTRINSICS_AVAILABLE
	if (HasSSE2())
		RecursiveMultiplyBottom<P4Optimized>(R, T, A, B, N);
	else
#endif
		RecursiveMultiplyBottom<LowLevel>(R, T, A, B, N);
}

inline void MultiplyTop(word *R, word *T, const word *L, const word *A, const word *B, unsigned int N)
{
#ifdef SSE2_INTRINSICS_AVAILABLE
	if (HasSSE2())
		RecursiveMultiplyTop<P4Optimized>(R, T, L, A, B, N);
	else
#endif
		RecursiveMultiplyTop<LowLevel>(R, T, L, A, B, N);
}

// R[NA+NB] - result = A*B
// T[NA+NB] - temporary work space
// A[NA] ---- multiplier
// B[NB] ---- multiplicant

void AsymmetricMultiply(word *R, word *T, const word *A, unsigned int NA, const word *B, unsigned int NB)
{
	if (NA == NB)
	{
		if (A == B)
			Square(R, T, A, NA);
		else
			Multiply(R, T, A, B, NA);

		return;
	}

	if (NA > NB)
	{
		std::swap(A, B);
		std::swap(NA, NB);
	}

	assert(NB % NA == 0);
	assert((NB/NA)%2 == 0); 	// NB is an even multiple of NA

	if (NA==2 && !A[1])
	{
		switch (A[0])
		{
		case 0:
			SetWords(R, 0, NB+2);
			return;
		case 1:
			CopyWords(R, B, NB);
			R[NB] = R[NB+1] = 0;
			return;
		default:
			R[NB] = LinearMultiply(R, B, A[0], NB);
			R[NB+1] = 0;
			return;
		}
	}

	Multiply(R, T, A, B, NA);
	CopyWords(T+2*NA, R+NA, NA);

	unsigned i;

	for (i=2*NA; i<NB; i+=2*NA)
		Multiply(T+NA+i, T, A, B+i, NA);
	for (i=NA; i<NB; i+=2*NA)
		Multiply(R+i, T, A, B+i, NA);

	if (Add(R+NA, R+NA, T+2*NA, NB-NA))
		Increment(R+NB, NA);
}

// R[N] ----- result = A inverse mod 2**(WORD_BITS*N)
// T[3*N/2] - temporary work space
// A[N] ----- an odd number as input

void RecursiveInverseModPower2(word *R, word *T, const word *A, unsigned int N)
{
	if (N==2)
		AtomicInverseModPower2(R, A[0], A[1]);
	else
	{
		const unsigned int N2 = N/2;
		RecursiveInverseModPower2(R0, T0, A0, N2);
		T0[0] = 1;
		SetWords(T0+1, 0, N2-1);
		MultiplyTop(R1, T1, T0, R0, A0, N2);
		MultiplyBottom(T0, T1, R0, A1, N2);
		Add(T0, R1, T0, N2);
		TwosComplement(T0, N2);
		MultiplyBottom(R1, T1, R0, T0, N2);
	}
}

// R[N] --- result = X/(2**(WORD_BITS*N)) mod M
// T[3*N] - temporary work space
// X[2*N] - number to be reduced
// M[N] --- modulus
// U[N] --- multiplicative inverse of M mod 2**(WORD_BITS*N)

void MontgomeryReduce(word *R, word *T, const word *X, const word *M, const word *U, unsigned int N)
{
	MultiplyBottom(R, T, X, U, N);
	MultiplyTop(T, T+N, X, R, M, N);
	word borrow = Subtract(T, X+N, T, N);
	// defend against timing attack by doing this Add even when not needed
	word carry = Add(T+N, T, M, N);
	assert(carry || !borrow);
	CopyWords(R, T + (borrow ? N : 0), N);
}

// R[N] --- result = X/(2**(WORD_BITS*N/2)) mod M
// T[2*N] - temporary work space
// X[2*N] - number to be reduced
// M[N] --- modulus
// U[N/2] - multiplicative inverse of M mod 2**(WORD_BITS*N/2)
// V[N] --- 2**(WORD_BITS*3*N/2) mod M

void HalfMontgomeryReduce(word *R, word *T, const word *X, const word *M, const word *U, const word *V, unsigned int N)
{
	assert(N%2==0 && N>=4);

#define M0		M
#define M1		(M+N2)
#define V0		V
#define V1		(V+N2)

#define X0		X
#define X1		(X+N2)
#define X2		(X+N)
#define X3		(X+N+N2)

	const unsigned int N2 = N/2;
	Multiply(T0, T2, V0, X3, N2);
	int c2 = Add(T0, T0, X0, N);
	MultiplyBottom(T3, T2, T0, U, N2);
	MultiplyTop(T2, R, T0, T3, M0, N2);
	c2 -= Subtract(T2, T1, T2, N2);
	Multiply(T0, R, T3, M1, N2);
	c2 -= Subtract(T0, T2, T0, N2);
	int c3 = -(int)Subtract(T1, X2, T1, N2);
	Multiply(R0, T2, V1, X3, N2);
	c3 += Add(R, R, T, N);

	if (c2>0)
		c3 += Increment(R1, N2);
	else if (c2<0)
		c3 -= Decrement(R1, N2, -c2);

	assert(c3>=-1 && c3<=1);
	if (c3>0)
		Subtract(R, R, M, N);
	else if (c3<0)
		Add(R, R, M, N);

#undef M0
#undef M1
#undef V0
#undef V1

#undef X0
#undef X1
#undef X2
#undef X3
}

#undef A0
#undef A1
#undef B0
#undef B1

#undef T0
#undef T1
#undef T2
#undef T3

#undef R0
#undef R1
#undef R2
#undef R3

// do a 3 word by 2 word divide, returns quotient and leaves remainder in A
static word SubatomicDivide(word *A, word B0, word B1)
{
	// assert {A[2],A[1]} < {B1,B0}, so quotient can fit in a word
	assert(A[2] < B1 || (A[2]==B1 && A[1] < B0));

	dword p, u;
	word Q;

	// estimate the quotient: do a 2 word by 1 word divide
	if (B1+1 == 0)
		Q = A[2];
	else
		Q = word(MAKE_DWORD(A[1], A[2]) / (B1+1));

	// now subtract Q*B from A
	p = (dword) B0*Q;
	u = (dword) A[0] - LOW_WORD(p);
	A[0] = LOW_WORD(u);
	u = (dword) A[1] - HIGH_WORD(p) - (word)(0-HIGH_WORD(u)) - (dword)B1*Q;
	A[1] = LOW_WORD(u);
	A[2] += HIGH_WORD(u);

	// Q <= actual quotient, so fix it
	while (A[2] || A[1] > B1 || (A[1]==B1 && A[0]>=B0))
	{
		u = (dword) A[0] - B0;
		A[0] = LOW_WORD(u);
		u = (dword) A[1] - B1 - (word)(0-HIGH_WORD(u));
		A[1] = LOW_WORD(u);
		A[2] += HIGH_WORD(u);
		Q++;
		assert(Q);	// shouldn't overflow
	}

	return Q;
}

// do a 4 word by 2 word divide, returns 2 word quotient in Q0 and Q1
static inline void AtomicDivide(word *Q, const word *A, const word *B)
{
	if (!B[0] && !B[1]) // if divisor is 0, we assume divisor==2**(2*WORD_BITS)
	{
		Q[0] = A[2];
		Q[1] = A[3];
	}
	else
	{
		word T[4];
		T[0] = A[0]; T[1] = A[1]; T[2] = A[2]; T[3] = A[3];
		Q[1] = SubatomicDivide(T+1, B[0], B[1]);
		Q[0] = SubatomicDivide(T, B[0], B[1]);

#ifndef NDEBUG
		// multiply quotient and divisor and add remainder, make sure it equals dividend
		assert(!T[2] && !T[3] && (T[1] < B[1] || (T[1]==B[1] && T[0]<B[0])));
		word P[4];
		LowLevel::Multiply2(P, Q, B);
		Add(P, P, T, 4);
		assert(memcmp(P, A, 4*WORD_SIZE)==0);
#endif
	}
}

// for use by Divide(), corrects the underestimated quotient {Q1,Q0}
static void CorrectQuotientEstimate(word *R, word *T, word *Q, const word *B, unsigned int N)
{
	assert(N && N%2==0);

	if (Q[1])
	{
		T[N] = T[N+1] = 0;
		unsigned i;
		for (i=0; i<N; i+=4)
			LowLevel::Multiply2(T+i, Q, B+i);
		for (i=2; i<N; i+=4)
			if (LowLevel::Multiply2Add(T+i, Q, B+i))
				T[i+5] += (++T[i+4]==0);
	}
	else
	{
		T[N] = LinearMultiply(T, B, Q[0], N);
		T[N+1] = 0;
	}

	word borrow = Subtract(R, R, T, N+2);
	assert(!borrow && !R[N+1]);

	while (R[N] || Compare(R, B, N) >= 0)
	{
		R[N] -= Subtract(R, R, B, N);
		Q[1] += (++Q[0]==0);
		assert(Q[0] || Q[1]); // no overflow
	}
}

// R[NB] -------- remainder = A%B
// Q[NA-NB+2] --- quotient	= A/B
// T[NA+2*NB+4] - temp work space
// A[NA] -------- dividend
// B[NB] -------- divisor

void Divide(word *R, word *Q, word *T, const word *A, unsigned int NA, const word *B, unsigned int NB)
{
	assert(NA && NB && NA%2==0 && NB%2==0);
	assert(B[NB-1] || B[NB-2]);
	assert(NB <= NA);

	// set up temporary work space
	word *const TA=T;
	word *const TB=T+NA+2;
	word *const TP=T+NA+2+NB;

	// copy B into TB and normalize it so that TB has highest bit set to 1
	unsigned shiftWords = (B[NB-1]==0);
	TB[0] = TB[NB-1] = 0;
	CopyWords(TB+shiftWords, B, NB-shiftWords);
	unsigned shiftBits = WORD_BITS - BitPrecision(TB[NB-1]);
	assert(shiftBits < WORD_BITS);
	ShiftWordsLeftByBits(TB, NB, shiftBits);

	// copy A into TA and normalize it
	TA[0] = TA[NA] = TA[NA+1] = 0;
	CopyWords(TA+shiftWords, A, NA);
	ShiftWordsLeftByBits(TA, NA+2, shiftBits);

	if (TA[NA+1]==0 && TA[NA] <= 1)
	{
		Q[NA-NB+1] = Q[NA-NB] = 0;
		while (TA[NA] || Compare(TA+NA-NB, TB, NB) >= 0)
		{
			TA[NA] -= Subtract(TA+NA-NB, TA+NA-NB, TB, NB);
			++Q[NA-NB];
		}
	}
	else
	{
		NA+=2;
		assert(Compare(TA+NA-NB, TB, NB) < 0);
	}

	word BT[2];
	BT[0] = TB[NB-2] + 1;
	BT[1] = TB[NB-1] + (BT[0]==0);

	// start reducing TA mod TB, 2 words at a time
	for (unsigned i=NA-2; i>=NB; i-=2)
	{
		AtomicDivide(Q+i-NB, TA+i-2, BT);
		CorrectQuotientEstimate(TA+i-NB, TP, Q+i-NB, TB, NB);
	}

	// copy TA into R, and denormalize it
	CopyWords(R, TA+shiftWords, NB);
	ShiftWordsRightByBits(R, NB, shiftBits);
}

static inline unsigned int EvenWordCount(const word *X, unsigned int N)
{
	while (N && X[N-2]==0 && X[N-1]==0)
		N-=2;
	return N;
}

// return k
// R[N] --- result = A^(-1) * 2^k mod M
// T[4*N] - temporary work space
// A[NA] -- number to take inverse of
// M[N] --- modulus

unsigned int AlmostInverse(word *R, word *T, const word *A, unsigned int NA, const word *M, unsigned int N)
{
	assert(NA<=N && N && N%2==0);

	word *b = T;
	word *c = T+N;
	word *f = T+2*N;
	word *g = T+3*N;
	unsigned int bcLen=2, fgLen=EvenWordCount(M, N);
	unsigned int k=0, s=0;

	SetWords(T, 0, 3*N);
	b[0]=1;
	CopyWords(f, A, NA);
	CopyWords(g, M, N);

	while (1)
	{
		word t=f[0];
		while (!t)
		{
			if (EvenWordCount(f, fgLen)==0)
			{
				SetWords(R, 0, N);
				return 0;
			}

			ShiftWordsRightByWords(f, fgLen, 1);
			if (c[bcLen-1]) bcLen+=2;
			assert(bcLen <= N);
			ShiftWordsLeftByWords(c, bcLen, 1);
			k+=WORD_BITS;
			t=f[0];
		}

		unsigned int i=0;
		while (t%2 == 0)
		{
			t>>=1;
			i++;
		}
		k+=i;

		if (t==1 && f[1]==0 && EvenWordCount(f, fgLen)==2)
		{
			if (s%2==0)
				CopyWords(R, b, N);
			else
				Subtract(R, M, b, N);
			return k;
		}

		ShiftWordsRightByBits(f, fgLen, i);
		t=ShiftWordsLeftByBits(c, bcLen, i);
		if (t)
		{
			c[bcLen] = t;
			bcLen+=2;
			assert(bcLen <= N);
		}

		if (f[fgLen-2]==0 && g[fgLen-2]==0 && f[fgLen-1]==0 && g[fgLen-1]==0)
			fgLen-=2;

		if (Compare(f, g, fgLen)==-1)
		{
			std::swap(f, g);
			std::swap(b, c);
			s++;
		}

		Subtract(f, f, g, fgLen);

		if (Add(b, b, c, bcLen))
		{
			b[bcLen] = 1;
			bcLen+=2;
			assert(bcLen <= N);
		}
	}
}

// R[N] - result = A/(2^k) mod M
// A[N] - input
// M[N] - modulus

void DivideByPower2Mod(word *R, const word *A, unsigned int k, const word *M, unsigned int N)
{
	CopyWords(R, A, N);

	while (k--)
	{
		if (R[0]%2==0)
			ShiftWordsRightByBits(R, N, 1);
		else
		{
			word carry = Add(R, R, M, N);
			ShiftWordsRightByBits(R, N, 1);
			R[N-1] += carry<<(WORD_BITS-1);
		}
	}
}

// R[N] - result = A*(2^k) mod M
// A[N] - input
// M[N] - modulus

void MultiplyByPower2Mod(word *R, const word *A, unsigned int k, const word *M, unsigned int N)
{
	CopyWords(R, A, N);

	while (k--)
		if (ShiftWordsLeftByBits(R, N, 1) || Compare(R, M, N)>=0)
			Subtract(R, R, M, N);
}

// ******************************************************************

static const unsigned int RoundupSizeTable[] = {2, 2, 2, 4, 4, 8, 8, 8, 8};

static inline unsigned int RoundupSize(unsigned int n)
{
	if (n<=8)
		return RoundupSizeTable[n];
	else if (n<=16)
		return 16;
	else if (n<=32)
		return 32;
	else if (n<=64)
		return 64;
	else return 1U << BitPrecision(n-1);
}

Integer::Integer()
	: reg(2), sign(POSITIVE)
{
	reg[0] = reg[1] = 0;
}

Integer::Integer(const Integer& t)
	: reg(RoundupSize(t.WordCount())), sign(t.sign)
{
	CopyWords(reg, t.reg, reg.size());
}

Integer::Integer(signed long value)
	: reg(2)
{
	if (value >= 0)
		sign = POSITIVE;
	else
	{
		sign = NEGATIVE;
		value = -value;
	}
	reg[0] = word(value);
	reg[1] = word(SafeRightShift<WORD_BITS, unsigned long>(value));
}

Integer::Integer(Sign s, word high, word low)
	: reg(2), sign(s)
{
	reg[0] = low;
	reg[1] = high;
}

bool Integer::IsConvertableToLong() const
{
	if (ByteCount() > sizeof(long))
		return false;

	unsigned long value = reg[0];
	value += SafeLeftShift<WORD_BITS, unsigned long>(reg[1]);

	if (sign==POSITIVE)
		return (signed long)value >= 0;
	else
		return -(signed long)value < 0;
}

signed long Integer::ConvertToLong() const
{
	assert(IsConvertableToLong());

	unsigned long value = reg[0];
	value += SafeLeftShift<WORD_BITS, unsigned long>(reg[1]);
	return sign==POSITIVE ? value : -(signed long)value;
}

Integer::Integer(BufferedTransformation &encodedInteger, unsigned int byteCount, Signedness s)
{
	Decode(encodedInteger, byteCount, s);
}

Integer::Integer(const byte *encodedInteger, unsigned int byteCount, Signedness s)
{
	Decode(encodedInteger, byteCount, s);
}

Integer::Integer(BufferedTransformation &bt)
{
	BERDecode(bt);
}

Integer::Integer(RandomNumberGenerator &rng, unsigned int bitcount)
{
	Randomize(rng, bitcount);
}

Integer::Integer(RandomNumberGenerator &rng, const Integer &min, const Integer &max, RandomNumberType rnType, const Integer &equiv, const Integer &mod)
{
	if (!Randomize(rng, min, max, rnType, equiv, mod))
		throw Integer::RandomNumberNotFound();
}

Integer Integer::Power2(unsigned int e)
{
	Integer r((word)0, BitsToWords(e+1));
	r.SetBit(e);
	return r;
}

const Integer &Integer::Zero()
{
	static const Integer zero;
	return zero;
}

const Integer &Integer::One()
{
	static const Integer one(1,2);
	return one;
}

const Integer &Integer::Two()
{
	static const Integer two(2,2);
	return two;
}

bool Integer::operator!() const
{
	return IsNegative() ? false : (reg[0]==0 && WordCount()==0);
}

Integer& Integer::operator=(const Integer& t)
{
	if (this != &t)
	{
		reg.New(RoundupSize(t.WordCount()));
		CopyWords(reg, t.reg, reg.size());
		sign = t.sign;
	}
	return *this;
}

bool Integer::GetBit(unsigned int n) const
{
	if (n/WORD_BITS >= reg.size())
		return 0;
	else
		return bool((reg[n/WORD_BITS] >> (n % WORD_BITS)) & 1);
}

void Integer::SetBit(unsigned int n, bool value)
{
	if (value)
	{
		reg.CleanGrow(RoundupSize(BitsToWords(n+1)));
		reg[n/WORD_BITS] |= (word(1) << (n%WORD_BITS));
	}
	else
	{
		if (n/WORD_BITS < reg.size())
			reg[n/WORD_BITS] &= ~(word(1) << (n%WORD_BITS));
	}
}

byte Integer::GetByte(unsigned int n) const
{
	if (n/WORD_SIZE >= reg.size())
		return 0;
	else
		return byte(reg[n/WORD_SIZE] >> ((n%WORD_SIZE)*8));
}

void Integer::SetByte(unsigned int n, byte value)
{
	reg.CleanGrow(RoundupSize(BytesToWords(n+1)));
	reg[n/WORD_SIZE] &= ~(word(0xff) << 8*(n%WORD_SIZE));
	reg[n/WORD_SIZE] |= (word(value) << 8*(n%WORD_SIZE));
}

unsigned long Integer::GetBits(unsigned int i, unsigned int n) const
{
	assert(n <= sizeof(unsigned long)*8);
	unsigned long v = 0;
	for (unsigned int j=0; j<n; j++)
		v |= GetBit(i+j) << j;
	return v;
}

Integer Integer::operator-() const
{
	Integer result(*this);
	result.Negate();
	return result;
}

Integer Integer::AbsoluteValue() const
{
	Integer result(*this);
	result.sign = POSITIVE;
	return result;
}

void Integer::swap(Integer &a)
{
	reg.swap(a.reg);
	std::swap(sign, a.sign);
}

Integer::Integer(word value, unsigned int length)
	: reg(RoundupSize(length)), sign(POSITIVE)
{
	reg[0] = value;
	SetWords(reg+1, 0, reg.size()-1);
}

template <class T>
static Integer StringToInteger(const T *str)
{
	word radix;
#if (defined(__GNUC__) && __GNUC__ <= 3)		// GCC workaround
	// std::char_traits doesn't exist in GCC 2.x
	// std::char_traits<wchar_t>::length() not defined in GCC 3.2
	unsigned int length;
	for (length = 0; str[length] != 0; length++) {}
#else
	unsigned int length = std::char_traits<T>::length(str);
#endif

	Integer v;

	if (length == 0)
		return v;

	switch (str[length-1])
	{
	case 'h':
	case 'H':
		radix=16;
		break;
	case 'o':
	case 'O':
		radix=8;
		break;
	case 'b':
	case 'B':
		radix=2;
		break;
	default:
		radix=10;
	}

	if (length > 2 && str[0] == '0' && str[1] == 'x')
		radix = 16;

	for (unsigned i=0; i<length; i++)
	{
		word digit;

		if (str[i] >= '0' && str[i] <= '9')
			digit = str[i] - '0';
		else if (str[i] >= 'A' && str[i] <= 'F')
			digit = str[i] - 'A' + 10;
		else if (str[i] >= 'a' && str[i] <= 'f')
			digit = str[i] - 'a' + 10;
		else
			digit = radix;

		if (digit < radix)
		{
			v *= radix;
			v += digit;
		}
	}

	if (str[0] == '-')
		v.Negate();

	return v;
}

Integer::Integer(const char *str)
	: reg(2), sign(POSITIVE)
{
	*this = StringToInteger(str);
}

Integer::Integer(const wchar_t *str)
	: reg(2), sign(POSITIVE)
{
	*this = StringToInteger(str);
}

unsigned int Integer::WordCount() const
{
	return CountWords(reg, reg.size());
}

unsigned int Integer::ByteCount() const
{
	unsigned wordCount = WordCount();
	if (wordCount)
		return (wordCount-1)*WORD_SIZE + BytePrecision(reg[wordCount-1]);
	else
		return 0;
}

unsigned int Integer::BitCount() const
{
	unsigned wordCount = WordCount();
	if (wordCount)
		return (wordCount-1)*WORD_BITS + BitPrecision(reg[wordCount-1]);
	else
		return 0;
}

void Integer::Decode(const byte *input, unsigned int inputLen, Signedness s)
{
	StringStore store(input, inputLen);
	Decode(store, inputLen, s);
}

void Integer::Decode(BufferedTransformation &bt, unsigned int inputLen, Signedness s)
{
	assert(bt.MaxRetrievable() >= inputLen);

	byte b;
	bt.Peek(b);
	sign = ((s==SIGNED) && (b & 0x80)) ? NEGATIVE : POSITIVE;

	while (inputLen>0 && (sign==POSITIVE ? b==0 : b==0xff))
	{
		bt.Skip(1);
		inputLen--;
		bt.Peek(b);
	}

	reg.CleanNew(RoundupSize(BytesToWords(inputLen)));

	for (unsigned int i=inputLen; i > 0; i--)
	{
		bt.Get(b);
		reg[(i-1)/WORD_SIZE] |= b << ((i-1)%WORD_SIZE)*8;
	}

	if (sign == NEGATIVE)
	{
		for (unsigned i=inputLen; i<reg.size()*WORD_SIZE; i++)
			reg[i/WORD_SIZE] |= 0xff << (i%WORD_SIZE)*8;
		TwosComplement(reg, reg.size());
	}
}

unsigned int Integer::MinEncodedSize(Signedness signedness) const
{
	unsigned int outputLen = STDMAX(1U, ByteCount());
	if (signedness == UNSIGNED)
		return outputLen;
	if (NotNegative() && (GetByte(outputLen-1) & 0x80))
		outputLen++;
	if (IsNegative() && *this < -Power2(outputLen*8-1))
		outputLen++;
	return outputLen;
}

unsigned int Integer::Encode(byte *output, unsigned int outputLen, Signedness signedness) const
{
	ArraySink sink(output, outputLen);
	return Encode(sink, outputLen, signedness);
}

unsigned int Integer::Encode(BufferedTransformation &bt, unsigned int outputLen, Signedness signedness) const
{
	if (signedness == UNSIGNED || NotNegative())
	{
		for (unsigned int i=outputLen; i > 0; i--)
			bt.Put(GetByte(i-1));
	}
	else
	{
		// take two's complement of *this
		Integer temp = Integer::Power2(8*STDMAX(ByteCount(), outputLen)) + *this;
		for (unsigned i=0; i<outputLen; i++)
			bt.Put(temp.GetByte(outputLen-i-1));
	}
	return outputLen;
}

void Integer::DEREncode(BufferedTransformation &bt) const
{
	DERGeneralEncoder enc(bt, INTEGER);
	Encode(enc, MinEncodedSize(SIGNED), SIGNED);
	enc.MessageEnd();
}

void Integer::BERDecode(const byte *input, unsigned int len)
{
	StringStore store(input, len);
	BERDecode(store);
}

void Integer::BERDecode(BufferedTransformation &bt)
{
	BERGeneralDecoder dec(bt, INTEGER);
	if (!dec.IsDefiniteLength() || dec.MaxRetrievable() < dec.RemainingLength())
		BERDecodeError();
	Decode(dec, dec.RemainingLength(), SIGNED);
	dec.MessageEnd();
}

void Integer::DEREncodeAsOctetString(BufferedTransformation &bt, unsigned int length) const
{
	DERGeneralEncoder enc(bt, OCTET_STRING);
	Encode(enc, length);
	enc.MessageEnd();
}

void Integer::BERDecodeAsOctetString(BufferedTransformation &bt, unsigned int length)
{
	BERGeneralDecoder dec(bt, OCTET_STRING);
	if (!dec.IsDefiniteLength() || dec.RemainingLength() != length)
		BERDecodeError();
	Decode(dec, length);
	dec.MessageEnd();
}

unsigned int Integer::OpenPGPEncode(byte *output, unsigned int len) const
{
	ArraySink sink(output, len);
	return OpenPGPEncode(sink);
}

unsigned int Integer::OpenPGPEncode(BufferedTransformation &bt) const
{
	word16 bitCount = BitCount();
	bt.PutWord16(bitCount);
	return 2 + Encode(bt, BitsToBytes(bitCount));
}

void Integer::OpenPGPDecode(const byte *input, unsigned int len)
{
	StringStore store(input, len);
	OpenPGPDecode(store);
}

void Integer::OpenPGPDecode(BufferedTransformation &bt)
{
	word16 bitCount;
	if (bt.GetWord16(bitCount) != 2 || bt.MaxRetrievable() < BitsToBytes(bitCount))
		throw OpenPGPDecodeErr();
	Decode(bt, BitsToBytes(bitCount));
}

void Integer::Randomize(RandomNumberGenerator &rng, unsigned int nbits)
{
	const unsigned int nbytes = nbits/8 + 1;
	SecByteBlock buf(nbytes);
	rng.GenerateBlock(buf, nbytes);
	if (nbytes)
		buf[0] = (byte)Crop(buf[0], nbits % 8);
	Decode(buf, nbytes, UNSIGNED);
}

void Integer::Randomize(RandomNumberGenerator &rng, const Integer &min, const Integer &max)
{
	if (min > max)
		throw InvalidArgument("Integer: Min must be no greater than Max");

	Integer range = max - min;
	const unsigned int nbits = range.BitCount();

	do
	{
		Randomize(rng, nbits);
	}
	while (*this > range);

	*this += min;
}

bool Integer::Randomize(RandomNumberGenerator &rng, const Integer &min, const Integer &max, RandomNumberType rnType, const Integer &equiv, const Integer &mod)
{
	return GenerateRandomNoThrow(rng, MakeParameters("Min", min)("Max", max)("RandomNumberType", rnType)("EquivalentTo", equiv)("Mod", mod));
}

class KDF2_RNG : public RandomNumberGenerator
{
public:
	KDF2_RNG(const byte *seed, unsigned int seedSize)
		: m_counter(0), m_counterAndSeed(seedSize + 4)
	{
		memcpy(m_counterAndSeed + 4, seed, seedSize);
	}

	byte GenerateByte()
	{
		byte b;
		GenerateBlock(&b, 1);
		return b;
	}

	void GenerateBlock(byte *output, unsigned int size)
	{
		UnalignedPutWord(BIG_ENDIAN_ORDER, m_counterAndSeed, m_counter);
		++m_counter;
		P1363_KDF2<SHA1>::DeriveKey(output, size, m_counterAndSeed, m_counterAndSeed.size());
	}

private:
	word32 m_counter;
	SecByteBlock m_counterAndSeed;
};

bool Integer::GenerateRandomNoThrow(RandomNumberGenerator &i_rng, const NameValuePairs &params)
{
	Integer min = params.GetValueWithDefault("Min", Integer::Zero());
	Integer max;
	if (!params.GetValue("Max", max))
	{
		int bitLength;
		if (params.GetIntValue("BitLength", bitLength))
			max = Integer::Power2(bitLength);
		else
			throw InvalidArgument("Integer: missing Max argument");
	}
	if (min > max)
		throw InvalidArgument("Integer: Min must be no greater than Max");

	Integer equiv = params.GetValueWithDefault("EquivalentTo", Integer::Zero());
	Integer mod = params.GetValueWithDefault("Mod", Integer::One());

	if (equiv.IsNegative() || equiv >= mod)
		throw InvalidArgument("Integer: invalid EquivalentTo and/or Mod argument");

	Integer::RandomNumberType rnType = params.GetValueWithDefault("RandomNumberType", Integer::ANY);

	member_ptr<KDF2_RNG> kdf2Rng;
	ConstByteArrayParameter seed;
	if (params.GetValue("Seed", seed))
	{
		ByteQueue bq;
		DERSequenceEncoder seq(bq);
		min.DEREncode(seq);
		max.DEREncode(seq);
		equiv.DEREncode(seq);
		mod.DEREncode(seq);
		DEREncodeUnsigned(seq, rnType);
		DEREncodeOctetString(seq, seed.begin(), seed.size());
		seq.MessageEnd();

		SecByteBlock finalSeed(bq.MaxRetrievable());
		bq.Get(finalSeed, finalSeed.size());
		kdf2Rng.reset(new KDF2_RNG(finalSeed.begin(), finalSeed.size()));
	}
	RandomNumberGenerator &rng = kdf2Rng.get() ? (RandomNumberGenerator &)*kdf2Rng : i_rng;

	switch (rnType)
	{
		case ANY:
			if (mod == One())
				Randomize(rng, min, max);
			else
			{
				Integer min1 = min + (equiv-min)%mod;
				if (max < min1)
					return false;
				Randomize(rng, Zero(), (max - min1) / mod);
				*this *= mod;
				*this += min1;
			}
			return true;

		case PRIME:
		{
			const PrimeSelector *pSelector = params.GetValueWithDefault("PointerToPrimeSelector", (const PrimeSelector *)NULL);

			int i;
			i = 0;
			while (1)
			{
				if (++i==16)
				{
					// check if there are any suitable primes in [min, max]
					Integer first = min;
					if (FirstPrime(first, max, equiv, mod, pSelector))
					{
						// if there is only one suitable prime, we're done
						*this = first;
						if (!FirstPrime(first, max, equiv, mod, pSelector))
							return true;
					}
					else
						return false;
				}

				Randomize(rng, min, max);
				if (FirstPrime(*this, STDMIN(*this+mod*PrimeSearchInterval(max), max), equiv, mod, pSelector))
					return true;
			}
		}

		default:
			throw InvalidArgument("Integer: invalid RandomNumberType argument");
	}
}

std::istream& operator>>(std::istream& in, Integer &a)
{
	char c;
	unsigned int length = 0;
	SecBlock<char> str(length + 16);

	std::ws(in);

	do
	{
		in.read(&c, 1);
		str[length++] = c;
		if (length >= str.size())
			str.Grow(length + 16);
	}
	while (in && (c=='-' || c=='x' || (c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F') || c=='h' || c=='H' || c=='o' || c=='O' || c==',' || c=='.'));

	if (in.gcount())
		in.putback(c);
	str[length-1] = '\0';
	a = Integer(str);

	return in;
}

std::ostream& operator<<(std::ostream& out, const Integer &a)
{
	// Get relevant conversion specifications from ostream.
	long f = out.flags() & std::ios::basefield; // Get base digits.
	int base, block;
	char suffix;
	switch(f)
	{
	case std::ios::oct :
		base = 8;
		block = 8;
		suffix = 'o';
		break;
	case std::ios::hex :
		base = 16;
		block = 4;
		suffix = 'h';
		break;
	default :
		base = 10;
		block = 3;
		suffix = '.';
	}

	SecBlock<char> s(a.BitCount() / (BitPrecision(base)-1) + 1);
	Integer temp1=a, temp2;
	unsigned i=0;
	const char vec[]="0123456789ABCDEF";

	if (a.IsNegative())
	{
		out << '-';
		temp1.Negate();
	}

	if (!a)
		out << '0';

	while (!!temp1)
	{
		word digit;
		Integer::Divide(digit, temp2, temp1, base);
		s[i++]=vec[digit];
		temp1=temp2;
	}

	while (i--)
	{
		out << s[i];
//		if (i && !(i%block))
//			out << ",";
	}
	return out << suffix;
}

Integer& Integer::operator++()
{
	if (NotNegative())
	{
		if (Increment(reg, reg.size()))
		{
			reg.CleanGrow(2*reg.size());
			reg[reg.size()/2]=1;
		}
	}
	else
	{
		word borrow = Decrement(reg, reg.size());
		assert(!borrow);
		if (WordCount()==0)
			*this = Zero();
	}
	return *this;
}

Integer& Integer::operator--()
{
	if (IsNegative())
	{
		if (Increment(reg, reg.size()))
		{
			reg.CleanGrow(2*reg.size());
			reg[reg.size()/2]=1;
		}
	}
	else
	{
		if (Decrement(reg, reg.size()))
			*this = -One();
	}
	return *this;
}

void PositiveAdd(Integer &sum, const Integer &a, const Integer& b)
{
	word carry;
	if (a.reg.size() == b.reg.size())
		carry = Add(sum.reg, a.reg, b.reg, a.reg.size());
	else if (a.reg.size() > b.reg.size())
	{
		carry = Add(sum.reg, a.reg, b.reg, b.reg.size());
		CopyWords(sum.reg+b.reg.size(), a.reg+b.reg.size(), a.reg.size()-b.reg.size());
		carry = Increment(sum.reg+b.reg.size(), a.reg.size()-b.reg.size(), carry);
	}
	else
	{
		carry = Add(sum.reg, a.reg, b.reg, a.reg.size());
		CopyWords(sum.reg+a.reg.size(), b.reg+a.reg.size(), b.reg.size()-a.reg.size());
		carry = Increment(sum.reg+a.reg.size(), b.reg.size()-a.reg.size(), carry);
	}

	if (carry)
	{
		sum.reg.CleanGrow(2*sum.reg.size());
		sum.reg[sum.reg.size()/2] = 1;
	}
	sum.sign = Integer::POSITIVE;
}

void PositiveSubtract(Integer &diff, const Integer &a, const Integer& b)
{
	unsigned aSize = a.WordCount();
	aSize += aSize%2;
	unsigned bSize = b.WordCount();
	bSize += bSize%2;

	if (aSize == bSize)
	{
		if (Compare(a.reg, b.reg, aSize) >= 0)
		{
			Subtract(diff.reg, a.reg, b.reg, aSize);
			diff.sign = Integer::POSITIVE;
		}
		else
		{
			Subtract(diff.reg, b.reg, a.reg, aSize);
			diff.sign = Integer::NEGATIVE;
		}
	}
	else if (aSize > bSize)
	{
		word borrow = Subtract(diff.reg, a.reg, b.reg, bSize);
		CopyWords(diff.reg+bSize, a.reg+bSize, aSize-bSize);
		borrow = Decrement(diff.reg+bSize, aSize-bSize, borrow);
		assert(!borrow);
		diff.sign = Integer::POSITIVE;
	}
	else
	{
		word borrow = Subtract(diff.reg, b.reg, a.reg, aSize);
		CopyWords(diff.reg+aSize, b.reg+aSize, bSize-aSize);
		borrow = Decrement(diff.reg+aSize, bSize-aSize, borrow);
		assert(!borrow);
		diff.sign = Integer::NEGATIVE;
	}
}

Integer Integer::Plus(const Integer& b) const
{
	Integer sum((word)0, STDMAX(reg.size(), b.reg.size()));
	if (NotNegative())
	{
		if (b.NotNegative())
			PositiveAdd(sum, *this, b);
		else
			PositiveSubtract(sum, *this, b);
	}
	else
	{
		if (b.NotNegative())
			PositiveSubtract(sum, b, *this);
		else
		{
			PositiveAdd(sum, *this, b);
			sum.sign = Integer::NEGATIVE;
		}
	}
	return sum;
}

Integer& Integer::operator+=(const Integer& t)
{
	reg.CleanGrow(t.reg.size());
	if (NotNegative())
	{
		if (t.NotNegative())
			PositiveAdd(*this, *this, t);
		else
			PositiveSubtract(*this, *this, t);
	}
	else
	{
		if (t.NotNegative())
			PositiveSubtract(*this, t, *this);
		else
		{
			PositiveAdd(*this, *this, t);
			sign = Integer::NEGATIVE;
		}
	}
	return *this;
}

Integer Integer::Minus(const Integer& b) const
{
	Integer diff((word)0, STDMAX(reg.size(), b.reg.size()));
	if (NotNegative())
	{
		if (b.NotNegative())
			PositiveSubtract(diff, *this, b);
		else
			PositiveAdd(diff, *this, b);
	}
	else
	{
		if (b.NotNegative())
		{
			PositiveAdd(diff, *this, b);
			diff.sign = Integer::NEGATIVE;
		}
		else
			PositiveSubtract(diff, b, *this);
	}
	return diff;
}

Integer& Integer::operator-=(const Integer& t)
{
	reg.CleanGrow(t.reg.size());
	if (NotNegative())
	{
		if (t.NotNegative())
			PositiveSubtract(*this, *this, t);
		else
			PositiveAdd(*this, *this, t);
	}
	else
	{
		if (t.NotNegative())
		{
			PositiveAdd(*this, *this, t);
			sign = Integer::NEGATIVE;
		}
		else
			PositiveSubtract(*this, t, *this);
	}
	return *this;
}

Integer& Integer::operator<<=(unsigned int n)
{
	const unsigned int wordCount = WordCount();
	const unsigned int shiftWords = n / WORD_BITS;
	const unsigned int shiftBits = n % WORD_BITS;

	reg.CleanGrow(RoundupSize(wordCount+BitsToWords(n)));
	ShiftWordsLeftByWords(reg, wordCount + shiftWords, shiftWords);
	ShiftWordsLeftByBits(reg+shiftWords, wordCount+BitsToWords(shiftBits), shiftBits);
	return *this;
}

Integer& Integer::operator>>=(unsigned int n)
{
	const unsigned int wordCount = WordCount();
	const unsigned int shiftWords = n / WORD_BITS;
	const unsigned int shiftBits = n % WORD_BITS;

	ShiftWordsRightByWords(reg, wordCount, shiftWords);
	if (wordCount > shiftWords)
		ShiftWordsRightByBits(reg, wordCount-shiftWords, shiftBits);
	if (IsNegative() && WordCount()==0)   // avoid -0
		*this = Zero();
	return *this;
}

void PositiveMultiply(Integer &product, const Integer &a, const Integer &b)
{
	unsigned aSize = RoundupSize(a.WordCount());
	unsigned bSize = RoundupSize(b.WordCount());

	product.reg.CleanNew(RoundupSize(aSize+bSize));
	product.sign = Integer::POSITIVE;

	SecAlignedWordBlock workspace(aSize + bSize);
	AsymmetricMultiply(product.reg, workspace, a.reg, aSize, b.reg, bSize);
}

void Multiply(Integer &product, const Integer &a, const Integer &b)
{
	PositiveMultiply(product, a, b);

	if (a.NotNegative() != b.NotNegative())
		product.Negate();
}

Integer Integer::Times(const Integer &b) const
{
	Integer product;
	Multiply(product, *this, b);
	return product;
}

/*
void PositiveDivide(Integer &remainder, Integer &quotient,
				   const Integer &dividend, const Integer &divisor)
{
	remainder.reg.CleanNew(divisor.reg.size());
	remainder.sign = Integer::POSITIVE;
	quotient.reg.New(0);
	quotient.sign = Integer::POSITIVE;
	unsigned i=dividend.BitCount();
	while (i--)
	{
		word overflow = ShiftWordsLeftByBits(remainder.reg, remainder.reg.size(), 1);
		remainder.reg[0] |= dividend[i];
		if (overflow || remainder >= divisor)
		{
			Subtract(remainder.reg, remainder.reg, divisor.reg, remainder.reg.size());
			quotient.SetBit(i);
		}
	}
}
*/

void PositiveDivide(Integer &remainder, Integer &quotient,
				   const Integer &a, const Integer &b)
{
	unsigned aSize = a.WordCount();
	unsigned bSize = b.WordCount();

	if (!bSize)
		throw Integer::DivideByZero();

	if (a.PositiveCompare(b) == -1)
	{
		remainder = a;
		remainder.sign = Integer::POSITIVE;
		quotient = Integer::Zero();
		return;
	}

	aSize += aSize%2;	// round up to next even number
	bSize += bSize%2;

	remainder.reg.CleanNew(RoundupSize(bSize));
	remainder.sign = Integer::POSITIVE;
	quotient.reg.CleanNew(RoundupSize(aSize-bSize+2));
	quotient.sign = Integer::POSITIVE;

	SecAlignedWordBlock T(aSize+2*bSize+4);
	Divide(remainder.reg, quotient.reg, T, a.reg, aSize, b.reg, bSize);
}

void Integer::Divide(Integer &remainder, Integer &quotient, const Integer &dividend, const Integer &divisor)
{
	PositiveDivide(remainder, quotient, dividend, divisor);

	if (dividend.IsNegative())
	{
		quotient.Negate();
		if (remainder.NotZero())
		{
			--quotient;
			remainder = divisor.AbsoluteValue() - remainder;
		}
	}

	if (divisor.IsNegative())
		quotient.Negate();
}

void Integer::DivideByPowerOf2(Integer &r, Integer &q, const Integer &a, unsigned int n)
{
	q = a;
	q >>= n;

	const unsigned int wordCount = BitsToWords(n);
	if (wordCount <= a.WordCount())
	{
		r.reg.resize(RoundupSize(wordCount));
		CopyWords(r.reg, a.reg, wordCount);
		SetWords(r.reg+wordCount, 0, r.reg.size()-wordCount);
		if (n % WORD_BITS != 0)
			r.reg[wordCount-1] %= (1 << (n % WORD_BITS));
	}
	else
	{
		r.reg.resize(RoundupSize(a.WordCount()));
		CopyWords(r.reg, a.reg, r.reg.size());
	}
	r.sign = POSITIVE;

	if (a.IsNegative() && r.NotZero())
	{
		--q;
		r = Power2(n) - r;
	}
}

Integer Integer::DividedBy(const Integer &b) const
{
	Integer remainder, quotient;
	Integer::Divide(remainder, quotient, *this, b);
	return quotient;
}

Integer Integer::Modulo(const Integer &b) const
{
	Integer remainder, quotient;
	Integer::Divide(remainder, quotient, *this, b);
	return remainder;
}

void Integer::Divide(word &remainder, Integer &quotient, const Integer &dividend, word divisor)
{
	if (!divisor)
		throw Integer::DivideByZero();

	assert(divisor);

	if ((divisor & (divisor-1)) == 0)	// divisor is a power of 2
	{
		quotient = dividend >> (BitPrecision(divisor)-1);
		remainder = dividend.reg[0] & (divisor-1);
		return;
	}

	unsigned int i = dividend.WordCount();
	quotient.reg.CleanNew(RoundupSize(i));
	remainder = 0;
	while (i--)
	{
		quotient.reg[i] = word(MAKE_DWORD(dividend.reg[i], remainder) / divisor);
		remainder = word(MAKE_DWORD(dividend.reg[i], remainder) % divisor);
	}

	if (dividend.NotNegative())
		quotient.sign = POSITIVE;
	else
	{
		quotient.sign = NEGATIVE;
		if (remainder)
		{
			--quotient;
			remainder = divisor - remainder;
		}
	}
}

Integer Integer::DividedBy(word b) const
{
	word remainder;
	Integer quotient;
	Integer::Divide(remainder, quotient, *this, b);
	return quotient;
}

word Integer::Modulo(word divisor) const
{
	if (!divisor)
		throw Integer::DivideByZero();

	assert(divisor);

	word remainder;

	if ((divisor & (divisor-1)) == 0)	// divisor is a power of 2
		remainder = reg[0] & (divisor-1);
	else
	{
		unsigned int i = WordCount();

		if (divisor <= 5)
		{
			dword sum=0;
			while (i--)
				sum += reg[i];
			remainder = word(sum%divisor);
		}
		else
		{
			remainder = 0;
			while (i--)
				remainder = word(MAKE_DWORD(reg[i], remainder) % divisor);
		}
	}

	if (IsNegative() && remainder)
		remainder = divisor - remainder;

	return remainder;
}

void Integer::Negate()
{
	if (!!(*this))	// don't flip sign if *this==0
		sign = Sign(1-sign);
}

int Integer::PositiveCompare(const Integer& t) const
{
	unsigned size = WordCount(), tSize = t.WordCount();

	if (size == tSize)
		return CryptoPP::Compare(reg, t.reg, size);
	else
		return size > tSize ? 1 : -1;
}

int Integer::Compare(const Integer& t) const
{
	if (NotNegative())
	{
		if (t.NotNegative())
			return PositiveCompare(t);
		else
			return 1;
	}
	else
	{
		if (t.NotNegative())
			return -1;
		else
			return -PositiveCompare(t);
	}
}

Integer Integer::SquareRoot() const
{
	if (!IsPositive())
		return Zero();

	// overestimate square root
	Integer x, y = Power2((BitCount()+1)/2);
	assert(y*y >= *this);

	do
	{
		x = y;
		y = (x + *this/x) >> 1;
	} while (y<x);

	return x;
}

bool Integer::IsSquare() const
{
	Integer r = SquareRoot();
	return *this == r.Squared();
}

bool Integer::IsUnit() const
{
	return (WordCount() == 1) && (reg[0] == 1);
}

Integer Integer::MultiplicativeInverse() const
{
	return IsUnit() ? *this : Zero();
}

Integer a_times_b_mod_c(const Integer &x, const Integer& y, const Integer& m)
{
	return x*y%m;
}

Integer a_exp_b_mod_c(const Integer &x, const Integer& e, const Integer& m)
{
	ModularArithmetic mr(m);
	return mr.Exponentiate(x, e);
}

Integer Integer::Gcd(const Integer &a, const Integer &b)
{
	return EuclideanDomainOf<Integer>().Gcd(a, b);
}

Integer Integer::InverseMod(const Integer &m) const
{
	assert(m.NotNegative());

	if (IsNegative() || *this>=m)
		return (*this%m).InverseMod(m);

	if (m.IsEven())
	{
		if (!m || IsEven())
			return Zero();	// no inverse
		if (*this == One())
			return One();

		Integer u = m.InverseMod(*this);
		return !u ? Zero() : (m*(*this-u)+1)/(*this);
	}

	SecBlock<word> T(m.reg.size() * 4);
	Integer r((word)0, m.reg.size());
	unsigned k = AlmostInverse(r.reg, T, reg, reg.size(), m.reg, m.reg.size());
	DivideByPower2Mod(r.reg, r.reg, k, m.reg, m.reg.size());
	return r;
}

word Integer::InverseMod(const word mod) const
{
	word g0 = mod, g1 = *this % mod;
	word v0 = 0, v1 = 1;
	word y;

	while (g1)
	{
		if (g1 == 1)
			return v1;
		y = g0 / g1;
		g0 = g0 % g1;
		v0 += y * v1;

		if (!g0)
			break;
		if (g0 == 1)
			return mod-v0;
		y = g1 / g0;
		g1 = g1 % g0;
		v1 += y * v0;
	}
	return 0;
}

// ********************************************************

ModularArithmetic::ModularArithmetic(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
	OID oid(seq);
	if (oid != ASN1::prime_field())
		BERDecodeError();
	modulus.BERDecode(seq);
	seq.MessageEnd();
	result.reg.resize(modulus.reg.size());
}

void ModularArithmetic::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
	ASN1::prime_field().DEREncode(seq);
	modulus.DEREncode(seq);
	seq.MessageEnd();
}

void ModularArithmetic::DEREncodeElement(BufferedTransformation &out, const Element &a) const
{
	a.DEREncodeAsOctetString(out, MaxElementByteLength());
}

void ModularArithmetic::BERDecodeElement(BufferedTransformation &in, Element &a) const
{
	a.BERDecodeAsOctetString(in, MaxElementByteLength());
}

const Integer& ModularArithmetic::Half(const Integer &a) const
{
	if (a.reg.size()==modulus.reg.size())
	{
		CryptoPP::DivideByPower2Mod(result.reg.begin(), a.reg, 1, modulus.reg, a.reg.size());
		return result;
	}
	else
		return result1 = (a.IsEven() ? (a >> 1) : ((a+modulus) >> 1));
}

const Integer& ModularArithmetic::Add(const Integer &a, const Integer &b) const
{
	if (a.reg.size()==modulus.reg.size() && b.reg.size()==modulus.reg.size())
	{
		if (CryptoPP::Add(result.reg.begin(), a.reg, b.reg, a.reg.size())
			|| Compare(result.reg, modulus.reg, a.reg.size()) >= 0)
		{
			CryptoPP::Subtract(result.reg.begin(), result.reg, modulus.reg, a.reg.size());
		}
		return result;
	}
	else
	{
		result1 = a+b;
		if (result1 >= modulus)
			result1 -= modulus;
		return result1;
	}
}

Integer& ModularArithmetic::Accumulate(Integer &a, const Integer &b) const
{
	if (a.reg.size()==modulus.reg.size() && b.reg.size()==modulus.reg.size())
	{
		if (CryptoPP::Add(a.reg, a.reg, b.reg, a.reg.size())
			|| Compare(a.reg, modulus.reg, a.reg.size()) >= 0)
		{
			CryptoPP::Subtract(a.reg, a.reg, modulus.reg, a.reg.size());
		}
	}
	else
	{
		a+=b;
		if (a>=modulus)
			a-=modulus;
	}

	return a;
}

const Integer& ModularArithmetic::Subtract(const Integer &a, const Integer &b) const
{
	if (a.reg.size()==modulus.reg.size() && b.reg.size()==modulus.reg.size())
	{
		if (CryptoPP::Subtract(result.reg.begin(), a.reg, b.reg, a.reg.size()))
			CryptoPP::Add(result.reg.begin(), result.reg, modulus.reg, a.reg.size());
		return result;
	}
	else
	{
		result1 = a-b;
		if (result1.IsNegative())
			result1 += modulus;
		return result1;
	}
}

Integer& ModularArithmetic::Reduce(Integer &a, const Integer &b) const
{
	if (a.reg.size()==modulus.reg.size() && b.reg.size()==modulus.reg.size())
	{
		if (CryptoPP::Subtract(a.reg, a.reg, b.reg, a.reg.size()))
			CryptoPP::Add(a.reg, a.reg, modulus.reg, a.reg.size());
	}
	else
	{
		a-=b;
		if (a.IsNegative())
			a+=modulus;
	}

	return a;
}

const Integer& ModularArithmetic::Inverse(const Integer &a) const
{
	if (!a)
		return a;

	CopyWords(result.reg.begin(), modulus.reg, modulus.reg.size());
	if (CryptoPP::Subtract(result.reg.begin(), result.reg, a.reg, a.reg.size()))
		Decrement(result.reg.begin()+a.reg.size(), 1, modulus.reg.size()-a.reg.size());

	return result;
}

Integer ModularArithmetic::CascadeExponentiate(const Integer &x, const Integer &e1, const Integer &y, const Integer &e2) const
{
	if (modulus.IsOdd())
	{
		MontgomeryRepresentation dr(modulus);
		return dr.ConvertOut(dr.CascadeExponentiate(dr.ConvertIn(x), e1, dr.ConvertIn(y), e2));
	}
	else
		return AbstractRing<Integer>::CascadeExponentiate(x, e1, y, e2);
}

void ModularArithmetic::SimultaneousExponentiate(Integer *results, const Integer &base, const Integer *exponents, unsigned int exponentsCount) const
{
	if (modulus.IsOdd())
	{
		MontgomeryRepresentation dr(modulus);
		dr.SimultaneousExponentiate(results, dr.ConvertIn(base), exponents, exponentsCount);
		for (unsigned int i=0; i<exponentsCount; i++)
			results[i] = dr.ConvertOut(results[i]);
	}
	else
		AbstractRing<Integer>::SimultaneousExponentiate(results, base, exponents, exponentsCount);
}

MontgomeryRepresentation::MontgomeryRepresentation(const Integer &m)	// modulus must be odd
	: ModularArithmetic(m),
	  u((word)0, modulus.reg.size()),
	  workspace(5*modulus.reg.size())
{
	if (!modulus.IsOdd())
		throw InvalidArgument("MontgomeryRepresentation: Montgomery representation requires an odd modulus");

	RecursiveInverseModPower2(u.reg, workspace, modulus.reg, modulus.reg.size());
}

const Integer& MontgomeryRepresentation::Multiply(const Integer &a, const Integer &b) const
{
	word *const T = workspace.begin();
	word *const R = result.reg.begin();
	const unsigned int N = modulus.reg.size();
	assert(a.reg.size()<=N && b.reg.size()<=N);

	AsymmetricMultiply(T, T+2*N, a.reg, a.reg.size(), b.reg, b.reg.size());
	SetWords(T+a.reg.size()+b.reg.size(), 0, 2*N-a.reg.size()-b.reg.size());
	MontgomeryReduce(R, T+2*N, T, modulus.reg, u.reg, N);
	return result;
}

const Integer& MontgomeryRepresentation::Square(const Integer &a) const
{
	word *const T = workspace.begin();
	word *const R = result.reg.begin();
	const unsigned int N = modulus.reg.size();
	assert(a.reg.size()<=N);

	CryptoPP::Square(T, T+2*N, a.reg, a.reg.size());
	SetWords(T+2*a.reg.size(), 0, 2*N-2*a.reg.size());
	MontgomeryReduce(R, T+2*N, T, modulus.reg, u.reg, N);
	return result;
}

Integer MontgomeryRepresentation::ConvertOut(const Integer &a) const
{
	word *const T = workspace.begin();
	word *const R = result.reg.begin();
	const unsigned int N = modulus.reg.size();
	assert(a.reg.size()<=N);

	CopyWords(T, a.reg, a.reg.size());
	SetWords(T+a.reg.size(), 0, 2*N-a.reg.size());
	MontgomeryReduce(R, T+2*N, T, modulus.reg, u.reg, N);
	return result;
}

const Integer& MontgomeryRepresentation::MultiplicativeInverse(const Integer &a) const
{
//	  return (EuclideanMultiplicativeInverse(a, modulus)<<(2*WORD_BITS*modulus.reg.size()))%modulus;
	word *const T = workspace.begin();
	word *const R = result.reg.begin();
	const unsigned int N = modulus.reg.size();
	assert(a.reg.size()<=N);

	CopyWords(T, a.reg, a.reg.size());
	SetWords(T+a.reg.size(), 0, 2*N-a.reg.size());
	MontgomeryReduce(R, T+2*N, T, modulus.reg, u.reg, N);
	unsigned k = AlmostInverse(R, T, R, N, modulus.reg, N);

//	cout << "k=" << k << " N*32=" << 32*N << endl;

	if (k>N*WORD_BITS)
		DivideByPower2Mod(R, R, k-N*WORD_BITS, modulus.reg, N);
	else
		MultiplyByPower2Mod(R, R, N*WORD_BITS-k, modulus.reg, N);

	return result;
}

template class AbstractRing<Integer>;    

NAMESPACE_END
