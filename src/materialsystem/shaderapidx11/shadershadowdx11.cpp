//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include "shadershadowdx11.h"
#include "utlvector.h"
#include "materialsystem/imaterialsystem.h"
#include "IHardwareConfigInternal.h"
#include "shadersystem.h"
#include "shaderapi/ishaderutil.h"
#include "materialsystem/imesh.h"
#include "tier0/dbg.h"
#include "materialsystem/idebugtextureinfo.h"

#include "shaderapidx11.h"
#include "shaderdevicedx11.h"


//-----------------------------------------------------------------------------
// Class Factory
//-----------------------------------------------------------------------------
static CShaderShadowDx11 s_ShaderShadow;
CShaderShadowDx11 *g_pShaderShadowDx11 = &s_ShaderShadow;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderShadowDx11, IShaderShadow, 
								  SHADERSHADOW_INTERFACE_VERSION, s_ShaderShadow )

//-----------------------------------------------------------------------------
// The shader shadow interface
//-----------------------------------------------------------------------------
CShaderShadowDx11::CShaderShadowDx11()
{
	m_IsTranslucent = false;
	m_IsAlphaTested = false;
	m_bIsDepthWriteEnabled = true;
	m_bUsesVertexAndPixelShaders = false;
}

CShaderShadowDx11::~CShaderShadowDx11()
{
}

// Sets the default *shadow* state
void CShaderShadowDx11::SetDefaultState()
{
	m_IsTranslucent = false;
	m_IsAlphaTested = false;
	m_bIsDepthWriteEnabled = true;
	m_bUsesVertexAndPixelShaders = false;
}

// Methods related to depth buffering
void CShaderShadowDx11::DepthFunc( ShaderDepthFunc_t depthFunc )
{
	m_ShadowState.m_ZFunc = depthFunc;
}

void CShaderShadowDx11::EnableDepthWrites( bool bEnable )
{
	m_ShadowState.m_ZWriteEnable = bEnable;
}

void CShaderShadowDx11::EnableDepthTest( bool bEnable )
{
	m_ShadowState.m_ZEnable = bEnable;
}

void CShaderShadowDx11::EnablePolyOffset( PolygonOffsetMode_t nOffsetMode )
{
	m_ShadowState.m_ZBias = nOffsetMode;
}

// Suppresses/activates color writing 
void CShaderShadowDx11::EnableColorWrites( bool bEnable )
{
	if ( bEnable )
	{
		m_ShadowState.m_ColorWriteEnable |= ( D3D11_COLOR_WRITE_ENABLE_RED |
						      D3D11_COLOR_WRITE_ENABLE_GREEN |
						      D3D11_COLOR_WRITE_ENABLE_BLUE );
	}
	else
	{
		m_ShadowState.m_ColorWriteEnable &= ~( D3D11_COLOR_WRITE_ENABLE_RED |
						       D3D11_COLOR_WRITE_ENABLE_GREEN |
						       D3D11_COLOR_WRITE_ENABLE_BLUE );
	}
	
}

// Suppresses/activates alpha writing 
void CShaderShadowDx11::EnableAlphaWrites( bool bEnable )
{
	if ( bEnable )
	{
		m_ShadowState.m_ColorWriteEnable |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
	}
	else
	{
		m_ShadowState.m_ColorWriteEnable &= ~D3D11_COLOR_WRITE_ENABLE_ALPHA;
	}
}

// Methods related to alpha blending
void CShaderShadowDx11::EnableBlending( bool bEnable )
{
	m_ShadowState.m_AlphaBlendEnable = bEnable;
}

void CShaderShadowDx11::BlendFunc( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor )
{
	m_ShadowState.m_SrcBlend = srcFactor;
	m_ShadowState.m_DestBlend = dstFactor;
}

// A simpler method of dealing with alpha modulation
void CShaderShadowDx11::EnableAlphaPipe( bool bEnable )
{
}

void CShaderShadowDx11::EnableConstantAlpha( bool bEnable )
{
}

void CShaderShadowDx11::EnableVertexAlpha( bool bEnable )
{
}

void CShaderShadowDx11::EnableTextureAlpha( TextureStage_t stage, bool bEnable )
{
}

void CShaderShadowDx11::SetShadowDepthFiltering( Sampler_t stage )
{
}

// Alpha testing
void CShaderShadowDx11::EnableAlphaTest( bool bEnable )
{
}

void CShaderShadowDx11::AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef /* [0-1] */ )
{
}

// Wireframe/filled polygons
void CShaderShadowDx11::PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode )
{
}


// Back face culling
void CShaderShadowDx11::EnableCulling( bool bEnable )
{
}

// Alpha to coverage
void CShaderShadowDx11::EnableAlphaToCoverage( bool bEnable )
{
}

// constant color + transparency
void CShaderShadowDx11::EnableConstantColor( bool bEnable )
{
}


// Indicates the vertex format for use with a vertex shader
// The flags to pass in here come from the VertexFormatFlags_t enum
// If pTexCoordDimensions is *not* specified, we assume all coordinates
// are 2-dimensional
void CShaderShadowDx11::VertexShaderVertexFormat( unsigned int flags, 
												  int numTexCoords, int* pTexCoordDimensions,
												  int userDataSize )
{
}

// Indicates we're going to light the model
void CShaderShadowDx11::EnableLighting( bool bEnable )
{
}

void CShaderShadowDx11::EnableSpecular( bool bEnable )
{
}

// Activate/deactivate skinning
void CShaderShadowDx11::EnableVertexBlend( bool bEnable )
{
}

// per texture unit stuff
void CShaderShadowDx11::OverbrightValue( TextureStage_t stage, float value )
{
}

void CShaderShadowDx11::EnableTexture( Sampler_t stage, bool bEnable )
{
}

void CShaderShadowDx11::EnableCustomPixelPipe( bool bEnable )
{
}

void CShaderShadowDx11::CustomTextureStages( int stageCount )
{
}

void CShaderShadowDx11::CustomTextureOperation( TextureStage_t stage, ShaderTexChannel_t channel, 
												ShaderTexOp_t op, ShaderTexArg_t arg1, ShaderTexArg_t arg2 )
{
}

void CShaderShadowDx11::EnableTexGen( TextureStage_t stage, bool bEnable )
{
}

void CShaderShadowDx11::TexGen( TextureStage_t stage, ShaderTexGenParam_t param )
{
}

// Sets the vertex and pixel shaders
void CShaderShadowDx11::SetVertexShader( const char *pShaderName, int vshIndex )
{
	m_bUsesVertexAndPixelShaders = ( pShaderName != NULL );
}

void CShaderShadowDx11::EnableBlendingSeparateAlpha( bool bEnable )
{
}
void CShaderShadowDx11::SetPixelShader( const char *pShaderName, int pshIndex )
{
	m_bUsesVertexAndPixelShaders = ( pShaderName != NULL );
}

void CShaderShadowDx11::BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor )
{
}

// indicates what per-vertex data we're providing
void CShaderShadowDx11::DrawFlags( unsigned int drawFlags )
{
}

void CShaderShadowDx11::SetConstantBuffer( ConstantBufferHandle_t cbuffer, size_t nBufSize )
{
}