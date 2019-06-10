//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef _SSE_H
#define _SSE_H

float _SSE_Sqrt(float x);
float _SSE_RSqrtAccurate(float a);
float _SSE_RSqrtFast(float x);
float FASTCALL _SSE_VectorNormalize(Vector& vec);
void FASTCALL _SSE_VectorNormalizeFast(Vector& vec);

/*
Inverse dot product
*/
float _SSE_InvRSquared(const float* v);
void _SSE_SinCos(float x, float* s, float* c);
float _SSE_cos( float x);
void _SSE2_SinCos(float x, float* s, float* c);
float _SSE2_cos(float x); 
#if 0
void VectorTransformSSE(const float *in1, const matrix3x4_t& in2, float *out1);
void VectorRotateSSE( const float *in1, const matrix3x4_t& in2, float *out1 );
#endif

#include <xmmintrin.h>

namespace SSEConst
{
	static const __m128	fac2 = _mm_set_ps1(2.0f);
	static const __m128	fac3 = _mm_set_ps1(6.0f);
	static const __m128	fac4 = _mm_set_ps1(24.0f);
	static const __m128	fac5 = _mm_set_ps1(120.0f);
	static const __m128	fac6 = _mm_set_ps1(720.0f);
	static const __m128	fac7 = _mm_set_ps1(5040.0f);
	static const __m128	fac8 = _mm_set_ps1(40320.0f);
	static const __m128	fac9 = _mm_set_ps1(362880.0f);
	static const __m128	inv_fac2 = _mm_set_ps1(1.0f/2.0f);
	static const __m128	inv_fac3 = _mm_set_ps1(1.0f/6.0f);
	static const __m128	inv_fac4 = _mm_set_ps1(1.0f/24.0f);
	static const __m128	inv_fac5 = _mm_set_ps1(1.0f/120.0f);
	static const __m128	inv_fac6 = _mm_set_ps1(1.0f/720.0f);
	static const __m128	inv_fac7 = _mm_set_ps1(1.0f/5040.0f);
	static const __m128	inv_fac8 = _mm_set_ps1(1.0f/40320.0f);
	static const __m128	inv_fac9 = _mm_set_ps1(1.0/362880.0f);
	static const __m128 f1 = _mm_set_ps1(1.0f);
	static const __m128 f2 = _mm_set_ps1(2.0f);
	static const __m128 pi = _mm_set_ps1(3.14159265359f);
	static const __m128 e = _mm_set_ps1(2.718281828459f);
}

#endif // _SSE_H
