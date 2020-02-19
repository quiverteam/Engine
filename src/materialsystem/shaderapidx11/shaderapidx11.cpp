//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: ShaderAPIDx11 implementation
//
// $NoKeywords: $
//
//===========================================================================//

#include "shaderapidx11.h"
#include "shaderapidx9/shaderapibase.h"
#include "shaderapi/ishaderutil.h"
#include "materialsystem/idebugtextureinfo.h"
#include "materialsystem/materialsystem_config.h"
#include "meshdx11.h"
#include "shadershadowdx11.h"
#include "shaderdevicedx11.h"
#include "shaderapidx11_global.h"
#include "imaterialinternal.h"
#include "ShaderConstantBufferDx11.h"
#include "vertexshaderdx11.h"
#include "VertexBufferDx11.h"
#include "IndexBufferDx11.h"

#define AttribsDiffer(var, type) memcmp(&m_TargetState.shadowState.##var, &m_State.shadowState.##var, sizeof(##type))

//-----------------------------------------------------------------------------
// Rasterizer state
//-----------------------------------------------------------------------------

static void GenerateRasterizerDesc( D3D11_RASTERIZER_DESC *pDesc, const StatesDx11::RenderState &state )
{
	const StatesDx11::RenderModeAttrib &rma = state.shadowState.renderModeAttrib;
	const StatesDx11::CullFaceAttrib &cfa = state.shadowState.cullFaceAttrib;
	const StatesDx11::DepthOffsetAttrib &doa = state.shadowState.depthOffsetAttrib;
	const StatesDx11::ScissorAttrib &sa = state.shadowState.scissorAttrib;
	const StatesDx11::AntialiasAttrib &aa = state.shadowState.antialiasAttrib;

	pDesc->FillMode = ( rma.faceMode == SHADER_POLYMODE_LINE ) ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;

	// Cull state
	if ( !cfa.bEnable )
	{
		pDesc->CullMode = D3D11_CULL_NONE;
	}
	else
	{
		pDesc->CullMode = ( cfa.cullMode == MATERIAL_CULLMODE_CW ) ? D3D11_CULL_BACK : D3D11_CULL_FRONT;
	}
	pDesc->FrontCounterClockwise = TRUE;

	// Depth bias state
	if ( !doa.bEnable )
	{
		pDesc->DepthBias	    = 0;
		pDesc->DepthBiasClamp	    = 0.0f;
		pDesc->SlopeScaledDepthBias = 0.0f;
		pDesc->DepthClipEnable	    = FALSE;
	}
	else
	{
		// FIXME: Implement! Read ConVars
		pDesc->DepthBias = doa.offset;
		pDesc->DepthBiasClamp = 0.0f;
		pDesc->SlopeScaledDepthBias = 0.0f;
		pDesc->DepthClipEnable = FALSE;
	}

	pDesc->ScissorEnable	     = sa.bScissorEnable ? TRUE : FALSE;
	pDesc->MultisampleEnable     = aa.bMultisampleEnable ? TRUE : FALSE;
	pDesc->AntialiasedLineEnable = FALSE;
}

//-----------------------------------------------------------------------------
// Blend state
//-----------------------------------------------------------------------------

FORCEINLINE static D3D11_BLEND TranslateBlendFunc( ShaderBlendFactor_t blend )
{
	switch ( blend )
	{
	case SHADER_BLEND_ZERO:
		return D3D11_BLEND_ZERO;

	case SHADER_BLEND_ONE:
		return D3D11_BLEND_ONE;

	case SHADER_BLEND_DST_COLOR:
		return D3D11_BLEND_DEST_COLOR;

	case SHADER_BLEND_ONE_MINUS_DST_COLOR:
		return D3D11_BLEND_INV_DEST_COLOR;

	case SHADER_BLEND_SRC_ALPHA:
		return D3D11_BLEND_SRC_ALPHA;

	case SHADER_BLEND_ONE_MINUS_SRC_ALPHA:
		return D3D11_BLEND_INV_SRC_ALPHA;

	case SHADER_BLEND_DST_ALPHA:
		return D3D11_BLEND_DEST_ALPHA;

	case SHADER_BLEND_ONE_MINUS_DST_ALPHA:
		return D3D11_BLEND_INV_DEST_ALPHA;

	case SHADER_BLEND_SRC_COLOR:
		return D3D11_BLEND_SRC_COLOR;

	case SHADER_BLEND_ONE_MINUS_SRC_COLOR:
		return D3D11_BLEND_INV_SRC_COLOR;

	default:
		// Impossible
		return D3D11_BLEND_ZERO;
	}
}

static void GenerateBlendDesc( D3D11_BLEND_DESC *pDesc, const StatesDx11::RenderState &newState )
{
	const StatesDx11::ColorBlendAttrib &cba = newState.shadowState.colorBlendAttrib;
	const StatesDx11::ColorWriteAttrib &cwa = newState.shadowState.colorWriteAttrib;

	memset( pDesc, 0, sizeof( D3D11_BLEND_DESC ) );

	pDesc->AlphaToCoverageEnable		     = FALSE;
	pDesc->IndependentBlendEnable		     = cba.bIndependentAlphaBlend ? TRUE : FALSE;
	pDesc->RenderTarget[0].BlendEnable	     = cba.bBlendEnable ? TRUE : FALSE;
	pDesc->RenderTarget[0].BlendOp		     = (D3D11_BLEND_OP)cba.blendOp;
	pDesc->RenderTarget[0].BlendOpAlpha	     = (D3D11_BLEND_OP)cba.blendOpAlpha;
	pDesc->RenderTarget[0].DestBlend	     = TranslateBlendFunc( cba.destBlend );
	pDesc->RenderTarget[0].SrcBlend		     = TranslateBlendFunc( cba.srcBlend );
	pDesc->RenderTarget[0].DestBlendAlpha	     = TranslateBlendFunc( cba.destBlendAlpha );
	pDesc->RenderTarget[0].SrcBlendAlpha	     = TranslateBlendFunc( cba.srcBlendAlpha );
	pDesc->RenderTarget[0].RenderTargetWriteMask = cwa.colorWriteMask;
}

//-----------------------------------------------------------------------------
// Depth/Stencil state
//-----------------------------------------------------------------------------

static void GenerateDepthStencilDesc( D3D11_DEPTH_STENCIL_DESC *pDesc, const StatesDx11::RenderState &newState )
{
	const StatesDx11::StencilAttrib &stencil = newState.shadowState.stencilAttrib;
	const StatesDx11::DepthTestAttrib &depthTest = newState.shadowState.depthTestAttrib;
	const StatesDx11::DepthWriteAttrib &depthWrite = newState.shadowState.depthWriteAttrib;

	pDesc->FrontFace.StencilDepthFailOp = (D3D11_STENCIL_OP)stencil.stencilDepthFailOp;
	pDesc->FrontFace.StencilFailOp	    = (D3D11_STENCIL_OP)stencil.stencilFailOp;
	pDesc->FrontFace.StencilPassOp	    = (D3D11_STENCIL_OP)stencil.stencilPassOp;
	pDesc->FrontFace.StencilFunc	    = (D3D11_COMPARISON_FUNC)stencil.stencilFunc;

	// UNDONE: Backface?
	pDesc->BackFace.StencilFunc	   = D3D11_COMPARISON_ALWAYS;
	pDesc->BackFace.StencilFailOp	   = D3D11_STENCIL_OP_KEEP;
	pDesc->BackFace.StencilPassOp	   = D3D11_STENCIL_OP_KEEP;
	pDesc->BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;

	pDesc->StencilEnable	= stencil.bStencilEnable ? TRUE : FALSE;
	pDesc->StencilReadMask	= stencil.stencilReadMask;
	pDesc->StencilWriteMask = stencil.stencilWriteMask;

	pDesc->DepthEnable    = depthTest.bEnableDepthTest ? TRUE : FALSE;
	pDesc->DepthFunc      = (D3D11_COMPARISON_FUNC)depthTest.depthFunc;
	pDesc->DepthWriteMask = depthWrite.bEnableDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
}

//-----------------------------------------------------------------------------
//
// Shader API Dx11
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Class Factory
//-----------------------------------------------------------------------------
static CShaderAPIDx11 s_ShaderAPIDx11;
CShaderAPIDx11 *g_pShaderAPIDx11 = &s_ShaderAPIDx11;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIDx11, IShaderAPI,
				   SHADERAPI_INTERFACE_VERSION, s_ShaderAPIDx11 )

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIDx11, IDebugTextureInfo,
				   DEBUG_TEXTURE_INFO_VERSION, s_ShaderAPIDx11 )

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CShaderAPIDx11::CShaderAPIDx11() :
	m_Textures( 32 )
{
	m_ModifyTextureHandle = INVALID_SHADERAPI_TEXTURE_HANDLE;
	m_ModifyTextureLockedLevel = -1;
	m_bResettingRenderState = false;
	m_TargetState = StatesDx11::RenderState();
	m_State = m_TargetState;
	m_DynamicState = DynamicStateDx11_t();
	ClearShaderState( &m_DX11TargetState );
	ClearShaderState( &m_DX11State );
}

CShaderAPIDx11::~CShaderAPIDx11()
{
}

//-----------------------------------------------------------------------------
// Clears the shader state to a well-defined value
//-----------------------------------------------------------------------------
void CShaderAPIDx11::ClearShaderState( ShaderStateDx11_t *pState )
{
	memset( pState, 0, sizeof( ShaderStateDx11_t ) );
}

//-----------------------------------------------------------------------------
// Resets the render state
//-----------------------------------------------------------------------------
void CShaderAPIDx11::ResetRenderState( bool bFullReset )
{
	D3D11_RASTERIZER_DESC rDesc;
	memset( &rDesc, 0, sizeof( rDesc ) );
	rDesc.FillMode		    = D3D11_FILL_SOLID;
	rDesc.CullMode		    = D3D11_CULL_NONE;
	rDesc.FrontCounterClockwise = TRUE; // right-hand rule

	ID3D11RasterizerState *pRasterizerState;
	HRESULT hr = D3D11Device()->CreateRasterizerState( &rDesc, &pRasterizerState );
	Assert( !FAILED( hr ) );
	D3D11DeviceContext()->RSSetState( pRasterizerState );

	D3D11_DEPTH_STENCIL_DESC dsDesc;
	memset( &dsDesc, 0, sizeof( dsDesc ) );

	ID3D11DepthStencilState *pDepthStencilState;
	hr = D3D11Device()->CreateDepthStencilState( &dsDesc, &pDepthStencilState );
	Assert( !FAILED( hr ) );
	D3D11DeviceContext()->OMSetDepthStencilState( pDepthStencilState, 0 );

	D3D11_BLEND_DESC bDesc;
	memset( &bDesc, 0, sizeof( bDesc ) );
	D3D11_RENDER_TARGET_BLEND_DESC *rtbDesc = &bDesc.RenderTarget[0];
	rtbDesc->SrcBlend			= D3D11_BLEND_ONE;
	rtbDesc->DestBlend			= D3D11_BLEND_ZERO;
	rtbDesc->BlendOp			= D3D11_BLEND_OP_ADD;
	rtbDesc->SrcBlendAlpha			= D3D11_BLEND_ONE;
	rtbDesc->DestBlendAlpha			= D3D11_BLEND_ZERO;
	rtbDesc->BlendOpAlpha			= D3D11_BLEND_OP_ADD;
	rtbDesc->RenderTargetWriteMask		= D3D11_COLOR_WRITE_ENABLE_ALL;

	FLOAT pBlendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	ID3D11BlendState *pBlendState;
	hr = D3D11Device()->CreateBlendState( &bDesc, &pBlendState );
	Assert( !FAILED( hr ) );
	D3D11DeviceContext()->OMSetBlendState( pBlendState, pBlendFactor, 0xFFFFFFFF );
}

#define ShaderAttribChanged(varname) m_TargetState.shaderState.shaderAttrib.##varname != m_State.shaderState.shaderAttrib.##varname
#define TargetShaderVar(varname) m_TargetState.shaderState.shaderAttrib.##varname
#define CurrentShaderVar(varname) m_State.shaderState.shaderAttrib.##varname

//-----------------------------------------------------------------------------
// Issues state changes to D3D for states that have been modified
//-----------------------------------------------------------------------------
void CShaderAPIDx11::IssueStateChanges( bool bForce )
{
	// Don't bother committing anything if we're deactivated
	if ( g_pShaderDevice->IsDeactivated() )
		return;

	bool bStateChanged = false;

	bool bViewportsChanged = bForce || ( m_DX11TargetState.m_nViewportCount != m_DX11State.m_nViewportCount );
	if ( !bViewportsChanged && m_DX11TargetState.m_nViewportCount > 0 )
	{
		bViewportsChanged = memcmp( m_DX11TargetState.m_pViewports, m_DX11State.m_pViewports,
					    sizeof( D3D11_VIEWPORT ) * m_DX11TargetState.m_nViewportCount ) != 0;
	}
	if ( bViewportsChanged );
	{
		DoIssueViewports();
		bStateChanged = true;
	}

	if ( bForce || memcmp( &m_DX11TargetState.m_InputLayout, &m_DX11State.m_InputLayout, sizeof( ShaderInputLayoutStateDx11_t ) ) )
	{
		DoIssueInputLayout();
		bStateChanged = true;
	}

	bool bVertexBufferChanged = DoIssueVertexBuffer( bForce );
	if ( bVertexBufferChanged )
		bStateChanged = true;

	if ( bForce || memcmp( &m_DX11TargetState.m_IndexBuffer, &m_DX11State.m_IndexBuffer, sizeof( ShaderIndexBufferStateDx11_t ) ) )
	{
		DoIssueIndexBuffer();
		bStateChanged = true;
	}

	if ( bForce || m_DX11TargetState.m_Topology != m_DX11State.m_Topology )
	{
		DoIssueTopology();
		bStateChanged = true;
	}

	if ( bForce || m_DX11TargetState.m_pVertexShader != m_DX11State.m_pVertexShader )
	{
		DoIssueVertexShader();
		bStateChanged = true;
	}

	if ( bForce || m_DX11TargetState.m_pGeometryShader != m_DX11State.m_pGeometryShader )
	{
		DoIssueGeometryShader();
		bStateChanged = true;
	}

	if ( bForce || m_DX11TargetState.m_pPixelShader != m_DX11State.m_pPixelShader )
	{
		DoIssuePixelShader();
		bStateChanged = true;
	}

	if ( bForce || ShaderAttribChanged( numConstantBuffers ) ||
	     memcmp( TargetShaderVar( constantBuffers ), CurrentShaderVar( constantBuffers ), sizeof( ConstantBuffer_t ) * MAX_DX11_CBUFFERS ) )
	{
		DoIssueConstantBuffers();
		bStateChanged = true;
	}
	
	if ( bForce || memcmp( m_TargetState.shadowState.samplerAttrib.textures,
		     m_State.shadowState.samplerAttrib.textures,
		     sizeof( ShaderAPITextureHandle_t ) * MAX_DX11_SAMPLERS ) )
	{
		DoIssueTexture();
		bStateChanged = true;
	}

	// DepthStencilState is affected by DepthTestAttrib, DepthWriteAttrib, and StencilAttrib
	if ( bForce ||
	     AttribsDiffer( depthTestAttrib, StatesDx11::DepthTestAttrib ) ||
	     AttribsDiffer( depthWriteAttrib, StatesDx11::DepthWriteAttrib ) ||
	     AttribsDiffer( stencilAttrib, StatesDx11::StencilAttrib ) )
	{
		DoIssueDepthStencilState();
		bStateChanged = true;
	}

	// BlendState
	if ( bForce ||
	     AttribsDiffer( colorBlendAttrib, StatesDx11::ColorBlendAttrib ) ||
	     AttribsDiffer( colorWriteAttrib, StatesDx11::ColorWriteAttrib ) )
	{
		DoIssueBlendState();
		bStateChanged = true;
	}

	// Raster state - RenderModeAttrib, CullFaceAttrib, DepthOffsetAttrib, ScissorAttrib, AntialiasAttrib
	if ( bForce ||
	     AttribsDiffer( renderModeAttrib, StatesDx11::RenderModeAttrib ) ||
	     AttribsDiffer( cullFaceAttrib, StatesDx11::CullFaceAttrib ) ||
	     AttribsDiffer( depthOffsetAttrib, StatesDx11::DepthOffsetAttrib ) ||
	     AttribsDiffer( scissorAttrib, StatesDx11::ScissorAttrib ) ||
	     AttribsDiffer( antialiasAttrib, StatesDx11::AntialiasAttrib ) )
	{
		DoIssueRasterState();
		bStateChanged = true;
	}

	if ( bStateChanged )
	{
		m_State = m_TargetState;
		m_DX11State = m_DX11TargetState;
	}
}

//------------------------------
// Issue state changes
//------------------------------

void CShaderAPIDx11::DoIssueVertexShader()
{
	D3D11DeviceContext()->VSSetShader( m_DX11TargetState.m_pVertexShader, NULL, 0 );
}

void CShaderAPIDx11::DoIssuePixelShader()
{
	D3D11DeviceContext()->PSSetShader( m_DX11TargetState.m_pPixelShader, NULL, 0 );
}

void CShaderAPIDx11::DoIssueGeometryShader()
{
	D3D11DeviceContext()->GSSetShader( m_DX11TargetState.m_pGeometryShader, NULL, 0 );
}

void CShaderAPIDx11::DoIssueConstantBuffers()
{
	for ( int i = 0; i < TargetShaderVar( numConstantBuffers ); i++ )
	{
		m_DX11TargetState.m_pPSConstantBuffers[i] =
			(ID3D11Buffer *)g_pShaderDevice->GetConstantBuffer( TargetShaderVar( constantBuffers )[i] );
	}

	D3D11DeviceContext()->VSSetConstantBuffers( 0, TargetShaderVar( numConstantBuffers ),
						    m_DX11TargetState.m_pPSConstantBuffers );
	D3D11DeviceContext()->GSSetConstantBuffers( 0, TargetShaderVar( numConstantBuffers ),
						    m_DX11TargetState.m_pPSConstantBuffers );
	D3D11DeviceContext()->PSSetConstantBuffers( 0, TargetShaderVar( numConstantBuffers ),
						    m_DX11TargetState.m_pPSConstantBuffers );
}

void CShaderAPIDx11::DoIssueTexture()
{
	int nMaxSamplers = g_pHardwareConfig->GetSamplerCount();
	for ( int i = 0; i < nMaxSamplers; i++ )
	{
		ShaderAPITextureHandle_t handle = m_TargetState.shadowState.samplerAttrib.textures[i];
		if ( handle == INVALID_SHADERAPI_TEXTURE_HANDLE )
		{
			m_DX11TargetState.m_pSamplers[i] = NULL;
			m_DX11TargetState.m_pTextureViews[i] = NULL;
		}
		else
		{
			CTextureDx11 &tex = GetTexture( handle );
			m_DX11TargetState.m_pSamplers[i] = tex.GetSamplerState();
			m_DX11TargetState.m_pTextureViews[i] = tex.GetView();
		}
	}
	D3D11DeviceContext()->PSSetSamplers( 0, nMaxSamplers, m_DX11TargetState.m_pSamplers );
	D3D11DeviceContext()->PSSetShaderResources( 0, nMaxSamplers, m_DX11TargetState.m_pTextureViews );
}

void CShaderAPIDx11::DoIssueRasterState()
{
	m_DX11TargetState.m_pRasterState = NULL;

	// Clear out the existing state
	if ( m_DX11State.m_pRasterState )
	{
		m_DX11State.m_pRasterState->Release();
		m_DX11State.m_pRasterState = NULL;
	}

	D3D11_RASTERIZER_DESC desc;
	GenerateRasterizerDesc( &desc, m_TargetState );

	// NOTE: This does a search for existing matching state objects
	HRESULT hr = D3D11Device()->CreateRasterizerState( &desc, &m_DX11TargetState.m_pRasterState );
	if ( FAILED( hr ) )
	{
		Warning( "Unable to create rasterizer state object!\n" );
	}

	D3D11DeviceContext()->RSSetState( m_DX11TargetState.m_pRasterState );
}

void CShaderAPIDx11::DoIssueBlendState()
{
	m_DX11TargetState.m_pBlendState = NULL;

	// Clear out existing blend state
	if ( m_DX11State.m_pBlendState )
	{
		m_DX11State.m_pBlendState->Release();
		m_DX11State.m_pBlendState = NULL;
	}

	D3D11_BLEND_DESC desc;
	GenerateBlendDesc( &desc, m_TargetState );

	// NOTE: This does a search for existing matching state objects
	HRESULT hr = D3D11Device()->CreateBlendState( &desc, &m_DX11TargetState.m_pBlendState );
	if ( FAILED( hr ) )
	{
		Warning( "Unable to create Dx11 blend state object!\n" );
	}

	D3D11DeviceContext()->OMSetBlendState( m_DX11TargetState.m_pBlendState, m_TargetState.shadowState.colorBlendAttrib.blendColor,
					       m_TargetState.shadowState.colorBlendAttrib.sampleMask );
}

void CShaderAPIDx11::DoIssueDepthStencilState()
{
	m_DX11TargetState.m_pDepthStencilState = NULL;

	// Clear out current state
	if ( m_DX11State.m_pDepthStencilState )
	{
		m_DX11State.m_pDepthStencilState->Release();
		m_DX11State.m_pDepthStencilState = NULL;
	}

	D3D11_DEPTH_STENCIL_DESC desc;
	GenerateDepthStencilDesc( &desc, m_TargetState );

	// NOTE: This does a search for existing matching state objects
	HRESULT hr = D3D11Device()->CreateDepthStencilState( &desc, &m_DX11TargetState.m_pDepthStencilState );
	if ( FAILED( hr ) )
	{
		Warning( "Unable to create depth/stencil object!\n" );
	}

	D3D11DeviceContext()->OMSetDepthStencilState( m_DX11TargetState.m_pDepthStencilState, m_TargetState.shadowState.stencilAttrib.stencilRef );
}

bool CShaderAPIDx11::DoIssueVertexBuffer( bool bForce )
{
	ID3D11Buffer *ppVertexBuffers[MAX_DX11_STREAMS];
	UINT pStrides[MAX_DX11_STREAMS];
	UINT pOffsets[MAX_DX11_STREAMS];

	bool bChanged = false;

	UINT nFirstBuffer = 0;
	UINT nBufferCount = 0;
	bool bInMatch = true;
	for ( int i = 0; i < MAX_DX11_STREAMS; ++i )
	{
		const ShaderVertexBufferStateDx11_t &newState = m_DX11TargetState.m_pVertexBuffer[i];
		bool bMatch = !bForce && !memcmp( &newState, &m_DX11State.m_pVertexBuffer[i], sizeof( ShaderVertexBufferStateDx11_t ) );
		if ( !bMatch )
		{
			ppVertexBuffers[i] = newState.m_pBuffer;
			pStrides[i] = newState.m_nStride;
			pOffsets[i] = newState.m_nOffset;
			++nBufferCount;
			memcpy( &m_DX11State.m_pVertexBuffer[i], &newState, sizeof( ShaderVertexBufferStateDx11_t ) );
		}

		if ( bInMatch )
		{
			if ( !bMatch )
			{
				bInMatch = false;
				nFirstBuffer = i;
			}
			continue;
		}

		if ( bMatch )
		{
			bInMatch = true;
			bChanged = true;
			D3D11DeviceContext()->IASetVertexBuffers( nFirstBuffer, nBufferCount,
								  &ppVertexBuffers[nFirstBuffer], &pStrides[nFirstBuffer], &pOffsets[nFirstBuffer] );
			nBufferCount = 0;
		}
	}

	if ( !bInMatch )
	{
		D3D11DeviceContext()->IASetVertexBuffers( nFirstBuffer, nBufferCount,
							  &ppVertexBuffers[nFirstBuffer], &pStrides[nFirstBuffer], &pOffsets[nFirstBuffer] );
		bChanged = true;
	}

	return bChanged;
}

void CShaderAPIDx11::DoIssueIndexBuffer()
{
	D3D11DeviceContext()->IASetIndexBuffer( m_DX11TargetState.m_IndexBuffer.m_pBuffer, m_DX11TargetState.m_IndexBuffer.m_Format,
						m_DX11TargetState.m_IndexBuffer.m_nOffset );
}

void CShaderAPIDx11::DoIssueInputLayout()
{
	// FIXME: Deal with multiple streams
	ID3D11InputLayout *pInputLayout = g_pShaderDeviceDx11->GetInputLayout(
		m_DX11TargetState.m_InputLayout.m_hVertexShader, m_DX11TargetState.m_InputLayout.m_pVertexDecl[0] );
	D3D11DeviceContext()->IASetInputLayout( pInputLayout );
}

void CShaderAPIDx11::DoIssueTopology()
{
	D3D11DeviceContext()->IASetPrimitiveTopology( m_DX11TargetState.m_Topology );
}

void CShaderAPIDx11::DoIssueViewports()
{
	D3D11DeviceContext()->RSSetViewports( m_DX11TargetState.m_nViewportCount, m_DX11TargetState.m_pViewports );
}

//-------------------------------------------------------------------//

//-----------------------------------------------------------------------------
// Methods of IShaderDynamicAPI
//-----------------------------------------------------------------------------
void CShaderAPIDx11::GetBackBufferDimensions( int &nWidth, int &nHeight ) const
{
	g_pShaderDeviceDx11->GetBackBufferDimensions( nWidth, nHeight );
}

//-----------------------------------------------------------------------------
// Viewport-related methods
//-----------------------------------------------------------------------------
void CShaderAPIDx11::SetViewports( int nCount, const ShaderViewport_t *pViewports )
{
	nCount	= min( nCount, MAX_DX11_VIEWPORTS );
	m_DX11TargetState.m_nViewportCount = nCount;

	for ( int i = 0; i < nCount; ++i )
	{
		Assert( pViewports[i].m_nVersion == SHADER_VIEWPORT_VERSION );

		D3D11_VIEWPORT &viewport = m_DX11TargetState.m_pViewports[i];
		viewport.TopLeftX	 = pViewports[i].m_nTopLeftX;
		viewport.TopLeftY	 = pViewports[i].m_nTopLeftY;
		viewport.Width		 = pViewports[i].m_nWidth;
		viewport.Height		 = pViewports[i].m_nHeight;
		viewport.MinDepth	 = pViewports[i].m_flMinZ;
		viewport.MaxDepth	 = pViewports[i].m_flMaxZ;
	}
}

int CShaderAPIDx11::GetViewports( ShaderViewport_t *pViewports, int nMax ) const
{
	int nCount = m_DX11State.m_nViewportCount;
	if ( pViewports && nMax )
	{
		nCount = min( nCount, nMax );
		memcpy( pViewports, m_DX11State.m_pViewports, nCount * sizeof( ShaderViewport_t ) );
	}
	return nCount;
}

//-----------------------------------------------------------------------------
// Methods related to state objects
//-----------------------------------------------------------------------------
void CShaderAPIDx11::SetRasterState( const ShaderRasterState_t &state )
{
	Warning( "Unsupported CShaderAPIDx11::SetRasterState() called!\n" );
}

//-----------------------------------------------------------------------------
// Methods related to clearing buffers
//-----------------------------------------------------------------------------
void CShaderAPIDx11::ClearColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
	m_DX11TargetState.m_ClearColor[0] = r / 255.0f;
	m_DX11TargetState.m_ClearColor[1] = g / 255.0f;
	m_DX11TargetState.m_ClearColor[2] = b / 255.0f;
	m_DX11TargetState.m_ClearColor[3] = 1.0f;
}

void CShaderAPIDx11::ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	m_DX11TargetState.m_ClearColor[0] = r / 255.0f;
	m_DX11TargetState.m_ClearColor[1] = g / 255.0f;
	m_DX11TargetState.m_ClearColor[2] = b / 255.0f;
	m_DX11TargetState.m_ClearColor[3] = a / 255.0f;
}

void CShaderAPIDx11::ClearBuffers( bool bClearColor, bool bClearDepth, bool bClearStencil, int renderTargetWidth, int renderTargetHeight )
{
	// NOTE: State change commit isn't necessary since clearing doesn't use state
	//	IssueStateChanges();

	// FIXME: This implementation is totally bust0red [doesn't guarantee exact color specified]
	if ( bClearColor )
	{
		D3D11DeviceContext()->ClearRenderTargetView( D3D11RenderTargetView(), m_DX11TargetState.m_ClearColor );
	}
}

//-----------------------------------------------------------------------------
// Methods related to binding shaders
//-----------------------------------------------------------------------------
void CShaderAPIDx11::BindVertexShader( VertexShaderHandle_t hVertexShader )
{
	ID3D11VertexShader *pVertexShader = g_pShaderDeviceDx11->GetVertexShader( hVertexShader );
	m_DX11TargetState.m_pVertexShader = pVertexShader;
	m_DX11TargetState.m_InputLayout.m_hVertexShader = hVertexShader;
}

void CShaderAPIDx11::BindGeometryShader( GeometryShaderHandle_t hGeometryShader )
{
	ID3D11GeometryShader *pGeometryShader = g_pShaderDeviceDx11->GetGeometryShader( hGeometryShader );
	m_DX11TargetState.m_pGeometryShader = pGeometryShader;
}

void CShaderAPIDx11::BindPixelShader( PixelShaderHandle_t hPixelShader )
{
	ID3D11PixelShader *pPixelShader = g_pShaderDeviceDx11->GetPixelShader( hPixelShader );
	m_DX11TargetState.m_pPixelShader = pPixelShader;
}

void CShaderAPIDx11::BindVertexBuffer( int nStreamID, IVertexBuffer *pVertexBuffer, int nOffsetInBytes, int nFirstVertex, int nVertexCount, VertexFormat_t fmt, int nRepetitions )
{
	// FIXME: What to do about repetitions?
	CVertexBufferDx11 *pVertexBufferDx11 = static_cast<CVertexBufferDx11 *>( pVertexBuffer );

	ShaderVertexBufferStateDx11_t state;
	if ( pVertexBufferDx11 )
	{
		state.m_pBuffer = pVertexBufferDx11->GetDx11Buffer();
		state.m_nStride = pVertexBufferDx11->VertexSize();
	}
	else
	{
		state.m_pBuffer = NULL;
		state.m_nStride = 0;
	}
	state.m_nOffset = nOffsetInBytes;

	m_DX11TargetState.m_pVertexBuffer[nStreamID] = state;
	m_DX11TargetState.m_InputLayout.m_pVertexDecl[nStreamID] = fmt;
}

void CShaderAPIDx11::BindIndexBuffer( IIndexBuffer *pIndexBuffer, int nOffsetInBytes )
{
	CIndexBufferDx11 *pIndexBufferDx11 = static_cast<CIndexBufferDx11 *>( pIndexBuffer );

	ShaderIndexBufferStateDx11_t state;
	if ( pIndexBufferDx11 )
	{
		state.m_pBuffer = pIndexBufferDx11->GetDx11Buffer();
		state.m_Format	= ( pIndexBufferDx11->GetIndexFormat() == MATERIAL_INDEX_FORMAT_16BIT ) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	}
	else
	{
		state.m_pBuffer = NULL;
		state.m_Format	= DXGI_FORMAT_R16_UINT;
	}
	state.m_nOffset = nOffsetInBytes;

	m_DX11TargetState.m_IndexBuffer = state;
}

//-----------------------------------------------------------------------------
// Unbinds resources because they are about to be deleted
//-----------------------------------------------------------------------------
void CShaderAPIDx11::Unbind( VertexShaderHandle_t hShader )
{
	ID3D11VertexShader *pShader = g_pShaderDeviceDx11->GetVertexShader( hShader );
	Assert( pShader );
	if ( m_DX11TargetState.m_pVertexShader == pShader )
	{
		BindVertexShader( VERTEX_SHADER_HANDLE_INVALID );
	}
	if ( m_DX11State.m_pVertexShader == pShader )
	{
		IssueStateChanges();
	}
}

void CShaderAPIDx11::Unbind( GeometryShaderHandle_t hShader )
{
	ID3D11GeometryShader *pShader = g_pShaderDeviceDx11->GetGeometryShader( hShader );
	Assert( pShader );
	if ( m_DX11TargetState.m_pGeometryShader == pShader )
	{
		BindGeometryShader( GEOMETRY_SHADER_HANDLE_INVALID );
	}
	if ( m_DX11State.m_pGeometryShader == pShader )
	{
		IssueStateChanges();
	}
}

void CShaderAPIDx11::Unbind( PixelShaderHandle_t hShader )
{
	ID3D11PixelShader *pShader = g_pShaderDeviceDx11->GetPixelShader( hShader );
	Assert( pShader );
	if ( m_DX11TargetState.m_pPixelShader == pShader )
	{
		BindPixelShader( PIXEL_SHADER_HANDLE_INVALID );
	}
	if ( m_DX11State.m_pPixelShader == pShader )
	{
		IssueStateChanges();
	}
}

void CShaderAPIDx11::UnbindVertexBuffer( ID3D11Buffer *pBuffer )
{
	Assert( pBuffer );

	for ( int i = 0; i < MAX_DX11_STREAMS; ++i )
	{
		if ( m_DX11TargetState.m_pVertexBuffer[i].m_pBuffer == pBuffer )
		{
			BindVertexBuffer( i, NULL, 0, 0, 0, VERTEX_POSITION, 0 );
		}
	}
	for ( int i = 0; i < MAX_DX11_STREAMS; ++i )
	{
		if ( m_DX11State.m_pVertexBuffer[i].m_pBuffer == pBuffer )
		{
			IssueStateChanges();
			break;
		}
	}
}

void CShaderAPIDx11::UnbindIndexBuffer( ID3D11Buffer *pBuffer )
{
	Assert( pBuffer );

	if ( m_DX11TargetState.m_IndexBuffer.m_pBuffer == pBuffer )
	{
		BindIndexBuffer( NULL, 0 );
	}
	if ( m_DX11State.m_IndexBuffer.m_pBuffer == pBuffer )
	{
		IssueStateChanges();
	}
}

//-----------------------------------------------------------------------------
// Sets the topology state
//-----------------------------------------------------------------------------
void CShaderAPIDx11::SetTopology( MaterialPrimitiveType_t topology )
{
	D3D11_PRIMITIVE_TOPOLOGY d3dTopology;
	switch ( topology )
	{
	case MATERIAL_POINTS:
		d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		break;

	case MATERIAL_LINES:
		d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		break;

	case MATERIAL_TRIANGLES:
		d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;

	case MATERIAL_TRIANGLE_STRIP:
		d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		break;

	case MATERIAL_LINE_STRIP:
		d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
		break;

	default:
	case MATERIAL_LINE_LOOP:
	case MATERIAL_POLYGON:
	case MATERIAL_QUADS:
		Assert( 0 );
		d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		break;
	}

	m_DX11TargetState.m_Topology = d3dTopology;
}

//-----------------------------------------------------------------------------
// Main entry point for rendering
//-----------------------------------------------------------------------------
void CShaderAPIDx11::Draw( MaterialPrimitiveType_t primitiveType, int nFirstIndex, int nIndexCount )
{
	SetTopology( primitiveType );

	IssueStateChanges();

	// FIXME: How do I set the base vertex location!?
	D3D11DeviceContext()->DrawIndexed( (UINT)nIndexCount, (UINT)nFirstIndex, 0 );
}

bool CShaderAPIDx11::OnDeviceInit()
{
	ResetRenderState();
	return true;
}

//-----------------------------------------------------------------------------
//
// Abandon all hope below this point
//
//-----------------------------------------------------------------------------

bool CShaderAPIDx11::DoRenderTargetsNeedSeparateDepthBuffer() const
{
	return false;
}

// Can we download textures?
bool CShaderAPIDx11::CanDownloadTextures() const
{
	return false;
}

// Used to clear the transition table when we know it's become invalid.
void CShaderAPIDx11::ClearSnapshots()
{
	g_pShaderShadowDx11->m_ShadowStateCache.RemoveAll();
}

// Sets the default *dynamic* state
void CShaderAPIDx11::SetDefaultState()
{
}

// Returns the snapshot id for the current shadow state
StateSnapshot_t CShaderAPIDx11::TakeSnapshot()
{
	return g_pShaderShadowDx11->FindOrCreateSnapshot();
}

// Returns true if the state snapshot is transparent
bool CShaderAPIDx11::IsTranslucent( StateSnapshot_t id ) const
{
	return ( id & TRANSLUCENT ) != 0;
}

bool CShaderAPIDx11::IsAlphaTested( StateSnapshot_t id ) const
{
	return ( id & ALPHATESTED ) != 0;
}

bool CShaderAPIDx11::IsDepthWriteEnabled( StateSnapshot_t id ) const
{
	return ( id & DEPTHWRITE ) != 0;
}

bool CShaderAPIDx11::UsesVertexAndPixelShaders( StateSnapshot_t id ) const
{
	return ( id & VERTEX_AND_PIXEL_SHADERS ) != 0;
}

// Gets the vertex format for a set of snapshot ids
VertexFormat_t CShaderAPIDx11::ComputeVertexFormat( int numSnapshots, StateSnapshot_t *pIds ) const
{
	return 0;
}

// Gets the vertex format for a set of snapshot ids
VertexFormat_t CShaderAPIDx11::ComputeVertexUsage( int numSnapshots, StateSnapshot_t *pIds ) const
{
	return 0;
}

// Uses a state snapshot
void CShaderAPIDx11::UseSnapshot( StateSnapshot_t snapshot )
{
	StatesDx11::RenderState entry = g_pShaderShadowDx11->m_ShadowStateCache.Element( snapshot );
	m_TargetState = entry;

	ShaderManager()->SetVertexShaderIndex( entry.shaderState.shaderAttrib.vertexShaderIndex );
	ShaderManager()->SetVertexShader( entry.shaderState.shaderAttrib.vertexShader );

	ShaderManager()->SetPixelShaderIndex( entry.shaderState.shaderAttrib.pixelShaderIndex );
	ShaderManager()->SetPixelShader( entry.shaderState.shaderAttrib.pixelShader );
}

// Sets the color to modulate by
void CShaderAPIDx11::Color3f( float r, float g, float b )
{
}

void CShaderAPIDx11::Color3fv( float const *pColor )
{
}

void CShaderAPIDx11::Color4f( float r, float g, float b, float a )
{
}

void CShaderAPIDx11::Color4fv( float const *pColor )
{
}

// Faster versions of color
void CShaderAPIDx11::Color3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPIDx11::Color3ubv( unsigned char const *rgb )
{
}

void CShaderAPIDx11::Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
}

void CShaderAPIDx11::Color4ubv( unsigned char const *rgba )
{
}

void CShaderAPIDx11::GetStandardTextureDimensions( int *pWidth, int *pHeight, StandardTextureId_t id )
{
	ShaderUtil()->GetStandardTextureDimensions( pWidth, pHeight, id );
}

// The shade mode
void CShaderAPIDx11::ShadeMode( ShaderShadeMode_t mode )
{
}

// Binds a particular material to render with
void CShaderAPIDx11::Bind( IMaterial *pMaterial )
{
	//m_pMaterial = pMaterial->Get;
}

// Cull mode
void CShaderAPIDx11::CullMode( MaterialCullMode_t cullMode )
{
}

void CShaderAPIDx11::ForceDepthFuncEquals( bool bEnable )
{
}

// Forces Z buffering on or off
void CShaderAPIDx11::OverrideDepthEnable( bool bEnable, bool bDepthEnable )
{
}

//legacy fast clipping linkage
void CShaderAPIDx11::SetHeightClipZ( float z )
{
}

void CShaderAPIDx11::SetHeightClipMode( enum MaterialHeightClipMode_t heightClipMode )
{
}

// Sets the lights
void CShaderAPIDx11::SetLight( int lightNum, const LightDesc_t &desc )
{
	LOCK_SHADERAPI();

	m_DynamicState.m_Lights[lightNum] = desc;
	if ( desc.m_Type != MATERIAL_LIGHT_DISABLE )
	{
		m_DynamicState.m_NumLights++;
	}
	
}

void CShaderAPIDx11::SetAmbientLight( float r, float g, float b )
{
	Warning( "Unsupported CShaderAPIDx11::SetAmbientLight() called!\n" );
}

void CShaderAPIDx11::SetAmbientLightCube( Vector4D cube[6] )
{
	LOCK_SHADERAPI();

	if ( memcmp( &m_DynamicState.m_AmbientLightCube[0][0], cube, 6 * sizeof( Vector4D ) ) )
	{
		memcpy( &m_DynamicState.m_AmbientLightCube[0][0], cube, 6 * sizeof( Vector4D ) );
	}
}

// Get lights
int CShaderAPIDx11::GetMaxLights( void ) const
{
	return HardwareConfig()->MaxNumLights();
}

const LightDesc_t &CShaderAPIDx11::GetLight( int lightNum ) const
{
	return m_DynamicState.m_Lights[lightNum];
}

// Render state for the ambient light cube (vertex shaders)
void CShaderAPIDx11::SetVertexShaderStateAmbientLightCube()
{
}

void CShaderAPIDx11::SetSkinningMatrices()
{
}

// Lightmap texture binding
void CShaderAPIDx11::BindLightmap( TextureStage_t stage )
{
}

void CShaderAPIDx11::BindBumpLightmap( TextureStage_t stage )
{
}

void CShaderAPIDx11::BindFullbrightLightmap( TextureStage_t stage )
{
}

void CShaderAPIDx11::BindWhite( TextureStage_t stage )
{
}

void CShaderAPIDx11::BindBlack( TextureStage_t stage )
{
}

void CShaderAPIDx11::BindGrey( TextureStage_t stage )
{
}

// Gets the lightmap dimensions
void CShaderAPIDx11::GetLightmapDimensions( int *w, int *h )
{
	g_pShaderUtil->GetLightmapDimensions( w, h );
}

// Special system flat normal map binding.
void CShaderAPIDx11::BindFlatNormalMap( TextureStage_t stage )
{
}

void CShaderAPIDx11::BindNormalizationCubeMap( TextureStage_t stage )
{
}

void CShaderAPIDx11::BindSignedNormalizationCubeMap( TextureStage_t stage )
{
}

void CShaderAPIDx11::BindFBTexture( TextureStage_t stage, int textureIndex )
{
}

// Flushes any primitives that are buffered
void CShaderAPIDx11::FlushBufferedPrimitives()
{
}

// Creates/destroys Mesh
IMesh *CShaderAPIDx11::CreateStaticMesh( VertexFormat_t fmt, const char *pTextureBudgetGroup, IMaterial *pMaterial )
{
	return m_pRenderingMesh;
}

void CShaderAPIDx11::DestroyStaticMesh( IMesh *mesh )
{
}

// Gets the dynamic mesh; note that you've got to render the mesh
// before calling this function a second time. Clients should *not*
// call DestroyStaticMesh on the mesh returned by this call.
IMesh *CShaderAPIDx11::GetDynamicMesh( IMaterial *pMaterial, int nHWSkinBoneCount, bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride )
{
	Assert( ( pMaterial == NULL ) || ( (IMaterialInternal *)pMaterial )->IsRealTimeVersion() );
	return m_pRenderingMesh;
}

IMesh *CShaderAPIDx11::GetDynamicMeshEx( IMaterial *pMaterial, VertexFormat_t fmt, int nHWSkinBoneCount, bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride )
{
	// UNDONE: support compressed dynamic meshes if needed (pro: less VB memory, con: time spent compressing)
	Assert( CompressionType( pVertexOverride->GetVertexFormat() ) != VERTEX_COMPRESSION_NONE );
	Assert( ( pMaterial == NULL ) || ( (IMaterialInternal *)pMaterial )->IsRealTimeVersion() );
	return m_pRenderingMesh;
}

IMesh *CShaderAPIDx11::GetFlexMesh()
{
	return m_pRenderingMesh;
}

// Begins a rendering pass that uses a state snapshot
void CShaderAPIDx11::BeginPass( StateSnapshot_t snapshot )
{
}

// Renders a single pass of a material
void CShaderAPIDx11::RenderPass( int nPass, int nPassCount )
{
}

// stuff related to matrix stacks
void CShaderAPIDx11::MatrixMode( MaterialMatrixMode_t matrixMode )
{
}

void CShaderAPIDx11::PushMatrix()
{
}

void CShaderAPIDx11::PopMatrix()
{
}

void CShaderAPIDx11::LoadMatrix( float *m )
{
}

void CShaderAPIDx11::MultMatrix( float *m )
{
}

void CShaderAPIDx11::MultMatrixLocal( float *m )
{
}

void CShaderAPIDx11::GetMatrix( MaterialMatrixMode_t matrixMode, float *dst )
{
}

void CShaderAPIDx11::LoadIdentity( void )
{
}

void CShaderAPIDx11::LoadCameraToWorld( void )
{
}

void CShaderAPIDx11::Ortho( double left, double top, double right, double bottom, double zNear, double zFar )
{
}

void CShaderAPIDx11::PerspectiveX( double fovx, double aspect, double zNear, double zFar )
{
}

void CShaderAPIDx11::PerspectiveOffCenterX( double fovx, double aspect, double zNear, double zFar, double bottom, double top, double left, double right )
{
}

void CShaderAPIDx11::PickMatrix( int x, int y, int width, int height )
{
}

void CShaderAPIDx11::Rotate( float angle, float x, float y, float z )
{
}

void CShaderAPIDx11::Translate( float x, float y, float z )
{
}

void CShaderAPIDx11::Scale( float x, float y, float z )
{
}

void CShaderAPIDx11::ScaleXY( float x, float y )
{
}

// Fog methods...
void CShaderAPIDx11::FogMode( MaterialFogMode_t fogMode )
{
}

void CShaderAPIDx11::FogStart( float fStart )
{
}

void CShaderAPIDx11::FogEnd( float fEnd )
{
}

void CShaderAPIDx11::SetFogZ( float fogZ )
{
}

void CShaderAPIDx11::FogMaxDensity( float flMaxDensity )
{
}

void CShaderAPIDx11::GetFogDistances( float *fStart, float *fEnd, float *fFogZ )
{
}

void CShaderAPIDx11::SceneFogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPIDx11::SceneFogMode( MaterialFogMode_t fogMode )
{
}

void CShaderAPIDx11::GetSceneFogColor( unsigned char *rgb )
{
}

MaterialFogMode_t CShaderAPIDx11::GetSceneFogMode()
{
	return MATERIAL_FOG_NONE;
}

int CShaderAPIDx11::GetPixelFogCombo()
{
	return 0; //FIXME
}

void CShaderAPIDx11::FogColor3f( float r, float g, float b )
{
}

void CShaderAPIDx11::FogColor3fv( float const *rgb )
{
}

void CShaderAPIDx11::FogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPIDx11::FogColor3ubv( unsigned char const *rgb )
{
}

void CShaderAPIDx11::Viewport( int x, int y, int width, int height )
{
}

void CShaderAPIDx11::GetViewport( int &x, int &y, int &width, int &height ) const
{
}

// Sets the vertex and pixel shaders
void CShaderAPIDx11::SetVertexShaderIndex( int vshIndex )
{
}

void CShaderAPIDx11::SetPixelShaderIndex( int pshIndex )
{
}

// Sets the constant register for vertex and pixel shaders
void CShaderAPIDx11::SetVertexShaderConstant( int var, float const *pVec, int numConst, bool bForce )
{
	Warning( "Unsupported CShaderAPIDx11::SetVertexShaderConstant() called!\n" );
}

void CShaderAPIDx11::SetPixelShaderConstant( int var, float const *pVec, int numConst, bool bForce )
{
	Warning( "Unsupported CShaderAPIDx11::SetPixelShaderConstant() called!\n" );
}

void CShaderAPIDx11::InvalidateDelayedShaderConstants( void )
{
	Warning( "Unsupported CShaderAPIDx11::InvalidateDelayedShaderConstants() called!\n" );
}

//Set's the linear->gamma conversion textures to use for this hardware for both srgb writes enabled and disabled(identity)
void CShaderAPIDx11::SetLinearToGammaConversionTextures( ShaderAPITextureHandle_t hSRGBWriteEnabledTexture, ShaderAPITextureHandle_t hIdentityTexture )
{
	Warning( "Unsupported CShaderAPIDx11::SetLinearToGammaConversionTextures() called!\n" );
}

// Returns the nearest supported format
ImageFormat CShaderAPIDx11::GetNearestSupportedFormat( ImageFormat fmt ) const
{
	return fmt;
}

ImageFormat CShaderAPIDx11::GetNearestRenderTargetFormat( ImageFormat fmt ) const
{
	return fmt;
}

// Sets the texture state
void CShaderAPIDx11::BindTexture( Sampler_t stage, ShaderAPITextureHandle_t textureHandle )
{
	m_TargetState.shadowState.samplerAttrib.textures[stage] = textureHandle;
}

// Indicates we're going to be modifying this texture
// TexImage2D, TexSubImage2D, TexWrap, TexMinFilter, and TexMagFilter
// all use the texture specified by this function.
void CShaderAPIDx11::ModifyTexture( ShaderAPITextureHandle_t textureHandle )
{
	LOCK_SHADERAPI();

	// Can't do this if we're locked!
	Assert( m_ModifyTextureLockedLevel < 0 );

	m_ModifyTextureHandle = textureHandle;

	// If we've got a multi-copy texture, we need to up the current copy count
	CTextureDx11 &tex = GetTexture( textureHandle );
	if ( tex.m_NumCopies > 1 )
	{
		// Each time we modify a texture, we'll want to switch texture
		// as soon as a TexImage2D call is made...
		tex.m_SwitchNeeded = true;
	}
}

// Texture management methods
void CShaderAPIDx11::TexImage2D( int level, int cubeFace, ImageFormat dstFormat, int zOffset, int width, int height,
				 ImageFormat srcFormat, bool bSrcIsTiled, void *imageData )
{
}

void CShaderAPIDx11::TexSubImage2D( int level, int cubeFace, int xOffset, int yOffset, int zOffset, int width, int height,
				    ImageFormat srcFormat, int srcStride, bool bSrcIsTiled, void *imageData )
{
}

bool CShaderAPIDx11::TexLock( int level, int cubeFaceID, int xOffset, int yOffset,
			      int width, int height, CPixelWriter &writer )
{
	return false;
}

void CShaderAPIDx11::TexUnlock()
{
}

// These are bound to the texture, not the texture environment
void CShaderAPIDx11::TexMinFilter( ShaderTexFilterMode_t texFilterMode )
{
	LOCK_SHADERAPI();

	ShaderAPITextureHandle_t hModifyTexture = m_ModifyTextureHandle;
	if ( hModifyTexture == INVALID_SHADERAPI_TEXTURE_HANDLE )
		return;

	GetTexture( hModifyTexture ).SetMinFilter( texFilterMode );
}

void CShaderAPIDx11::TexMagFilter( ShaderTexFilterMode_t texFilterMode )
{
	LOCK_SHADERAPI();

	ShaderAPITextureHandle_t hModifyTexture = m_ModifyTextureHandle;
	if ( hModifyTexture == INVALID_SHADERAPI_TEXTURE_HANDLE )
		return;

	GetTexture( hModifyTexture ).SetMagFilter( texFilterMode );
}

void CShaderAPIDx11::TexWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode )
{
	LOCK_SHADERAPI();

	ShaderAPITextureHandle_t hModifyTexture = m_ModifyTextureHandle;
	if ( hModifyTexture == INVALID_SHADERAPI_TEXTURE_HANDLE )
		return;

	GetTexture( hModifyTexture ).SetWrap( coord, wrapMode );
}

void CShaderAPIDx11::TexSetPriority( int priority )
{
	Warning( "Unsupported CShaderAPIDx11::SetTexPriority() called!\n" );
}

ShaderAPITextureHandle_t CShaderAPIDx11::CreateTexture(
    int width,
    int height,
    int depth,
    ImageFormat dstImageFormat,
    int numMipLevels,
    int numCopies,
    int flags,
    const char *pDebugName,
    const char *pTextureGroupName )
{
	ShaderAPITextureHandle_t handle;
	CreateTextures( &handle, 1, width, height, depth, dstImageFormat, numMipLevels, numCopies, flags, pDebugName, pTextureGroupName );
	return handle;
}

void CShaderAPIDx11::CreateTextureHandles( ShaderAPITextureHandle_t *handles, int count )
{
	if ( count <= 0 )
		return;

	MEM_ALLOC_CREDIT();

	int idxCreating = 0;
	ShaderAPITextureHandle_t hTexture;
	for ( hTexture = m_Textures.Head(); hTexture != m_Textures.InvalidIndex(); hTexture = m_Textures.Next( hTexture ) )
	{
		if ( !( m_Textures[hTexture].m_nFlags & CTextureDx11::IS_ALLOCATED ) )
		{
			handles[idxCreating++] = hTexture;
			if ( idxCreating >= count )
				return;
		}
	}

	while ( idxCreating < count )
		handles[idxCreating++] = m_Textures.AddToTail();
}

void CShaderAPIDx11::CreateTextures(
    ShaderAPITextureHandle_t *pHandles,
    int count,
    int width,
    int height,
    int depth,
    ImageFormat dstImageFormat,
    int numMipLevels,
    int numCopies,
    int flags,
    const char *pDebugName,
    const char *pTextureGroupName )
{
	LOCK_SHADERAPI();

	// Create a set of texture handles
	CreateTextureHandles( pHandles, count );
	CTextureDx11 **arrTxp = (CTextureDx11 **)stackalloc( count * sizeof( CTextureDx11 * ) );

	for ( int idxFrame = 0; idxFrame < count; ++idxFrame )
	{
		arrTxp[idxFrame]	  = &GetTexture( pHandles[idxFrame] );
		CTextureDx11 *pTexture	  = arrTxp[idxFrame];
		pTexture->SetupTexture2D( width, height, depth, count, idxFrame, flags,
					  numCopies, numMipLevels, dstImageFormat );

	}
}

CTextureDx11 &CShaderAPIDx11::GetTexture( ShaderAPITextureHandle_t handle )
{
	return m_Textures[handle];
}

ShaderAPITextureHandle_t CShaderAPIDx11::CreateTextureHandle()
{
	ShaderAPITextureHandle_t handle;
	CreateTextureHandles( &handle, 1 );
	return handle;
}

ShaderAPITextureHandle_t CShaderAPIDx11::CreateDepthTexture( ImageFormat renderFormat, int width, int height, const char *pDebugName, bool bTexture )
{
	LOCK_SHADERAPI();

	ShaderAPITextureHandle_t i = CreateTextureHandle();
	CTextureDx11 *pTexture = &GetTexture( i );
	pTexture->SetupDepthTexture( renderFormat, width, height, pDebugName, bTexture );

	return i;
}

void CShaderAPIDx11::DeleteTexture( ShaderAPITextureHandle_t textureHandle )
{
	LOCK_SHADERAPI();

	if ( !TextureIsAllocated( textureHandle ) )
	{
		// already deallocated
		return;
	}
	
	// Unbind it!

	// Delete it baby
	GetTexture( textureHandle ).Delete();
}

bool CShaderAPIDx11::IsTexture( ShaderAPITextureHandle_t textureHandle )
{
	LOCK_SHADERAPI();

	if ( !TextureIsAllocated( textureHandle ) )
		return false;

	if ( GetTexture( textureHandle ).m_nFlags & CTextureDx11::IS_DEPTH_STENCIL )
	{
		return GetTexture( textureHandle ).GetDepthStencilView() != 0;
	}
	else if ( ( GetTexture( textureHandle ).m_NumCopies == 1 && GetTexture( textureHandle ).GetTexture() != 0 ) ||
		( GetTexture( textureHandle ).m_NumCopies > 1 && GetTexture( textureHandle ).GetTexture( 0 ) != 0 ) )
	{
		return true;
	}

	return false;
}

void CShaderAPIDx11::SetAnisotropicLevel( int nAnisotropyLevel )
{
	LOCK_SHADERAPI();

	// NOTE: This must be called before the rest of the code in this function so
	//       anisotropic can be set per-texture to force it on! This will also avoid
	//       a possible infinite loop that existed before.
	g_pShaderUtil->NoteAnisotropicLevel( nAnisotropyLevel );

	// Never set this to 1. In the case we want it set to 1, we will use this to override
	//   aniso per-texture, so set it to something reasonable
	if ( nAnisotropyLevel > g_pHardwareConfig->Caps().m_nMaxAnisotropy || nAnisotropyLevel <= 1 )
	{
		// Set it to 1/4 the max but between 2-8
		nAnisotropyLevel = max( 2, min( 8, ( g_pHardwareConfig->Caps().m_nMaxAnisotropy / 4 ) ) );
	}

	// Set the D3D max aninsotropy state for all samplers
	for ( ShaderAPITextureHandle_t handle = m_Textures.Head();
	      handle != m_Textures.InvalidIndex();
	      handle = m_Textures.Next( handle ) )
	{
		CTextureDx11 &tex = GetTexture( handle );
		tex.SetAnisotropicLevel( nAnisotropyLevel );
	}
}

bool CShaderAPIDx11::IsTextureResident( ShaderAPITextureHandle_t textureHandle )
{
	return true;
}

// stuff that isn't to be used from within a shader
void CShaderAPIDx11::ClearBuffersObeyStencil( bool bClearColor, bool bClearDepth )
{
}

void CShaderAPIDx11::PerformFullScreenStencilOperation( void )
{
}

void CShaderAPIDx11::ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat )
{
}

void CShaderAPIDx11::ReadPixels( Rect_t *pSrcRect, Rect_t *pDstRect, unsigned char *data, ImageFormat dstFormat, int nDstStride )
{
}

void CShaderAPIDx11::FlushHardware()
{
}

// Set the number of bone weights
void CShaderAPIDx11::SetNumBoneWeights( int numBones )
{
}

// Selection mode methods
int CShaderAPIDx11::SelectionMode( bool selectionMode )
{
	return 0;
}

void CShaderAPIDx11::SelectionBuffer( unsigned int *pBuffer, int size )
{
}

void CShaderAPIDx11::ClearSelectionNames()
{
}

void CShaderAPIDx11::LoadSelectionName( int name )
{
}

void CShaderAPIDx11::PushSelectionName( int name )
{
}

void CShaderAPIDx11::PopSelectionName()
{
}

// Use this to get the mesh builder that allows us to modify vertex data
CMeshBuilder *CShaderAPIDx11::GetVertexModifyBuilder()
{
	return 0;
}

// Board-independent calls, here to unify how shaders set state
// Implementations should chain back to IShaderUtil->BindTexture(), etc.

// Use this to begin and end the frame
void CShaderAPIDx11::BeginFrame()
{
}

void CShaderAPIDx11::EndFrame()
{
}

// returns the current time in seconds....
double CShaderAPIDx11::CurrentTime() const
{
	return Sys_FloatTime();
}

// Get the current camera position in world space.
void CShaderAPIDx11::GetWorldSpaceCameraPosition( float *pPos ) const
{
}

void CShaderAPIDx11::ForceHardwareSync( void )
{
}

void CShaderAPIDx11::SetClipPlane( int index, const float *pPlane )
{
}

void CShaderAPIDx11::EnableClipPlane( int index, bool bEnable )
{
}

void CShaderAPIDx11::SetFastClipPlane( const float *pPlane )
{
}

void CShaderAPIDx11::EnableFastClip( bool bEnable )
{
}

int CShaderAPIDx11::GetCurrentNumBones( void ) const
{
	return 0;
}

// Is hardware morphing enabled?
bool CShaderAPIDx11::IsHWMorphingEnabled() const
{
	return false;
}

int CShaderAPIDx11::GetCurrentLightCombo( void ) const
{
	return 0;
}

int CShaderAPIDx11::MapLightComboToPSLightCombo( int nLightCombo ) const
{
	return 0;
}

MaterialFogMode_t CShaderAPIDx11::GetCurrentFogType( void ) const
{
	return MATERIAL_FOG_NONE;
}

void CShaderAPIDx11::RecordString( const char *pStr )
{
}

void CShaderAPIDx11::DestroyVertexBuffers( bool bExitingLevel )
{
}

int CShaderAPIDx11::GetCurrentDynamicVBSize( void )
{
	return 0;
}

void CShaderAPIDx11::EvictManagedResources()
{
}

void CShaderAPIDx11::ReleaseShaderObjects()
{
}

void CShaderAPIDx11::RestoreShaderObjects()
{
}

void CShaderAPIDx11::SetTextureTransformDimension( TextureStage_t textureStage, int dimension, bool projected )
{
	Warning( "Unsupported CShaderAPIDx11::SetTextureTransformDimension() called!\n" );
}

void CShaderAPIDx11::SetBumpEnvMatrix( TextureStage_t textureStage, float m00, float m01, float m10, float m11 )
{
	Warning( "Unsupported CShaderAPIDx11::SetBumpEnvMatrix() called!\n" );
}

void CShaderAPIDx11::SyncToken( const char *pToken )
{
}

// Stencils

void CShaderAPIDx11::SetStencilEnable( bool onoff )
{
	m_TargetState.shadowState.stencilAttrib.bStencilEnable = onoff;
}

void CShaderAPIDx11::SetStencilFailOperation( StencilOperation_t op )
{
	m_TargetState.shadowState.stencilAttrib.stencilFailOp = (ShaderStencilOp_t)op;
}

void CShaderAPIDx11::SetStencilZFailOperation( StencilOperation_t op )
{
	m_TargetState.shadowState.stencilAttrib.stencilDepthFailOp = (ShaderStencilOp_t)op;
}

void CShaderAPIDx11::SetStencilPassOperation( StencilOperation_t op )
{
	m_TargetState.shadowState.stencilAttrib.stencilPassOp = (ShaderStencilOp_t)op;
}

void CShaderAPIDx11::SetStencilCompareFunction( StencilComparisonFunction_t cmpfn )
{
	m_TargetState.shadowState.stencilAttrib.stencilFunc = (ShaderStencilFunc_t)cmpfn;
}

void CShaderAPIDx11::SetStencilReferenceValue( int ref )
{
	m_TargetState.shadowState.stencilAttrib.stencilRef = ref;
}

void CShaderAPIDx11::SetStencilTestMask( uint32 msk )
{
	m_TargetState.shadowState.stencilAttrib.stencilReadMask = msk;
}

void CShaderAPIDx11::SetStencilWriteMask( uint32 msk )
{
	m_TargetState.shadowState.stencilAttrib.stencilWriteMask = msk;
}

IDirect3DBaseTexture *CShaderAPIDx11::GetD3DTexture( ShaderAPITextureHandle_t handle )
{
	if ( handle == INVALID_SHADERAPI_TEXTURE_HANDLE )
		return NULL;

	CTextureDx11 &tex = GetTexture( handle );
	if ( tex.m_NumCopies == 1 )
		return tex.GetTexture();
	else
		return tex.GetTexture( tex.m_CurrentCopy );
}