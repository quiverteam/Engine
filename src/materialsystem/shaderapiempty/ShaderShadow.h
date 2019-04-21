//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines the shader shadow state
//
// $NoKeywords: $
//
//===========================================================================//
#pragma once

#include "utlvector.h"
#include "materialsystem/imaterialsystem.h"
#include "IHardwareConfigInternal.h"
#include "shadersystem.h"
#include "shaderapi/ishaderutil.h"
#include "shaderapi/ishaderapi.h"
#include "materialsystem/imesh.h"
#include "tier0/dbg.h"
#include "materialsystem/idebugtextureinfo.h"
#include "materialsystem/deformations.h"

/*

Defines a basic (empty) shader shadow

My understanding of the shader shadow is that it is where the static 
stuff of shaders is setup.
For example, this is where inputs to a shader would be bound, and this
is what it appears to be doing.

Different functions here enable things like culling, lighting, specular, etc.
this is most likely to choose what portion of the graphics pipeline this 
will go.

For example, if you enable lighting and alpha blending, this might go into
a later part of the shader pipeline.

This might also determine what the shader has access to in terms of textures
or buffers. Enabling color writes *might* give the shader access
to the framebuffer.

*/
class CShaderShadow : public IShaderShadow
{
public:
	CShaderShadow();
	virtual ~CShaderShadow();

	/*
    Sets the default shader shadow state
    */
	void SetDefaultState();

	// Methods related to depth buffering
    /*
    */
	void DepthFunc( ShaderDepthFunc_t depthFunc );

    /*
    */
	void EnableDepthWrites( bool bEnable );

    /*
    */
	void EnableDepthTest( bool bEnable );

    /*
    */
	void EnablePolyOffset( PolygonOffsetMode_t nOffsetMode );

	// Suppresses/activates color writing 
    /*
    Enables/Disables color writes
    */
	void EnableColorWrites( bool bEnable );

    /*
    Enables/disables alpha writes
    */
	void EnableAlphaWrites( bool bEnable );

	// Methods related to alpha blending
    /*
    Enables/disables blending funcs
    */
	void EnableBlending( bool bEnable );

    /*
    */
	void BlendFunc( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor );

	// Alpha testing
    /*
    */
	void EnableAlphaTest( bool bEnable );

    /*
    */
	void AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef /* [0-1] */ );

    /*
    Sets wireframe or filled polygons
    */
	void PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode );

	/*
    Enables/disables backface culling
    */
	void EnableCulling( bool bEnable );
	
	// constant color + transparency
	void EnableConstantColor( bool bEnable );

	// Indicates the vertex format for use with a vertex shader
	// The flags to pass in here come from the VertexFormatFlags_t enum
	// If pTexCoordDimensions is *not* specified, we assume all coordinates
	// are 2-dimensional
	void VertexShaderVertexFormat( unsigned int nFlags, 
		int nTexCoordCount, int* pTexCoordDimensions, int nUserDataSize );
	
	// Indicates we're going to light the model
    /*
    */
	void EnableLighting( bool bEnable );

    /*
    */
	void EnableSpecular( bool bEnable );

	// vertex blending
    /*
    */
	void EnableVertexBlend( bool bEnable );

	// per texture unit stuff
    /*
    */
	void OverbrightValue( TextureStage_t stage, float value );

    /*
    */
	void EnableTexture( Sampler_t stage, bool bEnable );

    /*
    */
	void EnableTexGen( TextureStage_t stage, bool bEnable );

    /*
    */
	void TexGen( TextureStage_t stage, ShaderTexGenParam_t param );

	// alternate method of specifying per-texture unit stuff, more flexible and more complicated
	// Can be used to specify different operation per channel (alpha/color)...
    /*
    */
	void EnableCustomPixelPipe( bool bEnable );

    /*
    */
	void CustomTextureStages( int stageCount );

    /*
    */
	void CustomTextureOperation( TextureStage_t stage, ShaderTexChannel_t channel, 
		ShaderTexOp_t op, ShaderTexArg_t arg1, ShaderTexArg_t arg2 );

	// indicates what per-vertex data we're providing
    /*
    */
	void DrawFlags( unsigned int drawFlags );

	// A simpler method of dealing with alpha modulation
    /*
    */
	void EnableAlphaPipe( bool bEnable );

    /*
    */
	void EnableConstantAlpha( bool bEnable );

    /*
    */
	void EnableVertexAlpha( bool bEnable );

    /*
    */
	void EnableTextureAlpha( TextureStage_t stage, bool bEnable );

	// GR - Separate alpha blending
    /*
    */
	void EnableBlendingSeparateAlpha( bool bEnable );

    /*
    */
	void BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor );

	// Sets the vertex and pixel shaders
    /*
    */
	void SetVertexShader( const char *pFileName, int vshIndex );

    /*
    */
	void SetPixelShader( const char *pFileName, int pshIndex );

	// Convert from linear to gamma color space on writes to frame buffer.
    /*
    */
	void EnableSRGBWrite( bool bEnable )
	{
	}

    /*
    */
	void EnableSRGBRead( Sampler_t stage, bool bEnable )
	{
	}

    /*
    */
	virtual void FogMode( ShaderFogMode_t fogMode )
	{
	}

    /*
    */
	virtual void DisableFogGammaCorrection( bool bDisable )
	{
	}

    /*
    */
	virtual void SetDiffuseMaterialSource( ShaderMaterialSource_t materialSource )
	{
	}

    /*
    */
	virtual void SetMorphFormat( MorphFormat_t flags )
	{
	}

    /*
    */
	virtual void EnableStencil( bool bEnable )
	{
	}

    /*
    */
	virtual void StencilFunc( ShaderStencilFunc_t stencilFunc )
	{
	}

    /*
    */
	virtual void StencilPassOp( ShaderStencilOp_t stencilOp )
	{
	}

    /*
    */
	virtual void StencilFailOp( ShaderStencilOp_t stencilOp )
	{
	}

    /*
    */
	virtual void StencilDepthFailOp( ShaderStencilOp_t stencilOp )
	{
	}

    /*
    */
	virtual void StencilReference( int nReference )
	{
	}

    /*
    */
	virtual void StencilMask( int nMask )
	{
	}

    /*
    */
	virtual void StencilWriteMask( int nMask )
	{
	}

    /*
    */
	virtual void ExecuteCommandBuffer( uint8 *pBuf ) 
	{
	}

	// Alpha to coverage
    /*
    */
	void EnableAlphaToCoverage( bool bEnable );
	
    /*
    */
	virtual void SetShadowDepthFiltering( Sampler_t stage )
	{
	}

	bool m_IsTranslucent;
	bool m_IsAlphaTested;
	bool m_bIsDepthWriteEnabled;
	bool m_bUsesVertexAndPixelShaders;
};