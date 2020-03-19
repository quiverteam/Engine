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
#include "StatesDx11.h"

#define DEFAULT_SHADOW_STATE_ID	-1

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

	// vertex blending
	void EnableVertexBlend( bool bEnable );

	// per texture unit stuff
	void EnableTexture( Sampler_t stage, bool bEnable );

	// GR - Separate alpha blending
	void EnableBlendingSeparateAlpha( bool bEnable );
	void BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor );

	// Sets the vertex and pixel shaders
	void SetVertexShader( const char *pFileName, ShaderIndex_t vshIndex );
	void SetPixelShader( const char *pFileName, ShaderIndex_t pshIndex );

	virtual void SetMorphFormat( MorphFormat_t flags );

	virtual void EnableStencil( bool bEnable );
	virtual void StencilFunc( ShaderStencilFunc_t stencilFunc );
	virtual void StencilPassOp( ShaderStencilOp_t stencilOp );
	virtual void StencilFailOp( ShaderStencilOp_t stencilOp );
	virtual void StencilDepthFailOp( ShaderStencilOp_t stencilOp );
	virtual void StencilReference( int nReference );
	virtual void StencilMask( int nMask );
	virtual void StencilWriteMask( int nMask );

	StateSnapshot_t FindOrCreateSnapshot();
	const StatesDx11::ShadowState *GetShadowState( StateSnapshot_t id );
	const StatesDx11::ShadowState *GetDefaultShadowState();

	// Constant buffer enabling
	virtual void SetVertexShaderConstantBuffer( int slot, ConstantBufferHandle_t cbuffer );
	virtual void SetVertexShaderConstantBuffer( int slot, ShaderInternalConstantBuffer_t cbuffer );
	virtual void SetGeometryShaderConstantBuffer( int slot, ConstantBufferHandle_t cbuffer );
	virtual void SetGeometryShaderConstantBuffer( int slot, ShaderInternalConstantBuffer_t cbuffer );
	virtual void SetPixelShaderConstantBuffer( int slot, ConstantBufferHandle_t cbuffer );
	virtual void SetPixelShaderConstantBuffer( int slot, ShaderInternalConstantBuffer_t cbuffer );

	// ---------------------------------------------------
	// Below are unsupported by Dx11, only included to
	// not break Dx9 compatibility.
	// ---------------------------------------------------

	// Convert from linear to gamma color space on writes to frame buffer.
	void EnableSRGBWrite( bool bEnable );
	void EnableSRGBRead( Sampler_t stage, bool bEnable );
	virtual void FogMode( ShaderFogMode_t fogMode );
	virtual void SetDiffuseMaterialSource( ShaderMaterialSource_t materialSource );
	void EnableTexGen( TextureStage_t stage, bool bEnable );
	void TexGen( TextureStage_t stage, ShaderTexGenParam_t param );
	// alternate method of specifying per-texture unit stuff, more flexible and more complicated
	// Can be used to specify different operation per channel (alpha/color)...
	void EnableCustomPixelPipe( bool bEnable );
	void CustomTextureStages( int stageCount );
	void CustomTextureOperation( TextureStage_t stage, ShaderTexChannel_t channel,
				     ShaderTexOp_t op, ShaderTexArg_t arg1, ShaderTexArg_t arg2 );
	void EnableTextureAlpha( TextureStage_t stage, bool bEnable );
	void EnableSpecular( bool bEnable );
	// A simpler method of dealing with alpha modulation
	void EnableAlphaPipe( bool bEnable );
	void EnableConstantAlpha( bool bEnable );
	void EnableVertexAlpha( bool bEnable );
	// Alpha to coverage
	void EnableAlphaToCoverage( bool bEnable );
	void SetShadowDepthFiltering( Sampler_t stage );
	void OverbrightValue( TextureStage_t stage, float value );
	virtual void DisableFogGammaCorrection( bool bDisable );
	// indicates what per-vertex data we're providing
	void DrawFlags( unsigned int drawFlags );

private:
	unsigned int FindOrCreateConstantBufferState( StatesDx11::ConstantBufferDesc &desc );
	unsigned int FindOrCreateDepthStencilState( StatesDx11::DepthStencilDesc &desc );
	unsigned int FindOrCreateBlendState( StatesDx11::BlendDesc &desc );
	unsigned int FindOrCreateRasterState( StatesDx11::RasterDesc &desc );

public:
	StatesDx11::ShadowStateDesc m_ShadowState;

	CUtlVector<StatesDx11::ConstantBufferDesc> m_ConstantBufferStates;
	CUtlVector<StatesDx11::DepthStencilDesc> m_DepthStencilStates;
	CUtlVector<StatesDx11::BlendDesc> m_BlendStates;
	CUtlVector<StatesDx11::RasterDesc> m_RasterizerStates;
	// Can have max value of StateSnapshot_t shadow states.
	CUtlVector<StatesDx11::ShadowState> m_ShadowStates;

	StatesDx11::ShadowState m_DefaultShadowState;
	StatesDx11::ConstantBufferDesc m_DefaultCBState;
	StatesDx11::DepthStencilDesc m_DefaultDepthStencilState;
	StatesDx11::BlendDesc m_DefaultBlendState;
	StatesDx11::RasterDesc m_DefaultRasterState;
};

extern CShaderShadowDx11* g_pShaderShadowDx11;

#endif // SHADERSHADOWDX11_H