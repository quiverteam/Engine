//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines the shader shadow state
//
// $NoKeywords: $
//
//===========================================================================//
#include "ShaderShadow.h"

CShaderShadow::CShaderShadow()
{
	m_IsTranslucent = false;
	m_IsAlphaTested = false;
	m_bIsDepthWriteEnabled = true;
	m_bUsesVertexAndPixelShaders = false;
}

CShaderShadow::~CShaderShadow()
{
}

// Sets the default *shadow* state
void CShaderShadow::SetDefaultState()
{
	m_IsTranslucent = false;
	m_IsAlphaTested = false;
	m_bIsDepthWriteEnabled = true;
	m_bUsesVertexAndPixelShaders = false;
}

// Methods related to depth buffering
void CShaderShadow::DepthFunc( ShaderDepthFunc_t depthFunc )
{
}

void CShaderShadow::EnableDepthWrites( bool bEnable )
{
	m_bIsDepthWriteEnabled = bEnable;
}

void CShaderShadow::EnableDepthTest( bool bEnable )
{
}

void CShaderShadow::EnablePolyOffset( PolygonOffsetMode_t nOffsetMode )
{
}

// Suppresses/activates color writing 
void CShaderShadow::EnableColorWrites( bool bEnable )
{
}

// Suppresses/activates alpha writing 
void CShaderShadow::EnableAlphaWrites( bool bEnable )
{
}

// Methods related to alpha blending
void CShaderShadow::EnableBlending( bool bEnable )
{
	m_IsTranslucent = bEnable;
}

void CShaderShadow::BlendFunc( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor )
{
}

// A simpler method of dealing with alpha modulation
void CShaderShadow::EnableAlphaPipe( bool bEnable )
{
}

void CShaderShadow::EnableConstantAlpha( bool bEnable )
{
}

void CShaderShadow::EnableVertexAlpha( bool bEnable )
{
}

void CShaderShadow::EnableTextureAlpha( TextureStage_t stage, bool bEnable )
{
}


// Alpha testing
void CShaderShadow::EnableAlphaTest( bool bEnable )
{
	m_IsAlphaTested = bEnable;
}

void CShaderShadow::AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef /* [0-1] */ )
{
}


// Wireframe/filled polygons
void CShaderShadow::PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode )
{
}


// Back face culling
void CShaderShadow::EnableCulling( bool bEnable )
{
}


// Alpha to coverage
void CShaderShadow::EnableAlphaToCoverage( bool bEnable )
{
}


// constant color + transparency
void CShaderShadow::EnableConstantColor( bool bEnable )
{
}

// Indicates the vertex format for use with a vertex shader
// The flags to pass in here come from the VertexFormatFlags_t enum
// If pTexCoordDimensions is *not* specified, we assume all coordinates
// are 2-dimensional
void CShaderShadow::VertexShaderVertexFormat( unsigned int nFlags, 
												   int nTexCoordCount,
												   int* pTexCoordDimensions,
												   int nUserDataSize )
{
}

// Indicates we're going to light the model
void CShaderShadow::EnableLighting( bool bEnable )
{
}

void CShaderShadow::EnableSpecular( bool bEnable )
{
}

// Activate/deactivate skinning
void CShaderShadow::EnableVertexBlend( bool bEnable )
{
}

// per texture unit stuff
void CShaderShadow::OverbrightValue( TextureStage_t stage, float value )
{
}

void CShaderShadow::EnableTexture( Sampler_t stage, bool bEnable )
{
}

void CShaderShadow::EnableCustomPixelPipe( bool bEnable )
{
}

void CShaderShadow::CustomTextureStages( int stageCount )
{
}

void CShaderShadow::CustomTextureOperation( TextureStage_t stage, ShaderTexChannel_t channel, 
	ShaderTexOp_t op, ShaderTexArg_t arg1, ShaderTexArg_t arg2 )
{
}

void CShaderShadow::EnableTexGen( TextureStage_t stage, bool bEnable )
{
}

void CShaderShadow::TexGen( TextureStage_t stage, ShaderTexGenParam_t param )
{
}

// Sets the vertex and pixel shaders
void CShaderShadow::SetVertexShader( const char *pShaderName, int vshIndex )
{
	m_bUsesVertexAndPixelShaders = ( pShaderName != NULL );
}

void CShaderShadow::EnableBlendingSeparateAlpha( bool bEnable )
{
}
void CShaderShadow::SetPixelShader( const char *pShaderName, int pshIndex )
{
	m_bUsesVertexAndPixelShaders = ( pShaderName != NULL );
}

void CShaderShadow::BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor )
{
}
// indicates what per-vertex data we're providing
void CShaderShadow::DrawFlags( unsigned int drawFlags )
{
}

