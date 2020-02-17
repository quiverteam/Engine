#include "ShaderConstantBufferDx11.h"
#include "shaderdevicedx11.h"

CShaderConstantBufferDx11::CShaderConstantBufferDx11() :
	m_pCBuffer( NULL ),
	m_nBufSize( 0 )
{
}

void CShaderConstantBufferDx11::Create( size_t nBufSize )
{
	m_nBufSize = nBufSize;

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = nBufSize;
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
	if ( !pNewData || !m_pCBuffer )
		return;

	D3D11DeviceContext()->UpdateSubresource( m_pCBuffer, 0, 0, pNewData, 0, 0 );
}

void CShaderConstantBufferDx11::Destroy()
{
	if ( !m_pCBuffer )
		return;

	m_pCBuffer->Release();
	m_pCBuffer = NULL;
}