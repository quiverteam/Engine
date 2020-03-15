#include "ShaderConstantBufferDx11.h"
#include "shaderdevicedx11.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

CShaderConstantBufferDx11::CShaderConstantBufferDx11() :
	m_pCBuffer( NULL ),
	m_nBufSize( 0 ),
	m_bNeedsUpdate( false ),
	m_bDynamic( false ),
	m_pData( NULL )
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

void CShaderConstantBufferDx11::Update( void *pNewData )
{
	if ( !pNewData )
		return;

	// If this new data is not the same as the data
	// the GPU currently has, we need to update.
	if ( memcmp( m_pData, pNewData, m_nBufSize ) )
	{
		memcpy( m_pData, pNewData, m_nBufSize );
		m_bNeedsUpdate = true;

		UploadToGPU();
	}

}

void *CShaderConstantBufferDx11::GetData()
{
	return m_pData;
}

void CShaderConstantBufferDx11::ForceUpdate()
{
	m_bNeedsUpdate = true;
	UploadToGPU();
}

void CShaderConstantBufferDx11::UploadToGPU()
{
	if ( !m_pCBuffer || !m_bNeedsUpdate )
		return;

	if ( m_bDynamic )
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		HRESULT hr = D3D11DeviceContext()->Map( m_pCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
		if ( FAILED( hr ) )
		{
			return;
		}
		memcpy( mapped.pData, m_pData, m_nBufSize );
		D3D11DeviceContext()->Unmap( m_pCBuffer, 0 );
	}
	else
	{
		D3D11DeviceContext()->UpdateSubresource( m_pCBuffer, 0, 0, m_pData, 0, 0 );
	}

	m_bNeedsUpdate = false;
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