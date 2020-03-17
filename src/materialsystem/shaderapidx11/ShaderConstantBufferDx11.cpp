#include "ShaderConstantBufferDx11.h"
#include "Dx11Global.h"
#include "FastMemcpy.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

static FORCEINLINE void *memcpy_SSE_V2( void *pdst, const void *psrc, size_t size )
{
	const char *dst = (const char *)pdst;
	const char *src = (const char *)psrc;

	_mm_prefetch( src, _MM_HINT_NTA );

	for ( ; size >= 16; size -= 16 )
	{
		__m128i w0, w1, w2, w3, w4, w5, w6, w7;
		w0 = _mm_load_si128( ( ( const __m128i * )src ) + 0 );
		w1 = _mm_load_si128( ( ( const __m128i * )src ) + 1 );
		w2 = _mm_load_si128( ( ( const __m128i * )src ) + 2 );
		w3 = _mm_load_si128( ( ( const __m128i * )src ) + 3 );
		w4 = _mm_load_si128( ( ( const __m128i * )src ) + 4 );
		w5 = _mm_load_si128( ( ( const __m128i * )src ) + 5 );
		w6 = _mm_load_si128( ( ( const __m128i * )src ) + 6 );
		w7 = _mm_load_si128( ( ( const __m128i * )src ) + 7 );
		_mm_prefetch( ( src + 128 ), _MM_HINT_NTA );
		src += 16;
		_mm_stream_si128( ( ( ( __m128i * )dst ) + 0 ), w0 );
		_mm_stream_si128( ( ( ( __m128i * )dst ) + 1 ), w1 );
		_mm_stream_si128( ( ( ( __m128i * )dst ) + 2 ), w2 );
		_mm_stream_si128( ( ( ( __m128i * )dst ) + 3 ), w3 );
		_mm_stream_si128( ( ( ( __m128i * )dst ) + 4 ), w4 );
		_mm_stream_si128( ( ( ( __m128i * )dst ) + 5 ), w5 );
		_mm_stream_si128( ( ( ( __m128i * )dst ) + 6 ), w6 );
		_mm_stream_si128( ( ( ( __m128i * )dst ) + 7 ), w7 );
		dst += 16;
	}

	return pdst;
}

CShaderConstantBufferDx11::CShaderConstantBufferDx11() :
	m_pCBuffer( NULL ),
	m_nBufSize( 0 ),
	m_bNeedsUpdate( false ),
	m_bDynamic( false ),
	m_pData( NULL ),
	m_bLocked( false )
{
}

void CShaderConstantBufferDx11::Create( size_t nBufferSize, bool bDynamic )
{
	m_nBufSize = nBufferSize;
	m_bDynamic = bDynamic;

	//Log( "Creating constant buffer of size %u\n", nBufferSize );

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = m_nBufSize;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;
	if ( bDynamic )
	{
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.CPUAccessFlags = 0;
	}

	// Fill the buffer with all zeros
	m_pData = _aligned_malloc( nBufferSize, 16 );
	ZeroMemory( m_pData, nBufferSize );
	D3D11_SUBRESOURCE_DATA initialData;
	initialData.pSysMem = m_pData;
	initialData.SysMemPitch = 0;
	initialData.SysMemSlicePitch = 0;

	//Log( "Creating D3D constant buffer: size: %i\n", m_nBufSize );

	HRESULT hr = D3D11Device()->CreateBuffer( &cbDesc, &initialData, &m_pCBuffer );
	if ( FAILED( hr ) )
	{
		Warning( "Could not set constant buffer!" );
		//return NULL;
	}
}

void CShaderConstantBufferDx11::Destroy()
{
	if ( m_pCBuffer )
		m_pCBuffer->Release();
	
	m_pCBuffer = NULL;

	if ( m_pData )
		_aligned_free( m_pData );
	m_pData = NULL;
}