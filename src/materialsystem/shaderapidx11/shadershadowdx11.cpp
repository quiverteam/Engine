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
CShaderShadowDx11::CShaderShadowDx11() :
	m_ShadowStateCache( 4096 )
{
}

CShaderShadowDx11::~CShaderShadowDx11()
{
}

// Sets the default *shadow* state
void CShaderShadowDx11::SetDefaultState()
{
	memset( &m_ShadowState, 0, sizeof( ShadowStateDx11_t ) );
	memset( &m_ShadowShaderState, 0, sizeof( ShadowShaderStateDx11_t ) );

	m_ShadowState.m_FillMode = SHADER_FILL_SOLID;
	m_ShadowState.m_ColorWriteEnable = true;
	m_ShadowState.m_ZEnable = true;
	m_ShadowState.m_ZWriteEnable = true;
	m_ShadowState.m_CullEnable = true;

	m_ShadowShaderState.m_PixelShader = -1;
	m_ShadowShaderState.m_VertexShader = -1;
	m_ShadowShaderState.m_nStaticPshIndex = -1;
	m_ShadowShaderState.m_nStaticVshIndex = -1;
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
	m_ShadowShaderState.m_AlphaTestEnable = bEnable;
}

void CShaderShadowDx11::AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef /* [0-1] */ )
{
}

// Wireframe/filled polygons
void CShaderShadowDx11::PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode )
{
	//m_ShadowState.m_FillMode = 
}


// Back face culling
void CShaderShadowDx11::EnableCulling( bool bEnable )
{
	m_ShadowState.m_CullEnable = bEnable;
}

// Alpha to coverage
void CShaderShadowDx11::EnableAlphaToCoverage( bool bEnable )
{
	m_ShadowState.m_EnableAlphaToCoverage = bEnable;
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
	// Code that creates a Mesh should specify whether it contains bone weights+indices, *not* the shader.
	Assert( ( nFlags & VERTEX_BONE_INDEX ) == 0 );
	nFlags &= ~VERTEX_BONE_INDEX;

	// This indicates we're using a vertex shader
	nFlags |= VERTEX_FORMAT_VERTEX_SHADER;
	m_ShadowShaderState.m_VertexUsage = MeshMgr()->ComputeVertexFormat( nFlags, nTexCoordCount,
									    pTexCoordDimensions, 0, nUserDataSize );
	//m_ShadowShaderState.m_
}

// Indicates we're going to light the model
void CShaderShadowDx11::EnableLighting( bool bEnable )
{
}

void CShaderShadowDx11::EnableSpecular( bool bEnable )
{
	Warning( "Unsupported CShaderShadowDx11::EnableSpecular() called!\n" );
}

// Activate/deactivate skinning
void CShaderShadowDx11::EnableVertexBlend( bool bEnable )
{
	// Activate/deactivate skinning. Indexed blending is automatically
	// enabled if it's available for this hardware. When blending is enabled,
	// we allocate enough room for 3 weights (max allowed)
	if ( ( HardwareConfig()->MaxBlendMatrices() > 0 ) || ( !bEnable ) )
	{
		m_ShadowState.m_VertexBlendEnable = bEnable;
	}
}

// per texture unit stuff
void CShaderShadowDx11::OverbrightValue( TextureStage_t stage, float value )
{
	Warning( "Unsupported CShaderShadowDx11::OverbrightValue() called\n" );
}

void CShaderShadowDx11::EnableTexture( Sampler_t sampler, bool bEnable )
{
	if ( sampler < HardwareConfig()->GetSamplerCount() )
	{
		m_ShadowState.m_Samplers[sampler].m_TextureEnable = bEnable;
	}
	else
	{
		Warning( "Attempting to bind a texture to an invalid sampler (%d)!\n", sampler );
	}
}

void CShaderShadowDx11::EnableCustomPixelPipe( bool bEnable )
{
	Warning( "Unsupported CShaderShadowDx11::EnableCustomPixelPipe() called\n" );
}

void CShaderShadowDx11::CustomTextureStages( int stageCount )
{
	Warning( "Unsupported CShaderShadowDx11::CustomTextureStages() called\n" );
}

void CShaderShadowDx11::CustomTextureOperation( TextureStage_t stage, ShaderTexChannel_t channel, 
						ShaderTexOp_t op, ShaderTexArg_t arg1, ShaderTexArg_t arg2 )
{
	Warning( "Unsupported CShaderShadowDx11::CustomTextureOperation() called\n" );
}

void CShaderShadowDx11::EnableTexGen( TextureStage_t stage, bool bEnable )
{
	Warning( "Unsupported CShaderShadowDx11::EnableTexGen() called\n" );
}

void CShaderShadowDx11::TexGen( TextureStage_t stage, ShaderTexGenParam_t param )
{
	Warning( "Unsupported CShaderShadowDx11::TexGen() called\n" );
}

// Sets the vertex and pixel shaders
void CShaderShadowDx11::SetVertexShader( const char *pShaderName, int vshIndex )
{
	m_ShadowShaderState.m_UseVertexAndPixelShaders = ( pShaderName != NULL );
}

void CShaderShadowDx11::EnableBlendingSeparateAlpha( bool bEnable )
{
	m_ShadowState.m_SeparateAlphaBlendEnable = bEnable;
}

void CShaderShadowDx11::SetPixelShader( const char *pShaderName, int pshIndex )
{
	m_ShadowShaderState.m_UseVertexAndPixelShaders = ( pShaderName != NULL );
}

void CShaderShadowDx11::BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor )
{
	m_ShadowState.m_SrcBlendAlpha = srcFactor;
	m_ShadowState.m_DestBlendAlpha = dstFactor;
}

// indicates what per-vertex data we're providing
void CShaderShadowDx11::DrawFlags( unsigned int drawFlags )
{
}

void CShaderShadowDx11::SetConstantBuffer( ConstantBufferHandle_t cbuffer )
{
	m_ShadowShaderState.m_CBuffers[m_ShadowShaderState.m_nCBuffers++] = cbuffer;
}

StateSnapshot_t CShaderShadowDx11::FindOrCreateSnapshot()
{
	int i;
	for ( i = m_ShadowStateCache.Head();
	      i != m_ShadowStateCache.InvalidIndex();
	      i = m_ShadowStateCache.Next( i ) )
	{
		ShadowStateCacheEntryDx11_t entry = m_ShadowStateCache.Element( i );
		if ( !memcmp( &entry.m_State, &m_ShadowState, sizeof( ShadowStateDx11_t ) ) &&
		     !memcmp( &entry.m_ShaderState, &m_ShadowShaderState, sizeof( ShadowShaderStateDx11_t ) ) )
		{
			// Matching states
			return i;
		}
	}

	// Didn't find it, add entry
	ShadowStateCacheEntryDx11_t entry;
	entry.m_State = m_ShadowState;
	entry.m_ShaderState = m_ShadowShaderState;
	return m_ShadowStateCache.AddToTail( entry );
}