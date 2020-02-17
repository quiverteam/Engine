#pragma once

class IShaderConstantBuffer
{
public:
	virtual void Create( size_t nBufSize ) = 0;
	virtual void Update( void* pNewData ) = 0;
	virtual void Destroy() = 0;
	//virtual void Bind() = 0;
};