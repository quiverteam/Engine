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


//-----------------------------------------------------------------------------
// Methods related to queuing functions to be called prior to rendering
//-----------------------------------------------------------------------------
CFunctionCommit::CFunctionCommit()
{
	m_pCommitFlags = NULL;
	m_nCommitBufferSize = 0;
}

CFunctionCommit::~CFunctionCommit()
{
	if ( m_pCommitFlags )
	{
		delete[] m_pCommitFlags;
		m_pCommitFlags = NULL;
	}
}

void CFunctionCommit::Init( int nFunctionCount )
{
	m_nCommitBufferSize = ( nFunctionCount + 7 ) >> 3;
	Assert( !m_pCommitFlags );
	m_pCommitFlags = new unsigned char[ m_nCommitBufferSize ];
	memset( m_pCommitFlags, 0, m_nCommitBufferSize );
}


//-----------------------------------------------------------------------------
// Methods related to queuing functions to be called per-(pMesh->Draw call) or per-pass
//-----------------------------------------------------------------------------
inline bool CFunctionCommit::IsCommitFuncInUse( int nFunc ) const
{
	Assert( nFunc >> 3 < m_nCommitBufferSize );
	return ( m_pCommitFlags[ nFunc >> 3 ] & ( 1 << ( nFunc & 0x7 ) ) ) != 0;
}

inline void CFunctionCommit::MarkCommitFuncInUse( int nFunc )
{
	Assert( nFunc >> 3 < m_nCommitBufferSize );
	m_pCommitFlags[ nFunc >> 3 ] |= 1 << ( nFunc & 0x7 );
}

inline void CFunctionCommit::AddCommitFunc( StateCommitFunc_t f )
{
	m_CommitFuncs.AddToTail( f );
}


//-----------------------------------------------------------------------------
// Clears all commit functions
//-----------------------------------------------------------------------------
inline void CFunctionCommit::ClearAllCommitFuncs( )
{
	memset( m_pCommitFlags, 0, m_nCommitBufferSize );
	m_CommitFuncs.RemoveAll();
}


//-----------------------------------------------------------------------------
// Calls all commit functions in a particular list
//-----------------------------------------------------------------------------
void CFunctionCommit::CallCommitFuncs( ID3D11Device *pDevice, ID3D11DeviceContext *pContext,
				       const ShaderStateDx11_t &desiredState, ShaderStateDx11_t &currentState, bool bForce )
{
	int nCount = m_CommitFuncs.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		m_CommitFuncs[i]( pDevice, pContext, desiredState, currentState, bForce );
	}

	ClearAllCommitFuncs( );
}


//-----------------------------------------------------------------------------
// Helpers for commit functions
//-----------------------------------------------------------------------------
#define ADD_COMMIT_FUNC( _func_name )	\
	if ( !m_Commit.IsCommitFuncInUse( COMMIT_FUNC_ ## _func_name ) )	\
	{																	\
		m_Commit.AddCommitFunc( _func_name );							\
		m_Commit.MarkCommitFuncInUse( COMMIT_FUNC_ ## _func_name );		\
	}

#define ADD_RENDERSTATE_FUNC( _func_name, _state, _val )					\
	if ( m_bResettingRenderState || ( m_DesiredState. ## _state != _val ) )	\
	{																		\
		m_DesiredState. ## _state = _val;									\
		ADD_COMMIT_FUNC( _func_name )										\
	}

#define IMPLEMENT_RENDERSTATE_FUNC( _func_name, _state, _d3dFunc )			\
	static void _func_name( ID3D11Device *pDevice, ID3D11DeviceContext *pContext, const ShaderStateDx11_t &desiredState, ShaderStateDx11_t &currentState, bool bForce )	\
	{																			\
		if ( bForce || ( desiredState. ## _state != currentState. ## _state ) )	\
		{																		\
			pContext->_d3dFunc( desiredState. ## _state );						\
			currentState. ## _state	= desiredState. ## _state;					\
		}																		\
	}

#define IMPLEMENT_RENDERSTATE_FUNC_DX11( _func_name, _state, _d3dFunc )			\
	static void _func_name( ID3D11Device *pDevice, ID3D11DeviceContext *pContext, const ShaderStateDx11_t &desiredState, ShaderStateDx11_t &currentState, bool bForce )	\
	{																			\
		if ( bForce || ( desiredState. ## _state != currentState. ## _state ) )	\
		{																		\
			pContext->_d3dFunc( desiredState. ## _state, NULL, 0 );						\
			currentState. ## _state	= desiredState. ## _state;					\
		}																		\
	}

//-----------------------------------------------------------------------------
// D3D state setting methods
//-----------------------------------------------------------------------------

// NOTE: For each commit func you create, add to this enumeration.
enum CommitFunc_t
{
	COMMIT_FUNC_CommitSetViewports = 0,
	COMMIT_FUNC_CommitSetVertexShader,
	COMMIT_FUNC_CommitSetGeometryShader,
	COMMIT_FUNC_CommitSetPixelShader,
	COMMIT_FUNC_CommitSetVertexBuffer,
	COMMIT_FUNC_CommitSetIndexBuffer,
	COMMIT_FUNC_CommitSetInputLayout,
	COMMIT_FUNC_CommitSetTopology,
	COMMIT_FUNC_CommitSetRasterState,
	COMMIT_FUNC_CommitSetVSConstantBuffers,
	COMMIT_FUNC_CommitSetPSConstantBuffers,
	COMMIT_FUNC_CommitSetGSConstantBuffers,
	COMMIT_FUNC_CommitSetDepthStencilState,
	COMMIT_FUNC_CommitSetBlendState,

	COMMIT_FUNC_COUNT,
};

IMPLEMENT_RENDERSTATE_FUNC( CommitSetTopology, m_Topology, IASetPrimitiveTopology )
IMPLEMENT_RENDERSTATE_FUNC_DX11( CommitSetVertexShader, m_pVertexShader, VSSetShader )
IMPLEMENT_RENDERSTATE_FUNC_DX11( CommitSetGeometryShader, m_pGeometryShader, GSSetShader )
IMPLEMENT_RENDERSTATE_FUNC_DX11( CommitSetPixelShader, m_pPixelShader, PSSetShader )

static void CommitSetVSConstantBuffers( ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext,
				       const ShaderStateDx11_t& desiredState, ShaderStateDx11_t& currentState,
				       bool bForce )
{
	static constexpr size_t vsBufSize = sizeof( void * ) * MAX_DX11_CBUFFERS;

	bool bChanged = bForce ||
		( desiredState.m_nVSConstantBuffers != currentState.m_nVSConstantBuffers ) ||
		memcmp( desiredState.m_pVSConstantBuffers, currentState.m_pVSConstantBuffers, vsBufSize );
	if ( bChanged )
	{
		pDeviceContext->VSSetConstantBuffers( 0, desiredState.m_nVSConstantBuffers, desiredState.m_pVSConstantBuffers );
		memcpy( currentState.m_pVSConstantBuffers, desiredState.m_pVSConstantBuffers, vsBufSize );
		currentState.m_nVSConstantBuffers = desiredState.m_nVSConstantBuffers;
	}
}

static void CommitSetPSConstantBuffers( ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext,
					const ShaderStateDx11_t& desiredState, ShaderStateDx11_t& currentState,
					bool bForce )
{
	static constexpr size_t psBufSize = sizeof( ShaderStateDx11_t::m_pPSConstantBuffers );

	bool bChanged = bForce ||
		( desiredState.m_nPSConstantBuffers != currentState.m_nPSConstantBuffers ) ||
		memcmp( desiredState.m_pPSConstantBuffers, currentState.m_pPSConstantBuffers, psBufSize );
	if ( bChanged )
	{
		pDeviceContext->PSSetConstantBuffers( 0, desiredState.m_nPSConstantBuffers, desiredState.m_pPSConstantBuffers );
		memcpy( currentState.m_pPSConstantBuffers, desiredState.m_pPSConstantBuffers, psBufSize );
		currentState.m_nPSConstantBuffers = desiredState.m_nPSConstantBuffers;
	}
}

static void CommitSetGSConstantBuffers( ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext,
					const ShaderStateDx11_t& desiredState, ShaderStateDx11_t& currentState,
					bool bForce )
{
	static constexpr size_t gsBufSize = sizeof( ShaderStateDx11_t::m_pGSConstantBuffers );

	bool bChanged = bForce ||
		( desiredState.m_nGSConstantBuffers != currentState.m_nGSConstantBuffers ) ||
		memcmp( desiredState.m_pGSConstantBuffers, currentState.m_pGSConstantBuffers, gsBufSize );
	if ( bChanged )
	{
		pDeviceContext->GSSetConstantBuffers( 0, desiredState.m_nGSConstantBuffers, desiredState.m_pGSConstantBuffers );
		memcpy( currentState.m_pGSConstantBuffers, desiredState.m_pGSConstantBuffers, gsBufSize );
		currentState.m_nGSConstantBuffers = desiredState.m_nGSConstantBuffers;
	}
}

static void CommitSetInputLayout( ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext, 
				  const ShaderStateDx11_t &desiredState, ShaderStateDx11_t &currentState,
				  bool bForce )
{
	const ShaderInputLayoutStateDx11_t& newState = desiredState.m_InputLayout;
	if ( bForce || memcmp( &newState, &currentState.m_InputLayout, sizeof(ShaderInputLayoutStateDx11_t) ) )	
	{
		// FIXME: Deal with multiple streams
		ID3D11InputLayout *pInputLayout = g_pShaderDeviceDx11->GetInputLayout( 
			newState.m_hVertexShader, newState.m_pVertexDecl[0] );
		pDeviceContext->IASetInputLayout( pInputLayout );						

		currentState.m_InputLayout = newState;
	}																		
}

static void CommitSetViewports( ID3D11Device *pDevice, ID3D11DeviceContext *pContext,
				const ShaderStateDx11_t &desiredState, ShaderStateDx11_t &currentState, bool bForce )
{
	bool bChanged = bForce || ( desiredState.m_nViewportCount != currentState.m_nViewportCount );
	if ( !bChanged && desiredState.m_nViewportCount > 0 )
	{
		bChanged = memcmp( desiredState.m_pViewports, currentState.m_pViewports, 
			desiredState.m_nViewportCount * sizeof( D3D11_VIEWPORT ) ) != 0;
	}

	if ( !bChanged )
		return;

	pContext->RSSetViewports( desiredState.m_nViewportCount, desiredState.m_pViewports );
	currentState.m_nViewportCount = desiredState.m_nViewportCount;

#ifdef _DEBUG
	memset( currentState.m_pViewports, 0xDD, sizeof( currentState.m_pViewports ) );
#endif

	memcpy( currentState.m_pViewports, desiredState.m_pViewports, 
		desiredState.m_nViewportCount * sizeof( D3D11_VIEWPORT ) );
}

static void CommitSetIndexBuffer( ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext,
				  const ShaderStateDx11_t &desiredState, ShaderStateDx11_t &currentState,
				  bool bForce )
{
	const ShaderIndexBufferStateDx11_t &newState = desiredState.m_IndexBuffer;
	bool bChanged = bForce || memcmp( &newState, &currentState.m_IndexBuffer, sizeof(ShaderIndexBufferStateDx11_t) );
	if ( !bChanged )
		return;

	pDeviceContext->IASetIndexBuffer( newState.m_pBuffer, newState.m_Format, newState.m_nOffset );
	memcpy( &currentState.m_IndexBuffer, &newState, sizeof( ShaderIndexBufferStateDx11_t ) );
}

static void CommitSetVertexBuffer( ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext,
				   const ShaderStateDx11_t &desiredState, ShaderStateDx11_t &currentState,
				   bool bForce )
{
	ID3D11Buffer *ppVertexBuffers[ MAX_DX11_STREAMS ];
	UINT pStrides[ MAX_DX11_STREAMS ];
	UINT pOffsets[ MAX_DX11_STREAMS ];

	UINT nFirstBuffer = 0;
	UINT nBufferCount = 0;
	bool bInMatch = true;
	for ( int i = 0; i < MAX_DX11_STREAMS; ++i )
	{
		const ShaderVertexBufferStateDx11_t &newState = desiredState.m_pVertexBuffer[i];
		bool bMatch = !bForce && !memcmp( &newState, &currentState.m_pVertexBuffer[i], sizeof(ShaderVertexBufferStateDx11_t) );
		if ( !bMatch )
		{
			ppVertexBuffers[i] = newState.m_pBuffer;
			pStrides[i] = newState.m_nStride;
			pOffsets[i] = newState.m_nOffset;
			++nBufferCount;
			memcpy( &currentState.m_pVertexBuffer[i], &newState, sizeof( ShaderVertexBufferStateDx11_t ) );
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
			pDeviceContext->IASetVertexBuffers( nFirstBuffer, nBufferCount, 
				&ppVertexBuffers[nFirstBuffer], &pStrides[nFirstBuffer], &pOffsets[nFirstBuffer] );
			nBufferCount = 0;
		}
	}

	if ( !bInMatch )
	{
		pDeviceContext->IASetVertexBuffers( nFirstBuffer, nBufferCount, 
			&ppVertexBuffers[nFirstBuffer], &pStrides[nFirstBuffer], &pOffsets[nFirstBuffer] );
	}
}

//-----------------------------------------------------------------------------
// Rasterizer state
//-----------------------------------------------------------------------------

static void GenerateRasterizerDesc( D3D11_RASTERIZER_DESC* pDesc, const ShaderRasterState_t& state )
{
	pDesc->FillMode = ( state.m_FillMode == SHADER_FILL_WIREFRAME ) ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	
	// Cull state
	if ( state.m_bCullEnable )
	{
		pDesc->CullMode = D3D11_CULL_NONE;
	}
	else
	{
		pDesc->CullMode = ( state.m_CullMode == MATERIAL_CULLMODE_CW ) ? D3D11_CULL_BACK : D3D11_CULL_FRONT;
	}
	pDesc->FrontCounterClockwise = TRUE;

	// Depth bias state
	if ( !state.m_bDepthBias )
	{
		pDesc->DepthBias = 0;
		pDesc->DepthBiasClamp = 0.0f;
		pDesc->SlopeScaledDepthBias = 0.0f;
		pDesc->DepthClipEnable = FALSE;
	}
	else
	{
		// FIXME: Implement! Read ConVars
	}

	pDesc->ScissorEnable = state.m_bScissorEnable ? TRUE : FALSE;
	pDesc->MultisampleEnable = state.m_bMultisampleEnable ? TRUE : FALSE;
	pDesc->AntialiasedLineEnable = FALSE;
}

static void CommitSetRasterState( ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext,
				  const ShaderStateDx11_t &desiredState, ShaderStateDx11_t &currentState,
				  bool bForce )
{
	const ShaderRasterState_t& newState = desiredState.m_RasterState;
	if ( bForce || memcmp( &newState, &currentState.m_RasterState, sizeof(ShaderRasterState_t) ) )	
	{
		// Clear out the existing state
		if ( currentState.m_pRasterState )
		{
			currentState.m_pRasterState->Release();
		}

		D3D11_RASTERIZER_DESC desc;
		GenerateRasterizerDesc( &desc, newState );

		// NOTE: This does a search for existing matching state objects
		ID3D11RasterizerState *pState = NULL;
		HRESULT hr = pDevice->CreateRasterizerState( &desc, &pState );
		if ( FAILED(hr) )
		{
			Warning( "Unable to create rasterizer state object!\n" );
		}

		pDeviceContext->RSSetState( pState );						

		currentState.m_pRasterState = pState;
		memcpy( &currentState.m_RasterState, &newState, sizeof( ShaderRasterState_t ) );
	}																		
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

static void GenerateBlendDesc( D3D11_BLEND_DESC* pDesc, const ShaderBlendStateDx11_t& newState )
{
	memset( pDesc, 0, sizeof( D3D11_BLEND_DESC ) );

	pDesc->AlphaToCoverageEnable = newState.m_bAlphaToCoverage ? TRUE : FALSE;
	pDesc->IndependentBlendEnable = newState.m_bIndependentBlend ? TRUE : FALSE;
	pDesc->RenderTarget[0].BlendEnable = newState.m_bBlendEnable ? TRUE : FALSE;
	pDesc->RenderTarget[0].BlendOp = (D3D11_BLEND_OP)newState.m_BlendOp;
	pDesc->RenderTarget[0].BlendOpAlpha = (D3D11_BLEND_OP)newState.m_BlendOpAlpha;
	pDesc->RenderTarget[0].DestBlend = TranslateBlendFunc( newState.m_DestBlend );
	pDesc->RenderTarget[0].SrcBlend = TranslateBlendFunc( newState.m_SrcBlend );
	pDesc->RenderTarget[0].DestBlendAlpha = TranslateBlendFunc( newState.m_DestBlendAlpha );
	pDesc->RenderTarget[0].SrcBlendAlpha = TranslateBlendFunc( newState.m_SrcBlendAlpha );
	pDesc->RenderTarget[0].RenderTargetWriteMask = newState.m_WriteMask;
}

static void CommitSetBlendState( ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext,
				 const ShaderStateDx11_t& desiredState, ShaderStateDx11_t& currentState,
				 bool bForce )
{
	bool bChanged = bForce ||
		memcmp( &desiredState.m_BlendState, &currentState.m_BlendState, sizeof( ShaderBlendStateDx11_t ) );
	if ( bChanged )
	{
		// Clear out existing blend state
		if ( currentState.m_pBlendState )
		{
			currentState.m_pBlendState->Release();
		}

		D3D11_BLEND_DESC desc;
		GenerateBlendDesc( &desc, desiredState.m_BlendState );

		// NOTE: This does a search for existing matching state objects
		ID3D11BlendState* pState = NULL;
		HRESULT hr = pDevice->CreateBlendState( &desc, &pState );
		if ( FAILED( hr ) )
		{
			Warning( "Unable to create Dx11 blend state object!\n" );
		}

		pDeviceContext->OMSetBlendState( pState, desiredState.m_BlendState.m_pBlendColor, desiredState.m_BlendState.m_nSampleMask );

		currentState.m_pBlendState = pState;
		memcpy( &currentState.m_BlendState, &desiredState.m_BlendState, sizeof( ShaderBlendStateDx11_t ) );
	}
}

//-----------------------------------------------------------------------------
// Depth/Stencil state
//-----------------------------------------------------------------------------

static void GenerateDepthStencilDesc( D3D11_DEPTH_STENCIL_DESC* pDesc, const ShaderDepthStencilStateDx11_t& newState )
{
	pDesc->FrontFace.StencilDepthFailOp = (D3D11_STENCIL_OP)newState.m_StencilDepthFailOp;
	pDesc->FrontFace.StencilFailOp = (D3D11_STENCIL_OP)newState.m_StencilFailOp;
	pDesc->FrontFace.StencilPassOp = (D3D11_STENCIL_OP)newState.m_StencilPassOp;
	pDesc->FrontFace.StencilFunc = (D3D11_COMPARISON_FUNC)newState.m_StencilFunc;

	// UNDONE: Backface?
	pDesc->BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	pDesc->BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	pDesc->BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	pDesc->BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;

	pDesc->StencilEnable = newState.m_bStencilEnable ? TRUE : FALSE;
	pDesc->StencilReadMask = newState.m_nStencilReadMask;
	pDesc->StencilWriteMask = newState.m_nStencilWriteMask;

	pDesc->DepthEnable = newState.m_bDepthEnable ? TRUE: FALSE;
	pDesc->DepthFunc = (D3D11_COMPARISON_FUNC)newState.m_DepthFunc;
	pDesc->DepthWriteMask = (D3D11_DEPTH_WRITE_MASK)newState.m_nDepthWriteMask;
}

static void CommitSetDepthStencilState( ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext,
					const ShaderStateDx11_t& desiredState, ShaderStateDx11_t& currentState,
					bool bForce )
{
	bool bChanged = bForce ||
		memcmp( &currentState.m_DepthStencilState, &desiredState.m_DepthStencilState,
			sizeof( ShaderDepthStencilStateDx11_t ) );
	if ( bChanged )
	{
		// Clear out current state
		if ( currentState.m_pDepthStencilState )
		{
			currentState.m_pDepthStencilState->Release();
		}

		D3D11_DEPTH_STENCIL_DESC desc;
		GenerateDepthStencilDesc( &desc, desiredState.m_DepthStencilState );

		// NOTE: This does a search for existing matching state objects
		ID3D11DepthStencilState* pState = NULL;
		HRESULT hr = pDevice->CreateDepthStencilState( &desc, &pState );
		if ( FAILED( hr ) )
		{
			Warning( "Unable to create depth/stencil object!\n" );
		}

		pDeviceContext->OMSetDepthStencilState( pState, desiredState.m_DepthStencilState.m_nStencilRef );

		currentState.m_pDepthStencilState = desiredState.m_pDepthStencilState;
		memcpy( &currentState.m_DepthStencilState, &desiredState.m_DepthStencilState, sizeof( ShaderDepthStencilStateDx11_t ) );
	}
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
CShaderAPIDx11* g_pShaderAPIDx11 = &s_ShaderAPIDx11;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIDx11, IShaderAPI,
				   SHADERAPI_INTERFACE_VERSION, s_ShaderAPIDx11 )

	EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIDx11, IDebugTextureInfo,
					   DEBUG_TEXTURE_INFO_VERSION, s_ShaderAPIDx11 )


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CShaderAPIDx11::CShaderAPIDx11() :
	m_ConstantBuffers( 32 )
{
	m_bResettingRenderState = false;
	m_Commit.Init( COMMIT_FUNC_COUNT );
	ClearShaderState( &m_DesiredState );
	ClearShaderState( &m_CurrentState );
}

CShaderAPIDx11::~CShaderAPIDx11()
{
}


//-----------------------------------------------------------------------------
// Clears the shader state to a well-defined value
//-----------------------------------------------------------------------------
void CShaderAPIDx11::ClearShaderState( ShaderStateDx11_t* pState )
{
	memset( pState, 0, sizeof( ShaderStateDx11_t ) );
}


//-----------------------------------------------------------------------------
// Resets the render state
//-----------------------------------------------------------------------------
void CShaderAPIDx11::ResetRenderState( bool bFullReset )
{
	D3D11_RASTERIZER_DESC rDesc;
	memset( &rDesc, 0, sizeof(rDesc) );
	rDesc.FillMode = D3D11_FILL_SOLID;
	rDesc.CullMode = D3D11_CULL_NONE;
	rDesc.FrontCounterClockwise = TRUE;	// right-hand rule 

	ID3D11RasterizerState *pRasterizerState;
	HRESULT hr = D3D11Device()->CreateRasterizerState( &rDesc, &pRasterizerState ); 
	Assert( !FAILED(hr) );
	D3D11DeviceContext()->RSSetState( pRasterizerState );

	D3D11_DEPTH_STENCIL_DESC dsDesc;
	memset( &dsDesc, 0, sizeof(dsDesc) );

	ID3D11DepthStencilState *pDepthStencilState;
	hr = D3D11Device()->CreateDepthStencilState( &dsDesc, &pDepthStencilState );
	Assert( !FAILED(hr) );
	D3D11DeviceContext()->OMSetDepthStencilState( pDepthStencilState, 0 );

	D3D11_BLEND_DESC bDesc;
	memset( &bDesc, 0, sizeof(bDesc) );
	D3D11_RENDER_TARGET_BLEND_DESC *rtbDesc = &bDesc.RenderTarget[0];
	rtbDesc->SrcBlend = D3D11_BLEND_ONE;
	rtbDesc->DestBlend = D3D11_BLEND_ZERO;
	rtbDesc->BlendOp = D3D11_BLEND_OP_ADD;
	rtbDesc->SrcBlendAlpha = D3D11_BLEND_ONE;
	rtbDesc->DestBlendAlpha = D3D11_BLEND_ZERO;
	rtbDesc->BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtbDesc->RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	FLOAT pBlendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	ID3D11BlendState *pBlendState;
	hr = D3D11Device()->CreateBlendState( &bDesc, &pBlendState );
	Assert( !FAILED(hr) );
	D3D11DeviceContext()->OMSetBlendState( pBlendState, pBlendFactor, 0xFFFFFFFF );
}


//-----------------------------------------------------------------------------
// Commits queued-up state change requests
//-----------------------------------------------------------------------------
void CShaderAPIDx11::CommitStateChanges( bool bForce )
{
	// Don't bother committing anything if we're deactivated
	if ( g_pShaderDevice->IsDeactivated() )
		return;

	m_Commit.CallCommitFuncs( D3D11Device(), D3D11DeviceContext(), m_DesiredState, m_CurrentState, bForce );
}


//-----------------------------------------------------------------------------
// Methods of IShaderDynamicAPI
//-----------------------------------------------------------------------------
void CShaderAPIDx11::GetBackBufferDimensions( int& nWidth, int& nHeight ) const
{
	g_pShaderDeviceDx11->GetBackBufferDimensions( nWidth, nHeight );
} 

	
//-----------------------------------------------------------------------------
// Viewport-related methods
//-----------------------------------------------------------------------------
void CShaderAPIDx11::SetViewports( int nCount, const ShaderViewport_t* pViewports )
{
	nCount = min( nCount, MAX_DX11_VIEWPORTS );
	m_DesiredState.m_nViewportCount = nCount;

	for ( int i = 0; i < nCount; ++i )
	{
		Assert( pViewports[i].m_nVersion == SHADER_VIEWPORT_VERSION );

		D3D11_VIEWPORT& viewport = m_DesiredState.m_pViewports[i];
		viewport.TopLeftX = pViewports[i].m_nTopLeftX;
		viewport.TopLeftY = pViewports[i].m_nTopLeftY;
		viewport.Width = pViewports[i].m_nWidth;
		viewport.Height = pViewports[i].m_nHeight;
		viewport.MinDepth = pViewports[i].m_flMinZ;
		viewport.MaxDepth = pViewports[i].m_flMaxZ;
	}

	ADD_COMMIT_FUNC( CommitSetViewports );
}

int CShaderAPIDx11::GetViewports( ShaderViewport_t* pViewports, int nMax ) const
{
	int nCount = m_DesiredState.m_nViewportCount;
	if ( pViewports && nMax )
	{
		nCount = min( nCount, nMax );
		memcpy( pViewports, m_DesiredState.m_pViewports, nCount * sizeof( ShaderViewport_t ) );
	}
	return nCount;
}

//-----------------------------------------------------------------------------
// Methods related to constant buffers
//-----------------------------------------------------------------------------
void CShaderAPIDx11::SetVertexShaderConstantBuffers( int nCount, const ConstantBufferHandle_t* pBuffers )
{
	nCount = min( nCount, MAX_DX11_CBUFFERS );
	m_DesiredState.m_nVSConstantBuffers = nCount;
	for ( int i = 0; i < nCount; i++ )
	{
		m_DesiredState.m_pVSConstantBuffers[i] = (ID3D11Buffer *)pBuffers[i];
	}

	if ( m_bResettingRenderState ||
	     m_DesiredState.m_nVSConstantBuffers != nCount ||
	     memcmp( m_DesiredState.m_pVSConstantBuffers, pBuffers, sizeof( ID3D11Buffer* ) * nCount ) )
		ADD_COMMIT_FUNC( CommitSetVSConstantBuffers );
}

void CShaderAPIDx11::SetPixelShaderConstantBuffers( int nCount, const ConstantBufferHandle_t* pBuffers )
{
	nCount = min( nCount, MAX_DX11_CBUFFERS );
	m_DesiredState.m_nPSConstantBuffers = nCount;
	for ( int i = 0; i < nCount; i++ )
	{
		m_DesiredState.m_pPSConstantBuffers[i] = (ID3D11Buffer*)pBuffers[i];
	}

	if ( m_bResettingRenderState ||
	     m_DesiredState.m_nPSConstantBuffers != nCount ||
	     memcmp( m_DesiredState.m_pPSConstantBuffers, pBuffers, sizeof( ID3D11Buffer* ) * nCount ) )
		ADD_COMMIT_FUNC( CommitSetPSConstantBuffers );
}

void CShaderAPIDx11::SetGeometryShaderConstantBuffers( int nCount, const ConstantBufferHandle_t* pBuffers )
{
	nCount = min( nCount, MAX_DX11_CBUFFERS );
	m_DesiredState.m_nGSConstantBuffers = nCount;
	for ( int i = 0; i < nCount; i++ )
	{
		m_DesiredState.m_pGSConstantBuffers[i] = (ID3D11Buffer*)pBuffers[i];
	}
	if ( m_bResettingRenderState ||
	     m_DesiredState.m_nGSConstantBuffers != nCount ||
	     memcmp( m_DesiredState.m_pGSConstantBuffers, pBuffers, sizeof( ID3D11Buffer* ) * nCount ) )
		ADD_COMMIT_FUNC( CommitSetGSConstantBuffers );
}

ConstantBufferHandle_t CShaderAPIDx11::CreateConstantBuffer( size_t nBufLen )
{
	CShaderConstantBufferDx11 buf;
	buf.Create( nBufLen );
	return m_ConstantBuffers.AddToTail( buf );
}

void CShaderAPIDx11::UpdateConstantBuffer( ConstantBufferHandle_t hBuffer, void* pData )
{
	CShaderConstantBufferDx11& buf = m_ConstantBuffers.Element( hBuffer );
	buf.Update( pData );
}

void CShaderAPIDx11::DestroyConstantBuffer( ConstantBufferHandle_t hBuffer )
{
	CShaderConstantBufferDx11& buf = m_ConstantBuffers.Element( hBuffer );
	buf.Destroy();
	m_ConstantBuffers.Remove( hBuffer );
}

//-----------------------------------------------------------------------------
// Methods related to state objects
//-----------------------------------------------------------------------------
void CShaderAPIDx11::SetRasterState( const ShaderRasterState_t& state )
{
	if ( memcmp( &state, &m_DesiredState.m_RasterState, sizeof(ShaderRasterState_t) ) )
	{
		memcpy( &m_DesiredState.m_RasterState, &state, sizeof(ShaderRasterState_t) );
		ADD_COMMIT_FUNC( CommitSetRasterState );
	}
}


//-----------------------------------------------------------------------------
// Methods related to clearing buffers
//-----------------------------------------------------------------------------
void CShaderAPIDx11::ClearColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
	m_DesiredState.m_ClearColor[0] = r / 255.0f;
	m_DesiredState.m_ClearColor[1] = g / 255.0f;
	m_DesiredState.m_ClearColor[2] = b / 255.0f;
	m_DesiredState.m_ClearColor[3] = 1.0f;
}

void CShaderAPIDx11::ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	m_DesiredState.m_ClearColor[0] = r / 255.0f;
	m_DesiredState.m_ClearColor[1] = g / 255.0f;
	m_DesiredState.m_ClearColor[2] = b / 255.0f;
	m_DesiredState.m_ClearColor[3] = a / 255.0f;
}

void CShaderAPIDx11::ClearBuffers( bool bClearColor, bool bClearDepth, bool bClearStencil, int renderTargetWidth, int renderTargetHeight )
{
	// NOTE: State change commit isn't necessary since clearing doesn't use state
//	CommitStateChanges();

	// FIXME: This implementation is totally bust0red [doesn't guarantee exact color specified]
	if ( bClearColor )
	{
		D3D11DeviceContext()->ClearRenderTargetView( D3D11RenderTargetView(), m_DesiredState.m_ClearColor ); 
	}
}


//-----------------------------------------------------------------------------
// Methods related to binding shaders
//-----------------------------------------------------------------------------
void CShaderAPIDx11::BindVertexShader( VertexShaderHandle_t hVertexShader )
{
	ID3D11VertexShader *pVertexShader = g_pShaderDeviceDx11->GetVertexShader( hVertexShader );
	ADD_RENDERSTATE_FUNC( CommitSetVertexShader, m_pVertexShader, pVertexShader );

	if ( m_bResettingRenderState || ( m_DesiredState.m_InputLayout.m_hVertexShader != hVertexShader ) )
	{
		m_DesiredState.m_InputLayout.m_hVertexShader = hVertexShader;
		ADD_COMMIT_FUNC( CommitSetInputLayout );
	}
}

void CShaderAPIDx11::BindGeometryShader( GeometryShaderHandle_t hGeometryShader )
{
	ID3D11GeometryShader *pGeometryShader = g_pShaderDeviceDx11->GetGeometryShader( hGeometryShader );
	ADD_RENDERSTATE_FUNC( CommitSetGeometryShader, m_pGeometryShader, pGeometryShader );
}

void CShaderAPIDx11::BindPixelShader( PixelShaderHandle_t hPixelShader )
{
	ID3D11PixelShader *pPixelShader = g_pShaderDeviceDx11->GetPixelShader( hPixelShader );
	ADD_RENDERSTATE_FUNC( CommitSetPixelShader, m_pPixelShader, pPixelShader );
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

	if ( m_bResettingRenderState || memcmp( &m_DesiredState.m_pVertexBuffer[ nStreamID ], &state, sizeof( ShaderVertexBufferStateDx11_t ) ) )
	{
		m_DesiredState.m_pVertexBuffer[ nStreamID ] = state;
		ADD_COMMIT_FUNC( CommitSetVertexBuffer );
	}

	if ( m_bResettingRenderState || ( m_DesiredState.m_InputLayout.m_pVertexDecl[ nStreamID ] != fmt ) )
	{
		m_DesiredState.m_InputLayout.m_pVertexDecl[ nStreamID ] = fmt;
		ADD_COMMIT_FUNC( CommitSetInputLayout );
	}
}

void CShaderAPIDx11::BindIndexBuffer( IIndexBuffer *pIndexBuffer, int nOffsetInBytes )
{
	CIndexBufferDx11 *pIndexBufferDx11 = static_cast<CIndexBufferDx11 *>( pIndexBuffer );

	ShaderIndexBufferStateDx11_t state;
	if ( pIndexBufferDx11 )
	{
		state.m_pBuffer = pIndexBufferDx11->GetDx11Buffer();
		state.m_Format = ( pIndexBufferDx11->GetIndexFormat() == MATERIAL_INDEX_FORMAT_16BIT ) ? 
			DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	}
	else
	{
		state.m_pBuffer = NULL;
		state.m_Format = DXGI_FORMAT_R16_UINT;
	}
	state.m_nOffset = nOffsetInBytes;

	ADD_RENDERSTATE_FUNC( CommitSetIndexBuffer, m_IndexBuffer, state );
}


//-----------------------------------------------------------------------------
// Unbinds resources because they are about to be deleted
//-----------------------------------------------------------------------------
void CShaderAPIDx11::Unbind( VertexShaderHandle_t hShader )
{
	ID3D11VertexShader* pShader = g_pShaderDeviceDx11->GetVertexShader( hShader );
	Assert ( pShader );
	if ( m_DesiredState.m_pVertexShader == pShader )
	{
		BindVertexShader( VERTEX_SHADER_HANDLE_INVALID );
	}
	if ( m_CurrentState.m_pVertexShader == pShader )
	{
		CommitStateChanges();
	}
}

void CShaderAPIDx11::Unbind( GeometryShaderHandle_t hShader )
{
	ID3D11GeometryShader* pShader = g_pShaderDeviceDx11->GetGeometryShader( hShader );
	Assert ( pShader );
	if ( m_DesiredState.m_pGeometryShader == pShader )
	{
		BindGeometryShader( GEOMETRY_SHADER_HANDLE_INVALID );
	}
	if ( m_CurrentState.m_pGeometryShader == pShader )
	{
		CommitStateChanges();
	}
}

void CShaderAPIDx11::Unbind( PixelShaderHandle_t hShader )
{
	ID3D11PixelShader* pShader = g_pShaderDeviceDx11->GetPixelShader( hShader );
	Assert ( pShader );
	if ( m_DesiredState.m_pPixelShader == pShader )
	{
		BindPixelShader( PIXEL_SHADER_HANDLE_INVALID );
	}
	if ( m_CurrentState.m_pPixelShader == pShader )
	{
		CommitStateChanges();
	}
}

void CShaderAPIDx11::UnbindVertexBuffer( ID3D11Buffer *pBuffer )
{
	Assert ( pBuffer );

	for ( int i = 0; i < MAX_DX11_STREAMS; ++i )
	{
		if ( m_DesiredState.m_pVertexBuffer[i].m_pBuffer == pBuffer )
		{
			BindVertexBuffer( i, NULL, 0, 0, 0, VERTEX_POSITION, 0 );
		}
	}
	for ( int i = 0; i < MAX_DX11_STREAMS; ++i )
	{
		if ( m_CurrentState.m_pVertexBuffer[i].m_pBuffer == pBuffer )
		{
			CommitStateChanges();
			break;
		}
	}
}

void CShaderAPIDx11::UnbindIndexBuffer( ID3D11Buffer *pBuffer )
{
	Assert ( pBuffer );

	if ( m_DesiredState.m_IndexBuffer.m_pBuffer == pBuffer )
	{
		BindIndexBuffer( NULL, 0 );
	}
	if ( m_CurrentState.m_IndexBuffer.m_pBuffer == pBuffer )
	{
		CommitStateChanges();
	}
}


//-----------------------------------------------------------------------------
// Sets the topology state
//-----------------------------------------------------------------------------
void CShaderAPIDx11::SetTopology( MaterialPrimitiveType_t topology )
{
	D3D11_PRIMITIVE_TOPOLOGY d3dTopology;
	switch( topology )
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

	ADD_RENDERSTATE_FUNC( CommitSetTopology, m_Topology, d3dTopology );
}


//-----------------------------------------------------------------------------
// Main entry point for rendering
//-----------------------------------------------------------------------------
void CShaderAPIDx11::Draw( MaterialPrimitiveType_t primitiveType, int nFirstIndex, int nIndexCount )
{
	SetTopology( primitiveType );

	CommitStateChanges();

	// FIXME: How do I set the base vertex location!?
	D3D11DeviceContext()->DrawIndexed( (UINT)nIndexCount, (UINT)nFirstIndex, 0 );
}

bool CShaderAPIDx11::OnDeviceInit()
{
	m_hTransformBuffer = CreateConstantBuffer( sizeof( TransformBuffer_t ) );
	m_hLightingBuffer = CreateConstantBuffer( sizeof( LightingBuffer_t ) );

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
StateSnapshot_t	 CShaderAPIDx11::TakeSnapshot( )
{
	return g_pShaderShadowDx11->FindOrCreateSnapshot();
}

// Returns true if the state snapshot is transparent
bool CShaderAPIDx11::IsTranslucent( StateSnapshot_t id ) const
{
	return (id & TRANSLUCENT) != 0; 
}

bool CShaderAPIDx11::IsAlphaTested( StateSnapshot_t id ) const
{
	return (id & ALPHATESTED) != 0; 
}

bool CShaderAPIDx11::IsDepthWriteEnabled( StateSnapshot_t id ) const
{
	return (id & DEPTHWRITE) != 0; 
}

bool CShaderAPIDx11::UsesVertexAndPixelShaders( StateSnapshot_t id ) const
{
	return (id & VERTEX_AND_PIXEL_SHADERS) != 0; 
}

// Gets the vertex format for a set of snapshot ids
VertexFormat_t CShaderAPIDx11::ComputeVertexFormat( int numSnapshots, StateSnapshot_t* pIds ) const
{
	return 0;
}

// Gets the vertex format for a set of snapshot ids
VertexFormat_t CShaderAPIDx11::ComputeVertexUsage( int numSnapshots, StateSnapshot_t* pIds ) const
{
	return 0;
}

// Uses a state snapshot
void CShaderAPIDx11::UseSnapshot( StateSnapshot_t snapshot )
{
	ShadowStateCacheEntryDx11_t entry = g_pShaderShadowDx11->m_ShadowStateCache.Element( snapshot );

	ADD_RENDERSTATE_FUNC( CommitSetDepthStencilState, m_DepthStencilState.m_bDepthEnable, entry.m_State.m_ZEnable );
	ADD_RENDERSTATE_FUNC( CommitSetDepthStencilState, m_DepthStencilState.m_DepthFunc, entry.m_State.m_ZFunc );
	uint8 depthWriteMask = entry.m_State.m_ZWriteEnable ? 1 : 0;
	ADD_RENDERSTATE_FUNC( CommitSetDepthStencilState, m_DepthStencilState.m_nDepthWriteMask, depthWriteMask );
	// TODO: ZBias
	ADD_RENDERSTATE_FUNC( CommitSetDepthStencilState, m_DepthStencilState.m_bStencilEnable, entry.m_State.m_StencilEnable );

	ADD_RENDERSTATE_FUNC( CommitSetRasterState, m_RasterState.m_bCullEnable, entry.m_State.m_CullEnable );
	ADD_RENDERSTATE_FUNC( CommitSetRasterState, m_RasterState.m_FillMode, entry.m_State.m_FillMode );

	ADD_RENDERSTATE_FUNC( CommitSetBlendState, m_BlendState.m_bAlphaToCoverage, entry.m_State.m_EnableAlphaToCoverage );
	ADD_RENDERSTATE_FUNC( CommitSetBlendState, m_BlendState.m_bBlendEnable, entry.m_State.m_AlphaBlendEnable );
	ADD_RENDERSTATE_FUNC( CommitSetBlendState, m_BlendState.m_bIndependentBlend, entry.m_State.m_SeparateAlphaBlendEnable );
	ADD_RENDERSTATE_FUNC( CommitSetBlendState, m_BlendState.m_WriteMask, entry.m_State.m_ColorWriteEnable );
	ADD_RENDERSTATE_FUNC( CommitSetBlendState, m_BlendState.m_SrcBlend, entry.m_State.m_SrcBlend );
	ADD_RENDERSTATE_FUNC( CommitSetBlendState, m_BlendState.m_DestBlend, entry.m_State.m_DestBlend );
	ADD_RENDERSTATE_FUNC( CommitSetBlendState, m_BlendState.m_SrcBlendAlpha, entry.m_State.m_SrcBlendAlpha );
	ADD_RENDERSTATE_FUNC( CommitSetBlendState, m_BlendState.m_DestBlendAlpha, entry.m_State.m_DestBlendAlpha );

	ShaderManager()->SetVertexShaderIndex( entry.m_ShaderState.m_nStaticVshIndex );
	ShaderManager()->SetVertexShader( entry.m_ShaderState.m_VertexShader );

	ShaderManager()->SetPixelShaderIndex( entry.m_ShaderState.m_nStaticPshIndex );
	ShaderManager()->SetPixelShader( entry.m_ShaderState.m_PixelShader );

	SetVertexShaderConstantBuffers( entry.m_ShaderState.m_nCBuffers, entry.m_ShaderState.m_CBuffers );
	SetGeometryShaderConstantBuffers( entry.m_ShaderState.m_nCBuffers, entry.m_ShaderState.m_CBuffers );
	SetPixelShaderConstantBuffers( entry.m_ShaderState.m_nCBuffers, entry.m_ShaderState.m_CBuffers );
}

// Sets the color to modulate by
void CShaderAPIDx11::Color3f( float r, float g, float b )
{
}

void CShaderAPIDx11::Color3fv( float const* pColor )
{
}

void CShaderAPIDx11::Color4f( float r, float g, float b, float a )
{
}

void CShaderAPIDx11::Color4fv( float const* pColor )
{
}

// Faster versions of color
void CShaderAPIDx11::Color3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPIDx11::Color3ubv( unsigned char const* rgb )
{
}

void CShaderAPIDx11::Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
}

void CShaderAPIDx11::Color4ubv( unsigned char const* rgba )
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
void CShaderAPIDx11::Bind( IMaterial* pMaterial )
{
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
void CShaderAPIDx11::SetLight( int lightNum, const LightDesc_t& desc )
{
}

void CShaderAPIDx11::SetAmbientLight( float r, float g, float b )
{
}

void CShaderAPIDx11::SetAmbientLightCube( Vector4D cube[6] )
{
}

// Get lights
int CShaderAPIDx11::GetMaxLights( void ) const
{
	return 0;
}

const LightDesc_t& CShaderAPIDx11::GetLight( int lightNum ) const
{
	static LightDesc_t blah;
	return blah;
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
IMesh* CShaderAPIDx11::CreateStaticMesh( VertexFormat_t fmt, const char *pTextureBudgetGroup, IMaterial * pMaterial )
{
	return &m_Mesh;
}

void CShaderAPIDx11::DestroyStaticMesh( IMesh* mesh )
{
}

// Gets the dynamic mesh; note that you've got to render the mesh
// before calling this function a second time. Clients should *not*
// call DestroyStaticMesh on the mesh returned by this call.
IMesh* CShaderAPIDx11::GetDynamicMesh( IMaterial* pMaterial, int nHWSkinBoneCount, bool buffered, IMesh* pVertexOverride, IMesh* pIndexOverride )
{
	Assert( (pMaterial == NULL) || ((IMaterialInternal *)pMaterial)->IsRealTimeVersion() );
	return &m_Mesh;
}

IMesh* CShaderAPIDx11::GetDynamicMeshEx( IMaterial* pMaterial, VertexFormat_t fmt, int nHWSkinBoneCount, bool buffered, IMesh* pVertexOverride, IMesh* pIndexOverride )
{
	// UNDONE: support compressed dynamic meshes if needed (pro: less VB memory, con: time spent compressing)
	Assert( CompressionType( pVertexOverride->GetVertexFormat() ) != VERTEX_COMPRESSION_NONE );
	Assert( (pMaterial == NULL) || ((IMaterialInternal *)pMaterial)->IsRealTimeVersion() );
	return &m_Mesh;
}

IMesh* CShaderAPIDx11::GetFlexMesh()
{
	return &m_Mesh;
}

// Begins a rendering pass that uses a state snapshot
void CShaderAPIDx11::BeginPass( StateSnapshot_t snapshot  )
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

MaterialFogMode_t CShaderAPIDx11::GetSceneFogMode( )
{
	return MATERIAL_FOG_NONE;
}

int CShaderAPIDx11::GetPixelFogCombo( )
{
	return 0; //FIXME
}

void CShaderAPIDx11::FogColor3f( float r, float g, float b )
{
}

void CShaderAPIDx11::FogColor3fv( float const* rgb )
{
}

void CShaderAPIDx11::FogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPIDx11::FogColor3ubv( unsigned char const* rgb )
{
}

void CShaderAPIDx11::Viewport( int x, int y, int width, int height )
{
}

void CShaderAPIDx11::GetViewport( int& x, int& y, int& width, int& height ) const
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
void CShaderAPIDx11::SetVertexShaderConstant( int var, float const* pVec, int numConst, bool bForce )
{
	Warning( "Unsupported CShaderAPIDx11::SetVertexShaderConstant() called!\n" );
}

void CShaderAPIDx11::SetPixelShaderConstant( int var, float const* pVec, int numConst, bool bForce )
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
}

// Indicates we're going to be modifying this texture
// TexImage2D, TexSubImage2D, TexWrap, TexMinFilter, and TexMagFilter
// all use the texture specified by this function.
void CShaderAPIDx11::ModifyTexture( ShaderAPITextureHandle_t textureHandle )
{
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
							  int width, int height, CPixelWriter& writer )
{
	return false;
}

void CShaderAPIDx11::TexUnlock( )
{
}


// These are bound to the texture, not the texture environment
void CShaderAPIDx11::TexMinFilter( ShaderTexFilterMode_t texFilterMode )
{
}

void CShaderAPIDx11::TexMagFilter( ShaderTexFilterMode_t texFilterMode )
{
}

void CShaderAPIDx11::TexWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode )
{
}

void CShaderAPIDx11::TexSetPriority( int priority )
{
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

	if ( depth == 0 )
		depth == 1;
	for ( int k = 0; k < count; ++ k )
	{
		pHandles[ k ] = 0;
	}

	bool bIsCubeMap = ( flags & TEXTURE_CREATE_CUBEMAP ) != 0;
	bool bIsRenderTarget = ( flags & TEXTURE_CREATE_RENDERTARGET ) != 0;
	bool bIsManaged = ( flags & TEXTURE_CREATE_MANAGED ) != 0;
	bool bIsDepthBuffer = ( flags & TEXTURE_CREATE_DEPTHBUFFER ) != 0;
	bool bIsDynamic = ( flags & TEXTURE_CREATE_DYNAMIC ) != 0;

	// Can't be both managed + dynamic. Dynamic is an optimization, but 
	// if it's not managed, then we gotta do special client-specific stuff
	// So, managed wins out!
	if ( bIsManaged )
		bIsDynamic = false;

	// Create a set of texture handles
	//CreateTextureHandles
}

ShaderAPITextureHandle_t CShaderAPIDx11::CreateDepthTexture( ImageFormat renderFormat, int width, int height, const char *pDebugName, bool bTexture )
{
	return 0;
}

void CShaderAPIDx11::DeleteTexture( ShaderAPITextureHandle_t textureHandle )
{
}

bool CShaderAPIDx11::IsTexture( ShaderAPITextureHandle_t textureHandle )
{
	return true;
}

bool CShaderAPIDx11::IsTextureResident( ShaderAPITextureHandle_t textureHandle )
{
	return false;
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

void CShaderAPIDx11::SelectionBuffer( unsigned int* pBuffer, int size )
{
}

void CShaderAPIDx11::ClearSelectionNames( )
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
CMeshBuilder* CShaderAPIDx11::GetVertexModifyBuilder()
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
void CShaderAPIDx11::GetWorldSpaceCameraPosition( float * pPos ) const
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
bool CShaderAPIDx11::IsHWMorphingEnabled( ) const
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
	ADD_RENDERSTATE_FUNC( CommitSetDepthStencilState, m_DepthStencilState.m_bStencilEnable, onoff );
}

void CShaderAPIDx11::SetStencilFailOperation( StencilOperation_t op )
{
	ADD_RENDERSTATE_FUNC( CommitSetDepthStencilState, m_DepthStencilState.m_StencilFailOp, op );
}

void CShaderAPIDx11::SetStencilZFailOperation( StencilOperation_t op )
{
	ADD_RENDERSTATE_FUNC( CommitSetDepthStencilState, m_DepthStencilState.m_StencilDepthFailOp, op );
}

void CShaderAPIDx11::SetStencilPassOperation( StencilOperation_t op )
{
	ADD_RENDERSTATE_FUNC( CommitSetDepthStencilState, m_DepthStencilState.m_StencilPassOp, op );
}

void CShaderAPIDx11::SetStencilCompareFunction( StencilComparisonFunction_t cmpfn )
{
	ADD_RENDERSTATE_FUNC( CommitSetDepthStencilState, m_DepthStencilState.m_StencilFunc, cmpfn );
}

void CShaderAPIDx11::SetStencilReferenceValue( int ref )
{
	ADD_RENDERSTATE_FUNC( CommitSetDepthStencilState, m_DepthStencilState.m_nStencilRef, ref );
}

void CShaderAPIDx11::SetStencilTestMask( uint32 msk )
{
	ADD_RENDERSTATE_FUNC( CommitSetDepthStencilState, m_DepthStencilState.m_nStencilReadMask, msk );
}

void CShaderAPIDx11::SetStencilWriteMask( uint32 msk )
{
	ADD_RENDERSTATE_FUNC( CommitSetDepthStencilState, m_DepthStencilState.m_nStencilWriteMask, msk );
}