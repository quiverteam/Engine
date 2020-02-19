#pragma once

#include "shaderapi/ishaderconstantbuffer.h"
#include "../shaderapidx9/locald3dtypes.h"

class CShaderConstantBufferDx11 : public IShaderConstantBuffer
{
public:
	CShaderConstantBufferDx11();
	virtual void Create( size_t nBufSize );
	virtual void Update( void* pNewData );
	virtual void Destroy();
	//virtual void Bind();

	ID3D11Buffer *GetBuffer() const;

private:
	ID3D11Buffer* m_pCBuffer;
	size_t m_nBufSize;
};

FORCEINLINE ID3D11Buffer *CShaderConstantBufferDx11::GetBuffer() const
{
	return m_pCBuffer;
}