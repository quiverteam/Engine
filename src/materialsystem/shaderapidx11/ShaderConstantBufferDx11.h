#pragma once

#include "shaderapi/ishaderconstantbuffer.h"
#include "../shaderapidx9/locald3dtypes.h"
#include "Dx11Global.h"
#include "tier0/vprof.h"

class CShaderConstantBufferDx11 : public IShaderConstantBuffer
{
public:
	CShaderConstantBufferDx11();
	virtual void Create( size_t nBufferSize, bool bDynamic = true );
	virtual void Update( void *pNewData );
	virtual void Destroy();
	virtual bool NeedsUpdate() const;
	virtual void UploadToGPU();
	virtual ConstantBufferHandle_t GetBuffer() const;

	void *GetData();

	void *Lock();
	void Unlock();

	void ForceUpdate();

	ID3D11Buffer *GetD3DBuffer() const;

private:
	ID3D11Buffer* m_pCBuffer;
	size_t m_nBufSize;
	bool m_bNeedsUpdate;
	bool m_bDynamic;
	bool m_bLocked;
	void *m_pData;
};

FORCEINLINE ID3D11Buffer *CShaderConstantBufferDx11::GetD3DBuffer() const
{
	return m_pCBuffer;
}

FORCEINLINE bool CShaderConstantBufferDx11::NeedsUpdate() const
{
	return m_bNeedsUpdate;
}

FORCEINLINE ConstantBufferHandle_t CShaderConstantBufferDx11::GetBuffer() const
{
	return (ConstantBufferHandle_t)m_pCBuffer;
}

FORCEINLINE void *CShaderConstantBufferDx11::GetData()
{
	return m_pData;
}

FORCEINLINE void CShaderConstantBufferDx11::ForceUpdate()
{
	m_bNeedsUpdate = true;
	UploadToGPU();
}

FORCEINLINE void CShaderConstantBufferDx11::Update( void *pNewData )
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

FORCEINLINE void *CShaderConstantBufferDx11::Lock()
{
	if ( !m_bDynamic || m_bLocked || !m_pCBuffer )
	{
		return NULL;
	}

	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT hr = D3D11DeviceContext()->Map( m_pCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
	if ( FAILED( hr ) )
	{
		return NULL;
	}

	m_bLocked = true;

	return mapped.pData;
}

FORCEINLINE void CShaderConstantBufferDx11::Unlock()
{
	if ( !m_bDynamic || !m_bLocked || !m_pCBuffer )
	{
		return;
	}

	m_bLocked = false;

	D3D11DeviceContext()->Unmap( m_pCBuffer, 0 );
}

FORCEINLINE void CShaderConstantBufferDx11::UploadToGPU()
{
	VPROF_BUDGET( "CShaderConstantBufferDx11::UploadToGPU()", VPROF_BUDGETGROUP_OTHER_UNACCOUNTED );

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
		//{
			//VPROF_BUDGET( "CShaderConstantBufferDx11::memcpy_SSE", VPROF_BUDGETGROUP_OTHER_UNACCOUNTED );
			memcpy( mapped.pData, m_pData, m_nBufSize );
		//}
		D3D11DeviceContext()->Unmap( m_pCBuffer, 0 );
	}
	else
	{
		D3D11DeviceContext()->UpdateSubresource( m_pCBuffer, 0, 0, m_pData, 0, 0 );
	}

	m_bNeedsUpdate = false;
}