#include "ShaderConstantBufferDx11.h"
#include "shaderdevicedx11.h"

CShaderConstantBufferDx11::CShaderConstantBufferDx11() :
	m_pCBuffer( NULL ),
	m_nBufSize( 0 ),
	m_bNeedsUpdate( true ),
	m_pData( NULL )
{
}

void CShaderConstantBufferDx11::Create( size_t nBufferSize )
{
	m_nBufSize = nBufferSize;

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = m_nBufSize;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	HRESULT hr = D3D11Device()->CreateBuffer( &cbDesc, NULL, &m_pCBuffer );
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
	if ( !m_pData || memcmp( m_pData, pNewData, m_nBufSize ) )
	{
		if ( m_pData )
			free( m_pData );
		m_pData = malloc( m_nBufSize );
		memcpy( m_pData, pNewData, m_nBufSize );
		m_bNeedsUpdate = true;
	}
}

void CShaderConstantBufferDx11::UploadToGPU()
{
	if ( !m_pCBuffer || !m_bNeedsUpdate )
		return;

	D3D11DeviceContext()->UpdateSubresource( m_pCBuffer, 0, 0, m_pData, 0, 0 );
	m_bNeedsUpdate = false;
}

void CShaderConstantBufferDx11::Destroy()
{
	if ( m_pCBuffer )
		m_pCBuffer->Release();
	
	m_pCBuffer = NULL;

	if ( m_pData )
		free( m_pData );
	m_pData = NULL;
}