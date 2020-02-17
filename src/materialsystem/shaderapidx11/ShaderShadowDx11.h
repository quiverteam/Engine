//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef SHADERSHADOWDX11_H
#define SHADERSHADOWDX11_H

#ifdef _WIN32
#pragma once
#endif

#include "shaderapi/ishadershadow.h"
#include "../shaderapidx9/locald3dtypes.h"
#include "shaderapidx11.h"

enum
{
	MAX_TEXTURES = 16,
	MAX_SAMPLERS = 16,
};

//
// Common constant buffers
//

ALIGN16 struct TransformBuffer_t
{
	DirectX::XMFLOAT4X4 modelTransform;
	DirectX::XMFLOAT4X4 viewTransform;
	DirectX::XMFLOAT4X4 projTransform;
};

ALIGN16 struct LightingBuffer_t
{
	DirectX::XMFLOAT4X4 lightData[MAX_NUM_LIGHTS];
	DirectX::XMFLOAT4X4 lightData2[MAX_NUM_LIGHTS];
	int lightTypes[MAX_NUM_LIGHTS];
	int numLights;
	DirectX::XMFLOAT3 ambientCube[6];	
};

struct SamplerShadowStateDx11_t
{
	bool m_TextureEnable : 1;
};

// DX11 fixed function state
struct ShadowStateDx11_t
{
	// Depth buffering state
	ShaderDepthFunc_t m_ZFunc;
	bool m_ZEnable;

	// Write enable
	uint64 m_ColorWriteEnable;

	// Fill mode
	ShaderFillMode_t m_FillMode;

	// Alpha state
	ShaderBlendFactor_t m_SrcBlend;
	ShaderBlendFactor_t m_DestBlend;

	// Separate alpha blend state
	ShaderBlendFactor_t m_SrcBlendAlpha;
	ShaderBlendFactor_t m_DestBlendAlpha;

	SamplerShadowStateDx11_t m_Samplers[MAX_SAMPLERS];

	StencilComparisonFunction_t m_AlphaFunc;
	int m_AlphaRef;

	bool	m_ZWriteEnable : 1;
	bool	m_ZBias : 2;
	bool	m_CullEnable : 1;
	bool	m_AlphaBlendEnable : 1;
	bool	m_SeparateAlphaBlendEnable : 1;
	bool	m_StencilEnable : 1;
	bool	m_EnableAlphaToCoverage : 1;
	bool	m_VertexBlendEnable : 1;

	unsigned char m_Reserved[4];
};

// DX11 shader (non-fixed function) state
struct ShadowShaderStateDx11_t
{
	// Which constant buffers does the shader use?
	ConstantBufferHandle_t m_CBuffers[MAX_DX11_CBUFFERS];
	int m_nCBuffers;

	// The vertex + pixel shader group to use...
	VertexShader_t m_VertexShader;
	PixelShader_t  m_PixelShader;
	
	// The static vertex + pixel shader indices
	int	m_nStaticVshIndex;
	int	m_nStaticPshIndex;

	// Vertex data used by this snapshot
	// Note that the vertex format actually used will be the
	// aggregate of the vertex formats used by all snapshots in a material
	VertexFormat_t m_VertexUsage;

	// Morph data used by this snapshot
	// Note that the morph format actually used will be the
	// aggregate of the morph formats used by all snapshots in a material
	MorphFormat_t m_MorphUsage;

	// Modulate constant color into the vertex color
	bool m_ModulateConstantColor;

	// These are in the shader state because
	// in Dx11, these options are no longer fixed function
	// and must be done in the shader.
	bool m_AlphaTestEnable;
	bool m_Translucent;
	
	bool m_UseVertexAndPixelShaders;

	//int m_nReserved[3];
};

struct ShadowStateCacheEntryDx11_t
{
	ShadowStateDx11_t m_State;
	ShadowShaderStateDx11_t m_ShaderState;
};

//-----------------------------------------------------------------------------
// The empty shader shadow
//-----------------------------------------------------------------------------
class CShaderShadowDx11 : public IShaderShadow
{
public:
	CShaderShadowDx11();
	virtual ~CShaderShadowDx11();

	// Sets the default *shadow* state
	void SetDefaultState();

	// Methods related to depth buffering
	void DepthFunc( ShaderDepthFunc_t depthFunc );
	void EnableDepthWrites( bool bEnable );
	void EnableDepthTest( bool bEnable );
	void EnablePolyOffset( PolygonOffsetMode_t nOffsetMode );

	// Suppresses/activates color writing 
	void EnableColorWrites( bool bEnable );
	void EnableAlphaWrites( bool bEnable );

	// Methods related to alpha blending
	void EnableBlending( bool bEnable );
	void BlendFunc( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor );

	// Alpha testing
	void EnableAlphaTest( bool bEnable );
	void AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef /* [0-1] */ );

	// Wireframe/filled polygons
	void PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode );

	// Back face culling
	void EnableCulling( bool bEnable );

	// constant color + transparency
	void EnableConstantColor( bool bEnable );

	// Indicates the vertex format for use with a vertex shader
	// The flags to pass in here come from the VertexFormatFlags_t enum
	// If pTexCoordDimensions is *not* specified, we assume all coordinates
	// are 2-dimensional
	void VertexShaderVertexFormat( unsigned int flags, 
		int numTexCoords, int* pTexCoordDimensions,
		int userDataSize );

	// Indicates we're going to light the model
	void EnableLighting( bool bEnable );
	void EnableSpecular( bool bEnable );

	// vertex blending
	void EnableVertexBlend( bool bEnable );

	// per texture unit stuff
	void OverbrightValue( TextureStage_t stage, float value );
	void EnableTexture( Sampler_t stage, bool bEnable );
	void EnableTexGen( TextureStage_t stage, bool bEnable );
	void TexGen( TextureStage_t stage, ShaderTexGenParam_t param );

	// alternate method of specifying per-texture unit stuff, more flexible and more complicated
	// Can be used to specify different operation per channel (alpha/color)...
	void EnableCustomPixelPipe( bool bEnable );
	void CustomTextureStages( int stageCount );
	void CustomTextureOperation( TextureStage_t stage, ShaderTexChannel_t channel, 
		ShaderTexOp_t op, ShaderTexArg_t arg1, ShaderTexArg_t arg2 );

	// indicates what per-vertex data we're providing
	void DrawFlags( unsigned int drawFlags );

	// A simpler method of dealing with alpha modulation
	void EnableAlphaPipe( bool bEnable );
	void EnableConstantAlpha( bool bEnable );
	void EnableVertexAlpha( bool bEnable );
	void EnableTextureAlpha( TextureStage_t stage, bool bEnable );

	// GR - Separate alpha blending
	void EnableBlendingSeparateAlpha( bool bEnable );
	void BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor );

	// Sets the vertex and pixel shaders
	void SetVertexShader( const char *pFileName, int vshIndex );
	void SetPixelShader( const char *pFileName, int pshIndex );

	// Convert from linear to gamma color space on writes to frame buffer.
	void EnableSRGBWrite( bool bEnable )
	{
	}

	void EnableSRGBRead( Sampler_t stage, bool bEnable )
	{
	}

	virtual void FogMode( ShaderFogMode_t fogMode )
	{
	}
	virtual void SetDiffuseMaterialSource( ShaderMaterialSource_t materialSource )
	{
	}

	virtual void SetMorphFormat( MorphFormat_t flags )
	{
	}

	virtual void EnableStencil( bool bEnable )
	{
	}
	virtual void StencilFunc( ShaderStencilFunc_t stencilFunc )
	{
	}
	virtual void StencilPassOp( ShaderStencilOp_t stencilOp )
	{
	}
	virtual void StencilFailOp( ShaderStencilOp_t stencilOp )
	{
	}
	virtual void StencilDepthFailOp( ShaderStencilOp_t stencilOp )
	{
	}
	virtual void StencilReference( int nReference )
	{
	}
	virtual void StencilMask( int nMask )
	{
	}
	virtual void StencilWriteMask( int nMask )
	{
	}

	virtual void DisableFogGammaCorrection( bool bDisable )
	{
		//FIXME: empty for now.
	}

	// Alpha to coverage
	void EnableAlphaToCoverage( bool bEnable );

	void SetShadowDepthFiltering( Sampler_t stage );

	virtual void SetConstantBuffer( ConstantBufferHandle_t cbuffer );

	StateSnapshot_t FindOrCreateSnapshot();

public:

	ShadowStateDx11_t m_ShadowState;
	ShadowShaderStateDx11_t m_ShadowShaderState;

	CUtlFixedLinkedList<ShadowStateCacheEntryDx11_t> m_ShadowStateCache;

};


extern CShaderShadowDx11* g_pShaderShadowDx11;

#endif // SHADERSHADOWDX11_H