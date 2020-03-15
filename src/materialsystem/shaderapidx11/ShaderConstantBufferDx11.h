#pragma once

#include "shaderapi/ishaderconstantbuffer.h"
#include "../shaderapidx9/locald3dtypes.h"

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

	void ForceUpdate();

	ID3D11Buffer *GetD3DBuffer() const;

private:
	ID3D11Buffer* m_pCBuffer;
	size_t m_nBufSize;
	bool m_bNeedsUpdate;
	bool m_bDynamic;
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