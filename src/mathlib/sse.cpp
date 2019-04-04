//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: SSE Math primitives.
//
//=====================================================================================//

#include <math.h>
#include <float.h>	// Needed for FLT_EPSILON
#include "basetypes.h"
#include <memory.h>
#include "tier0/dbg.h"
#include "mathlib/mathlib.h"
#include "mathlib/vector.h"
#include "sse.h"

#include "platform_defs.h"

#if defined(_WIN32) || defined(WIN64)
#include <intrin.h>
#elif defined(LINUX64) || defined(POSIX)
#include <xmmintrin.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const uint32 _sincos_masks[]	  = { (uint32)0x0,  (uint32)~0x0 };
static const uint32 _sincos_inv_masks[] = { (uint32)~0x0, (uint32)0x0 };

//-----------------------------------------------------------------------------
// Macros and constants required by some of the SSE assembly:
//-----------------------------------------------------------------------------

#ifdef _WIN32
	#define _PS_EXTERN_CONST(Name, Val) \
		const __declspec(align(16)) float _ps_##Name[4] = { Val, Val, Val, Val }

	#define _PS_EXTERN_CONST_TYPE(Name, Type, Val) \
		const __declspec(align(16)) Type _ps_##Name[4] = { Val, Val, Val, Val }; \

	#define _EPI32_CONST(Name, Val) \
		static const __declspec(align(16)) __int32 _epi32_##Name[4] = { Val, Val, Val, Val }

	#define _PS_CONST(Name, Val) \
		static const __declspec(align(16)) float _ps_##Name[4] = { Val, Val, Val, Val }
#elif POSIX
	#define _PS_EXTERN_CONST(Name, Val) \
		const float _ps_##Name[4] __attribute__((aligned(16))) = { Val, Val, Val, Val }

	#define _PS_EXTERN_CONST_TYPE(Name, Type, Val) \
		const Type _ps_##Name[4]  __attribute__((aligned(16))) = { Val, Val, Val, Val }; \

	#define _EPI32_CONST(Name, Val) \
		static const int32 _epi32_##Name[4]  __attribute__((aligned(16))) = { Val, Val, Val, Val }

	#define _PS_CONST(Name, Val) \
		static const float _ps_##Name[4]  __attribute__((aligned(16))) = { Val, Val, Val, Val }
#endif

_PS_EXTERN_CONST(am_0, 0.0f);
_PS_EXTERN_CONST(am_1, 1.0f);
_PS_EXTERN_CONST(am_m1, -1.0f);
_PS_EXTERN_CONST(am_0p5, 0.5f);
_PS_EXTERN_CONST(am_1p5, 1.5f);
_PS_EXTERN_CONST(am_pi, (float)M_PI);
_PS_EXTERN_CONST(am_pi_o_2, (float)(M_PI / 2.0));
_PS_EXTERN_CONST(am_2_o_pi, (float)(2.0 / M_PI));
_PS_EXTERN_CONST(am_pi_o_4, (float)(M_PI / 4.0));
_PS_EXTERN_CONST(am_4_o_pi, (float)(4.0 / M_PI));
_PS_EXTERN_CONST_TYPE(am_sign_mask, int32, 0x80000000);
_PS_EXTERN_CONST_TYPE(am_inv_sign_mask, int32, ~0x80000000);
_PS_EXTERN_CONST_TYPE(am_min_norm_pos,int32, 0x00800000);
_PS_EXTERN_CONST_TYPE(am_mant_mask, int32, 0x7f800000);
_PS_EXTERN_CONST_TYPE(am_inv_mant_mask, int32, ~0x7f800000);

_EPI32_CONST(1, 1);
_EPI32_CONST(2, 2);

_PS_CONST(sincos_p0, 0.15707963267948963959e1f);
_PS_CONST(sincos_p1, -0.64596409750621907082e0f);
_PS_CONST(sincos_p2, 0.7969262624561800806e-1f);
_PS_CONST(sincos_p3, -0.468175413106023168e-2f);

#ifdef PFN_VECTORMA
void  __cdecl _SSE_VectorMA( const float *start, float scale, const float *direction, float *dest );
#endif

//-----------------------------------------------------------------------------
// SSE implementations of optimized routines:
//-----------------------------------------------------------------------------
float _SSE_Sqrt(float x)
{
	Assert(s_bMathlibInitialized);
	float	root = 0.f;
	__m128 _x = _mm_set_ps1(x);
	_mm_store_ss(&root, _mm_rcp_ss(_mm_rsqrt_ss(_x)));
	return root;
}

// Single iteration NewtonRaphson reciprocal square root:
// 0.5 * rsqrtps * (3 - x * rsqrtps(x) * rsqrtps(x)) 	
// Very low error, and fine to use in place of 1.f / sqrtf(x).	
#if 0
float _SSE_RSqrtAccurate(float x)
{
	Assert( s_bMathlibInitialized );

	float rroot;
	_asm
	{
		rsqrtss	xmm0, x
		movss	rroot, xmm0
	}

	return (0.5f * rroot) * (3.f - (x * rroot) * rroot);
}
#else

const __m128	f1 = _mm_set_ss(1.0f);
const __m128	f3  = _mm_set_ss(3.0f);  // 3 as SSE value
const __m128	f05 = _mm_set_ss(0.5f);  // 0.5 as SSE value
const __m128	f0 = _mm_set_ss(0.0f);
const __m128	fac2 = _mm_set_ss(2.0f);
const __m128	fac3 = _mm_set_ss(6.0f);
const __m128	fac4 = _mm_set_ss(24.0f);
const __m128	fac5 = _mm_set_ss(120.0f);
const __m128	fac6 = _mm_set_ss(720.0f);
const __m128	fac7 = _mm_set_ss(5040.0f);
const __m128	fac8 = _mm_set_ss(40320.0f);
const __m128	fac9 = _mm_set_ss(362880.0f);

// Intel / Kipps SSE RSqrt.  Significantly faster than above.
float _SSE_RSqrtAccurate(float a)
{

	__m128  xx = _mm_load_ss( &a );
    __m128  xr = _mm_rsqrt_ss( xx );
    __m128  xt;
	
    xt = _mm_mul_ss( xr, xr );
    xt = _mm_mul_ss( xt, xx );
    xt = _mm_sub_ss( f3, xt );
    xt = _mm_mul_ss( xt, f05 );
    xr = _mm_mul_ss( xr, xt );
	
    _mm_store_ss( &a, xr );
    return a;

}
#endif

// Simple SSE rsqrt.  Usually accurate to around 6 (relative) decimal places 
// or so, so ok for closed transforms.  (ie, computing lighting normals)
float _SSE_RSqrtFast(float x)
{
	Assert( s_bMathlibInitialized );

	float rroot;

	__m128 _x = _mm_set1_ps(x);
	_mm_store_ps(&rroot, _mm_rsqrt_ss(_x));

	return rroot;
}

float FASTCALL _SSE_VectorNormalize (Vector& vec)
{
	Assert( s_bMathlibInitialized );

	// NOTE: This is necessary to prevent an memory overwrite...
	// sice vec only has 3 floats, we can't "movaps" directly into it.
#if defined(_WIN32) || defined(WIN64)
	__declspec(align(16)) float result[4];
#elif POSIX
	 float result[4] __attribute__((aligned(16)));
#endif

	float *v = &vec[0];
	float *r = &result[0];

	float	radius = 0.f;
	// Blah, get rid of these comparisons ... in reality, if you have all 3 as zero, it shouldn't 
	// be much of a performance win, considering you will very likely miss 3 branch predicts in a row.
	if ( v[0] || v[1] || v[2] )
	{
		/* Compiles to slightly less instructions than what was initially implemented. */
		/* Works on both windows and posix */

		// Load vector
	#ifdef ALIGNED_VECTOR
		__m128 _v = _mm_load_ps(v);
	#else
		__m128 _v = _mm_loadu_ps(v);
	#endif

		// Compute square
		__m128 res = _mm_mul_ps(_v, _v);
		
		// Shuffle
		__m128 v2 = _mm_shuffle_ps(res, res, 1);
		__m128 v3 = _mm_shuffle_ps(res, res, 2);
		
		// Adda all together & store
		res = _mm_add_ps(res, _mm_add_ps(v3, v2));

		// Sqrt and store mag in res
		res = _mm_sqrt_ps(res);

		// Need to duplicate to all elems
		res = _mm_shuffle_ps(res, res, 0);

		// Compute components again
		_v = _mm_div_ps(_v, res);

		// Store in result & set vec
		_mm_store_ps(r, _v);
		vec.x = r[0];
		vec.y = r[1];
		vec.z = r[2];

		float ret;
		_mm_store_ss(&ret, res);
		return ret;
	}

	return radius;
}

void FASTCALL _SSE_VectorNormalizeFast (Vector& vec)
{
	float ool = _SSE_RSqrtAccurate( FLT_EPSILON + vec.x * vec.x + vec.y * vec.y + vec.z * vec.z );

	vec.x *= ool;
	vec.y *= ool;
	vec.z *= ool;
}

float _SSE_InvRSquared(const float* v)
{
	float	inv_r2 = 1.f;

	/*

	res = (vx+vx) + (vy+vy) + (vz*vz);
	return 1 / max(1.0, res);
	
	*/

#ifdef ALIGNED_VECTOR
	__m128 _v = _mm_load_ps(v);
#else
	__m128 _v = _mm_loadu_ps(v);
#endif
	__m128 res = _mm_mul_ps(_v, _v);
	__m128 x = res;
	__m128 y = _mm_shuffle_ps(res, res, 1);
	__m128 z = _mm_shuffle_ps(res, res, 2);

	res = _mm_add_ps(x, _mm_add_ps(y, z));

	res = _mm_rcp_ps(_mm_max_ps(f0, res));

	_mm_store_ss(&inv_r2, res);

	return inv_r2;
}


#ifdef POSIX
// #define _PS_CONST(Name, Val) static const ALIGN16 float _ps_##Name[4] ALIGN16_POST = { Val, Val, Val, Val }
#define _PS_CONST_TYPE(Name, Type, Val) static const ALIGN16 Type _ps_##Name[4] ALIGN16_POST = { Val, Val, Val, Val }

_PS_CONST_TYPE(sign_mask, int, 0x80000000);
_PS_CONST_TYPE(inv_sign_mask, int, ~0x80000000);


#define _PI32_CONST(Name, Val)  static const ALIGN16 int _pi32_##Name[4]  ALIGN16_POST = { Val, Val, Val, Val }

_PI32_CONST(1, 1);
_PI32_CONST(inv1, ~1);
_PI32_CONST(2, 2);
_PI32_CONST(4, 4);
_PI32_CONST(0x7f, 0x7f);
_PS_CONST(1  , 1.0f);
_PS_CONST(0p5, 0.5f);

_PS_CONST(minus_cephes_DP1, -0.78515625);
_PS_CONST(minus_cephes_DP2, -2.4187564849853515625e-4);
_PS_CONST(minus_cephes_DP3, -3.77489497744594108e-8);
_PS_CONST(sincof_p0, -1.9515295891E-4);
_PS_CONST(sincof_p1,  8.3321608736E-3);
_PS_CONST(sincof_p2, -1.6666654611E-1);
_PS_CONST(coscof_p0,  2.443315711809948E-005);
_PS_CONST(coscof_p1, -1.388731625493765E-003);
_PS_CONST(coscof_p2,  4.166664568298827E-002);
_PS_CONST(cephes_FOPI, 1.27323954473516); // 4 / M_PI

typedef union xmm_mm_union {
	__m128 xmm;
	__m64 mm[2];
} xmm_mm_union;

#define COPY_MM_TO_XMM(mm0_, mm1_, xmm_) { xmm_mm_union u; u.mm[0]=mm0_; u.mm[1]=mm1_; xmm_ = u.xmm; }

typedef __m128 v4sf;  // vector of 4 float (sse1)
typedef __m64 v2si;   // vector of 2 int (mmx)

#endif

/* Avoid this, standard library is faster */
void _SSE_SinCos(float x, float* s, float* c)
{
	// Taylor series approximation of sin & cos
	__m128 val = _mm_set1_ps(x);

	__m128 _s = val;
	const __m128 squared = _mm_mul_ss(_s, _s);

	__m128 phase1 = val;
	phase1 = _mm_mul_ps(phase1, squared); // x = x^3
	_s = _mm_sub_ss(_s, _mm_div_ss(phase1, fac3));

	phase1 = _mm_mul_ps(phase1, squared); // x = x^5
	_s = _mm_add_ss(_s, _mm_div_ss(phase1, fac5));

	phase1 = _mm_mul_ps(phase1, squared); // x = x^7
	_s = _mm_sub_ss(_s, _mm_div_ss(phase1, fac7));

	phase1 = _mm_mul_ps(phase1, squared); // x = x^9
	_s = _mm_add_ss(_s, _mm_div_ss(phase1, fac9));

	// Done with sine
	_mm_store_ss(s, _s);

	//cos x = sqrt(1-sin^2 x)
	_s = _mm_mul_ss(_s, _s); // square
	_s = _mm_sub_ss(f1, _s);
	_s = _mm_rsqrt_ss(_s);
	_s = _mm_rcp_ss(_s);

	// Done with cosine
	_mm_store_ss(c, _s);
}

float _SSE_cos( float x )
{
	// Taylor series implementation of approximation of cos
	__m128 val = _mm_set1_ps(x);

	__m128 _s = _mm_set_ss(1.0f);
	const __m128 squared = _mm_mul_ss(val, val);

	__m128 r1 = val;
	r1 = _mm_mul_ss(r1, r1);
	_s = _mm_sub_ss(_s, _mm_div_ss(r1, fac2));

	r1 = _mm_mul_ss(r1, squared);
	_s = _mm_add_ss(_s, _mm_div_ss(r1, fac4));

	r1 = _mm_mul_ss(r1, squared);
	_s = _mm_sub_ss(_s, _mm_div_ss(r1, fac6));

	r1 = _mm_mul_ss(r1, squared);
	_s = _mm_add_ss(_s, _mm_div_ss(r1, fac8));

	float ret = 0.0f;
	_mm_store_ss(&ret, _s);
	return ret;
}

//-----------------------------------------------------------------------------
// SSE2 implementations of optimized routines:
//-----------------------------------------------------------------------------
#if defined(_WIN32) || defined(WIN64)
// Identical to SSE SinCos, no benefit from using SSE2 instructions
void _SSE2_SinCos(float x, float* s, float* c)  // any x
{
	// Taylor series approximation of sin & cos
	__m128 val = _mm_set1_ps(x);

	__m128 _s = val;
	const __m128 squared = _mm_mul_ss(_s, _s);

	__m128 phase1 = val;
	phase1 = _mm_mul_ps(phase1, squared); // x = x^3
	_s = _mm_sub_ss(_s, _mm_div_ss(phase1, fac3));

	phase1 = _mm_mul_ps(phase1, squared); // x = x^5
	_s = _mm_add_ss(_s, _mm_div_ss(phase1, fac5));

	phase1 = _mm_mul_ps(phase1, squared); // x = x^7
	_s = _mm_sub_ss(_s, _mm_div_ss(phase1, fac7));

	phase1 = _mm_mul_ps(phase1, squared); // x = x^9
	_s = _mm_add_ss(_s, _mm_div_ss(phase1, fac9));

	// Done with sine
	_mm_store_ss(s, _s);

	//cos x = sqrt(1-sin^2 x)
	_s = _mm_mul_ss(_s, _s); // square
	_s = _mm_sub_ss(f1, _s);
	_s = _mm_rsqrt_ss(_s);
	_s = _mm_rcp_ss(_s);

	// Done with cosine
	_mm_store_ss(c, _s);
}

float _SSE2_cos(float x)  
{
	// Taylor series implementation of approximation of cos
	__m128 val = _mm_set1_ps(x);

	__m128 _s = _mm_set_ss(1.0f);
	const __m128 squared = _mm_mul_ss(val, val);

	__m128 r1 = val;
	r1 = _mm_mul_ss(r1, r1);
	_s = _mm_sub_ss(_s, _mm_div_ss(r1, fac2));

	r1 = _mm_mul_ss(r1, squared);
	_s = _mm_add_ss(_s, _mm_div_ss(r1, fac4));

	r1 = _mm_mul_ss(r1, squared);
	_s = _mm_sub_ss(_s, _mm_div_ss(r1, fac6));

	r1 = _mm_mul_ss(r1, squared);
	_s = _mm_add_ss(_s, _mm_div_ss(r1, fac8));

	float ret = 0.0f;
	_mm_store_ss(&ret, _s);
	return ret;
}
#endif

#if 0
// SSE Version of VectorTransform
void VectorTransformSSE(const float *in1, const matrix3x4_t& in2, float *out1)
{
	Assert( s_bMathlibInitialized );
	Assert( in1 != out1 );

#ifdef _WIN32
	__asm
	{
		mov eax, in1;
		mov ecx, in2;
		mov edx, out1;

		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
		addss xmm0, [ecx+12]
 		movss [edx], xmm0;
		add ecx, 16;

		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
		addss xmm0, [ecx+12]
		movss [edx+4], xmm0;
		add ecx, 16;

		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
		addss xmm0, [ecx+12]
		movss [edx+8], xmm0;
	}
#elif POSIX
	#warning "VectorTransformSSE C implementation only"
		out1[0] = DotProduct(in1, in2[0]) + in2[0][3];
		out1[1] = DotProduct(in1, in2[1]) + in2[1][3];
		out1[2] = DotProduct(in1, in2[2]) + in2[2][3];
#else
	#error "Not Implemented"
#endif
}
#endif

#if 0
void VectorRotateSSE( const float *in1, const matrix3x4_t& in2, float *out1 )
{
	Assert( s_bMathlibInitialized );
	Assert( in1 != out1 );

#ifdef _WIN32
	__asm
	{
		mov eax, in1;
		mov ecx, in2;
		mov edx, out1;

		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
 		movss [edx], xmm0;
		add ecx, 16;

		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
		movss [edx+4], xmm0;
		add ecx, 16;

		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
		movss [edx+8], xmm0;
	}
#elif POSIX
	#warning "VectorRotateSSE C implementation only"
		out1[0] = DotProduct( in1, in2[0] );
		out1[1] = DotProduct( in1, in2[1] );
		out1[2] = DotProduct( in1, in2[2] );
#else
	#error "Not Implemented"
#endif
}
#endif

#ifdef _WIN32
void VNAKED _SSE_VectorMA( const float *start, float scale, const float *direction, float *dest )
{
#if 0
	// FIXME: This don't work!! It will overwrite memory in the write to dest
	Assert(0);

	Assert( s_bMathlibInitialized );
	_asm {  // Intel SSE only routine
		mov	eax, DWORD PTR [esp+0x04]	; *start, s0..s2
		mov ecx, DWORD PTR [esp+0x0c]	; *direction, d0..d2
		mov edx, DWORD PTR [esp+0x10]	; *dest
		movss	xmm2, [esp+0x08]		; x2 = scale, 0, 0, 0
#ifdef ALIGNED_VECTOR
		movaps	xmm3, [ecx]				; x3 = dir0,dir1,dir2,X
		pshufd	xmm2, xmm2, 0			; x2 = scale, scale, scale, scale
		movaps	xmm1, [eax]				; x1 = start1, start2, start3, X
		mulps	xmm3, xmm2				; x3 *= x2
		addps	xmm3, xmm1				; x3 += x1
		movaps	[edx], xmm3				; *dest = x3
#else
		movups	xmm3, [ecx]				; x3 = dir0,dir1,dir2,X
		pshufd	xmm2, xmm2, 0			; x2 = scale, scale, scale, scale
		movups	xmm1, [eax]				; x1 = start1, start2, start3, X
		mulps	xmm3, xmm2				; x3 *= x2
		addps	xmm3, xmm1				; x3 += x1
		movups	[edx], xmm3				; *dest = x3
#endif
	}
#endif
}
#endif

#ifdef _WIN32
#ifdef PFN_VECTORMA
void VNAKED __cdecl _SSE_VectorMA( const Vector &start, float scale, const Vector &direction, Vector &dest )
{
#if 0
	// FIXME: This don't work!! It will overwrite memory in the write to dest
	Assert(0);

	Assert( s_bMathlibInitialized );
	_asm 
	{  
		// Intel SSE only routine
		mov	eax, DWORD PTR [esp+0x04]	; *start, s0..s2
		mov ecx, DWORD PTR [esp+0x0c]	; *direction, d0..d2
		mov edx, DWORD PTR [esp+0x10]	; *dest
		movss	xmm2, [esp+0x08]		; x2 = scale, 0, 0, 0
#ifdef ALIGNED_VECTOR
		movaps	xmm3, [ecx]				; x3 = dir0,dir1,dir2,X
		pshufd	xmm2, xmm2, 0			; x2 = scale, scale, scale, scale
		movaps	xmm1, [eax]				; x1 = start1, start2, start3, X
		mulps	xmm3, xmm2				; x3 *= x2
		addps	xmm3, xmm1				; x3 += x1
		movaps	[edx], xmm3				; *dest = x3
#else
		movups	xmm3, [ecx]				; x3 = dir0,dir1,dir2,X
		pshufd	xmm2, xmm2, 0			; x2 = scale, scale, scale, scale
		movups	xmm1, [eax]				; x1 = start1, start2, start3, X
		mulps	xmm3, xmm2				; x3 *= x2
		addps	xmm3, xmm1				; x3 += x1
		movups	[edx], xmm3				; *dest = x3
#endif
	}
#endif
}
float (__cdecl *pfVectorMA)(Vector& v) = _VectorMA;
#endif
#endif


// SSE DotProduct -- it's a smidgen faster than the asm DotProduct...
//   Should be validated too!  :)
//   NJS: (Nov 1 2002) -NOT- faster.  may time a couple cycles faster in a single function like 
//   this, but when inlined, and instruction scheduled, the C version is faster.  
//   Verified this via VTune
/*
vec_t DotProduct (const vec_t *a, const vec_t *c)
{
	vec_t temp;

	__asm
	{
		mov eax, a;
		mov ecx, c;
		mov edx, DWORD PTR [temp]
		movss xmm0, [eax];
		mulss xmm0, [ecx];
		movss xmm1, [eax+4];
		mulss xmm1, [ecx+4];
		movss xmm2, [eax+8];
		mulss xmm2, [ecx+8];
		addss xmm0, xmm1;
		addss xmm0, xmm2;
		movss [edx], xmm0;
		fld DWORD PTR [edx];
		ret
	}
}
*/