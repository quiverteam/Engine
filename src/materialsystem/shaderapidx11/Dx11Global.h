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