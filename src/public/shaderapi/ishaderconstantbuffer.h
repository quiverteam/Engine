#pragma once

#include "IShaderDevice.h"

class IShaderConstantBuffer
{
public:
	virtual void Create( size_t nBufferSize ) = 0;
	virtual void Update( void *pNewData ) = 0;
	virtual void Destroy() = 0;

	virtual ConstantBufferHandle_t GetBuffer() const = 0;

	// Do we need to update the data to the graphics card?
	virtual bool NeedsUpdate() const = 0;

	virtual void UploadToGPU() = 0;

	//virtual void Bind() = 0;
};