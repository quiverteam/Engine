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
#include "vertexshaderdx11.h"

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
	m_ShadowState = StatesDx11::ShadowRenderState();
	m_ShadowShaderState = StatesDx11::ShadowShaderState();
}

// Methods related to depth buffering
void CShaderShadowDx11::DepthFunc( ShaderDepthFunc_t depthFunc )
{
	m_ShadowState.depthTestAttrib.depthFunc = depthFunc;
}

void CShaderShadowDx11::EnableDepthWrites( bool bEnable )
{
	m_ShadowState.depthWriteAttrib.bEnableDepthWrite = bEnable;
}

void CShaderShadowDx11::EnableDepthTest( bool bEnable )
{
	m_ShadowState.depthTestAttrib.bEnableDepthTest = bEnable;
}

void CShaderShadowDx11::EnablePolyOffset( PolygonOffsetMode_t nOffsetMode )
{
	m_ShadowState.depthOffsetAttrib.bEnable = nOffsetMode != SHADER_POLYOFFSET_DISABLE;
}

// Suppresses/activates color writing 
void CShaderShadowDx11::EnableColorWrites( bool bEnable )
{
	if ( bEnable )
	{
		m_ShadowState.colorWriteAttrib.colorWriteMask |= StatesDx11::ColorWriteAttrib::COLORWRITE_RGB;
	}
	else
	{
		m_ShadowState.colorWriteAttrib.colorWriteMask &= ~StatesDx11::ColorWriteAttrib::COLORWRITE_RGB;
	}
	
}

// Suppresses/activates alpha writing 
void CShaderShadowDx11::EnableAlphaWrites( bool bEnable )
{
	if ( bEnable )
	{
		m_ShadowState.colorWriteAttrib.colorWriteMask |= StatesDx11::ColorWriteAttrib::COLORWRITE_ALPHA;
	}
	else
	{
		m_ShadowState.colorWriteAttrib.colorWriteMask &= ~StatesDx11::ColorWriteAttrib::COLORWRITE_ALPHA;
	}
}

// Methods related to alpha blending
void CShaderShadowDx11::EnableBlending( bool bEnable )
{
	m_ShadowState.transparencyAttrib.bTranslucent = bEnable;
}

void CShaderShadowDx11::BlendFunc( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor )
{
	m_ShadowState.colorBlendAttrib.bBlendEnable = true;
	m_ShadowState.colorBlendAttrib.srcBlendAlpha = srcFactor;
	m_ShadowState.colorBlendAttrib.destBlendAlpha = dstFactor;
	m_ShadowState.colorBlendAttrib.srcBlend = srcFactor;
	m_ShadowState.colorBlendAttrib.destBlend = dstFactor;
}

// Alpha testing
void CShaderShadowDx11::EnableAlphaTest( bool bEnable )
{
	m_ShadowState.alphaTestAttrib.bEnable = bEnable;
}

void CShaderShadowDx11::AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef /* [0-1] */ )
{
	m_ShadowState.alphaTestAttrib.alphaFunc = alphaFunc;
	m_ShadowState.alphaTestAttrib.alphaTestRef = alphaRef;
}

// Wireframe/filled polygons
void CShaderShadowDx11::PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode )
{
	m_ShadowState.renderModeAttrib.polyMode = polyMode;
	m_ShadowState.renderModeAttrib.faceMode = face;
}


// Back face culling
void CShaderShadowDx11::EnableCulling( bool bEnable )
{
	// DX11FIXME
	m_ShadowState.cullFaceAttrib.cullMode = MATERIAL_CULLMODE_CCW;
}

// constant color + transparency
void CShaderShadowDx11::EnableConstantColor( bool bEnable )
{
	m_ShadowState.colorAttrib.bEnable = bEnable;
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
	Assert( ( flags & VERTEX_BONE_INDEX ) == 0 );
	flags &= ~VERTEX_BONE_INDEX;

	// This indicates we're using a vertex shader
	flags |= VERTEX_FORMAT_VERTEX_SHADER;
	m_ShadowShaderState.shaderAttrib.vertexFormat = CMeshDx11::ComputeVertexFormat( flags, numTexCoords,
									    pTexCoordDimensions, 0, userDataSize );
	// Avoid an error if vertex stream 0 is too narrow
	if ( CVertexBufferBase::VertexFormatSize( m_ShadowShaderState.shaderAttrib.vertexFormat ) <= 16 )
	{
		// FIXME: this is only necessary because we
		//          (a) put the flex normal/position stream in ALL vertex decls
		//          (b) bind stream 0's VB to stream 2 if there is no actual flex data
		//        ...it would be far more sensible to not add stream 2 to all vertex decls.
		static bool bComplained = false;
		if ( !bComplained )
		{
			Warning( "ERROR: shader asking for a too-narrow vertex format - you will see errors if running with debug D3D DLLs!\n\tPadding the vertex format with extra texcoords\n\tWill not warn again.\n" );
			bComplained = true;
		}
		// All vertex formats should contain position...
		Assert( nFlags & VERTEX_POSITION );
		flags |= VERTEX_POSITION;
		// This error should occur only if we have zero texcoords, or if we have a single, 1-D texcoord
		Assert( ( nTexCoordCount == 0 ) ||
			( ( nTexCoordCount == 1 ) && pTexCoordDimensions && ( pTexCoordDimensions[0] == 1 ) ) );
		numTexCoords			  = 1;
		m_ShadowShaderState.shaderAttrib.vertexFormat = CMeshDx11::ComputeVertexFormat(
			flags, numTexCoords, NULL, 0, userDataSize );
	}
}

// Indicates we're going to light the model
void CShaderShadowDx11::EnableLighting( bool bEnable )
{
	m_ShadowState.lightAttrib.bEnable = bEnable;
}

// Activate/deactivate skinning
void CShaderShadowDx11::EnableVertexBlend( bool bEnable )
{
	// Activate/deactivate skinning. Indexed blending is automatically
	// enabled if it's available for this hardware. When blending is enabled,
	// we allocate enough room for 3 weights (max allowed)
	if ( ( HardwareConfig()->MaxBlendMatrices() > 0 ) || ( !bEnable ) )
	{
		m_ShadowState.vertexBlendAttrib.bEnable = bEnable;
	}
}

void CShaderShadowDx11::EnableTexture( Sampler_t sampler, bool bEnable )
{
	if ( sampler < HardwareConfig()->GetSamplerCount() )
	{
		m_ShadowState.samplerAttrib.samplers[sampler] = bEnable;
	}
	else
	{
		Warning( "Attempting to bind a texture to an invalid sampler (%d)!\n", sampler );
	}
}

// Sets the vertex and pixel shaders
void CShaderShadowDx11::SetVertexShader( const char *pShaderName, int vshIndex )
{
	m_ShadowShaderState.shaderAttrib.vertexShader = ShaderManager()->CreateVertexShader( pShaderName, vshIndex );
	m_ShadowShaderState.shaderAttrib.vertexShaderIndex = vshIndex;
}

void CShaderShadowDx11::EnableBlendingSeparateAlpha( bool bEnable )
{
	// DX11FIXME
	//m_ShadowState.m_SeparateAlphaBlendEnable = bEnable;
}

void CShaderShadowDx11::SetPixelShader( const char *pShaderName, int pshIndex )
{
	m_ShadowShaderState.shaderAttrib.pixelShader = ShaderManager()->CreatePixelShader( pShaderName, pshIndex );
	m_ShadowShaderState.shaderAttrib.pixelShaderIndex = pshIndex;
}

void CShaderShadowDx11::SetMorphFormat( MorphFormat_t flags )
{
	m_ShadowShaderState.shaderAttrib.morphFormat = flags;
}

void CShaderShadowDx11::EnableStencil( bool bEnable )
{
	m_ShadowState.stencilAttrib.bStencilEnable = bEnable;
}

void CShaderShadowDx11::StencilFunc( ShaderStencilFunc_t stencilFunc )
{
	m_ShadowState.stencilAttrib.stencilFunc = stencilFunc;
}

void CShaderShadowDx11::StencilPassOp( ShaderStencilOp_t stencilOp )
{
	m_ShadowState.stencilAttrib.stencilPassOp = stencilOp;
}

void CShaderShadowDx11::StencilFailOp( ShaderStencilOp_t stencilOp )
{
	m_ShadowState.stencilAttrib.stencilFailOp = stencilOp;
}

void CShaderShadowDx11::StencilDepthFailOp( ShaderStencilOp_t stencilOp )
{
	m_ShadowState.stencilAttrib.stencilDepthFailOp = stencilOp;
}

void CShaderShadowDx11::StencilReference( int nReference )
{
	m_ShadowState.stencilAttrib.stencilRef = nReference;
}

void CShaderShadowDx11::StencilMask( int nMask )
{
	m_ShadowState.stencilAttrib.stencilWriteMask = nMask;
}

void CShaderShadowDx11::StencilWriteMask( int nMask )
{
	m_ShadowState.stencilAttrib.stencilReadMask = nMask;
}

void CShaderShadowDx11::BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor )
{
	m_ShadowState.colorBlendAttrib.srcBlendAlpha = srcFactor;
	m_ShadowState.colorBlendAttrib.destBlendAlpha = dstFactor;
}

void CShaderShadowDx11::SetConstantBuffer( ConstantBufferHandle_t cbuffer )
{
	m_ShadowShaderState.shaderAttrib.AddConstantBuffer( cbuffer );
}

StateSnapshot_t CShaderShadowDx11::FindOrCreateSnapshot()
{
	int i;
	for ( i = m_ShadowStateCache.Head();
	      i != m_ShadowStateCache.InvalidIndex();
	      i = m_ShadowStateCache.Next( i ) )
	{
		StatesDx11::RenderState entry = m_ShadowStateCache.Element( i );
		if ( !memcmp( &entry.shadowState, &m_ShadowState, sizeof( StatesDx11::ShadowRenderState ) ) )
		{
			// Matching states
			return i;
		}
	}

	// Didn't find it, add entry
	StatesDx11::RenderState rs;
	rs.shadowState = m_ShadowState;
	rs.shaderState = m_ShadowShaderState;
	return m_ShadowStateCache.AddToTail( rs );
}

// ---------------------------------------------------
// Below are unsupported by Dx11, only included to
// not break compatibility with Dx9.
// ---------------------------------------------------

// A simpler method of dealing with alpha modulation
void CShaderShadowDx11::EnableAlphaPipe( bool bEnable )
{
	Warning( "Unsupported CShaderShadowDx11::EnableAlphaPipe() called\n" );
}

void CShaderShadowDx11::EnableConstantAlpha( bool bEnable )
{
	Warning( "Unsupported CShaderShadowDx11::EnableConstantAlpha() called\n" );
}

void CShaderShadowDx11::EnableVertexAlpha( bool bEnable )
{
	Warning( "Unsupported CShaderShadowDx11::EnableVertexAlpha() called\n" );
}

void CShaderShadowDx11::EnableTextureAlpha( TextureStage_t stage, bool bEnable )
{
	Warning( "Unsupported CShaderShadowDx11::EnableTextureAlpha() called\n" );
}

void CShaderShadowDx11::SetShadowDepthFiltering( Sampler_t stage )
{
	Warning( "Unsupported CShaderShadowDx11::SetShadowDepthFiltering() called\n" );
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

void CShaderShadowDx11::EnableSRGBWrite( bool bEnable )
{
	Warning( "Unsupported CShaderShadowDx11::EnableSRGBWrite() called\n" );
}

void CShaderShadowDx11::EnableSRGBRead( Sampler_t stage, bool bEnable )
{
	Warning( "Unsupported CShaderShadowDx11::EnableSRGBRead() called\n" );
}

void CShaderShadowDx11::FogMode( ShaderFogMode_t fogMode )
{
	Warning( "Unsupported CShaderShadowDx11::FogMode() called\n" );
}

void CShaderShadowDx11::SetDiffuseMaterialSource( ShaderMaterialSource_t materialSource )
{
	Warning( "Unsupported CShaderShadowDx11::SetDiffuseMaterialSource() called\n" );
}

void CShaderShadowDx11::EnableTexGen( TextureStage_t stage, bool bEnable )
{
	Warning( "Unsupported CShaderShadowDx11::EnableTexGen() called\n" );
}

void CShaderShadowDx11::TexGen( TextureStage_t stage, ShaderTexGenParam_t param )
{
	Warning( "Unsupported CShaderShadowDx11::TexGen() called\n" );
}

// per texture unit stuff
void CShaderShadowDx11::OverbrightValue( TextureStage_t stage, float value )
{
	Warning( "Unsupported CShaderShadowDx11::OverbrightValue() called\n" );
}

void CShaderShadowDx11::DisableFogGammaCorrection( bool bDisable )
{
	Warning( "Unsupported CShaderShadowDx11::DisableFogGammaCorrection() called\n" );
}

void CShaderShadowDx11::EnableSpecular( bool bEnable )
{
	Warning( "Unsupported CShaderShadowDx11::EnableSpecular() called!\n" );
}

// Alpha to coverage
void CShaderShadowDx11::EnableAlphaToCoverage( bool bEnable )
{
	Warning( "Unsupported CShaderShadowDx11::EnableAlphaToCoverage() called\n" );
	//m_ShadowState.m_EnableAlphaToCoverage = bEnable;
}

// indicates what per-vertex data we're providing
void CShaderShadowDx11::DrawFlags( unsigned int drawFlags )
{
	Warning( "Unsupported CShaderShadowDx11::DrawFlags() called!\n" );
}