#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <xmmintrin.h>
#include <nmmintrin.h>
#include <stdint.h>

#ifndef MAX_DX11_STREAMS
#define MAX_DX11_STREAMS 16
#endif

#ifndef MAX_DX11_SAMPLERS
#define MAX_DX11_SAMPLERS 16
#endif

extern ID3D11Device *g_pD3DDevice;
extern ID3D11DeviceContext *g_pD3DDeviceContext;
extern IDXGISwapChain *g_pD3DSwapChain;

//-----------------------------------------------------------------------------
// Utility methods
//-----------------------------------------------------------------------------
inline ID3D11Device *D3D11Device()
{
	return g_pD3DDevice;
}

inline ID3D11DeviceContext *D3D11DeviceContext()
{
	return g_pD3DDeviceContext;
}

inline IDXGISwapChain *D3D11SwapChain()
{
	return g_pD3DSwapChain;
}

FORCEINLINE static void memcpy_SSE( void *dest, const void *src, size_t count )
{
	__m128i *srcPtr = ( __m128i * )src;
	__m128i *destPtr = ( __m128i * )dest;

	unsigned int index = 0;
	while ( count )
	{

		__m128i x = _mm_load_si128( &srcPtr[index] );
		_mm_stream_si128( &destPtr[index], x );

		count -= 16;
		index++;
	}
}

#if 0
FORCEINLINE static long FastMemCompare( const void *cmp1, const void *cmp2, unsigned long length )
{
	if ( length >= 4 )
	{
		long difference = *(unsigned long *)cmp1 - *(unsigned long *)cmp2;
		if ( difference )
			return difference;
	}

	return memcmp( cmp1, cmp2, length );
}
#endif

#define FastMemCompare memcmp