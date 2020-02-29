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
#include "tier0/vprof.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

enum
{
	MATRIXDX11_DIRTY,
	MATRIXDX11_IDENTITY,
};

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
	m_Textures( 32 ),
	m_SelectionMinZ( FLT_MAX ),
	m_SelectionMaxZ( FLT_MIN ),
	m_pSelectionBuffer( 0 ),
	m_pSelectionBufferEnd( 0 ),
	m_nDynamicVBSize( DYNAMIC_VERTEX_BUFFER_MEMORY )
{
	m_ModifyTextureHandle = INVALID_SHADERAPI_TEXTURE_HANDLE;
	m_ModifyTextureLockedLevel = -1;
	m_bResettingRenderState = false;
	m_TargetState = StatesDx11::RenderState();
	m_State = m_TargetState;
	m_DynamicState = DynamicStateDx11_t();
	m_bSelectionMode = false;

	for ( int i = 0; i < NUM_MATRIX_MODES; i++ )
	{
		m_MatrixStacks[i].Push();
		m_MatrixStacks[i].Top().m_Matrix = DirectX::XMMatrixIdentity();
		m_MatrixStacks[i].Top().m_Flags = ( MATRIXDX11_DIRTY | MATRIXDX11_IDENTITY );
		m_ChangedMatrices[i] = true;
	}
	m_pCurMatrixItem = &m_MatrixStacks[0].Top();
}

CShaderAPIDx11::~CShaderAPIDx11()
{
}

void CShaderAPIDx11::UpdateConstantBuffer( ConstantBuffer_t cbuffer, void *pNewData )
{
	g_pShaderDeviceDx11->UpdateConstantBuffer( cbuffer, pNewData );
}

ConstantBuffer_t CShaderAPIDx11::GetInternalConstantBuffer( int type )
{
	return g_pShaderDeviceDx11->GetInternalConstantBuffer( type );
}

void CShaderAPIDx11::BindPixelShaderConstantBuffer( ConstantBuffer_t cbuffer )
{
	// God awful
	m_TargetState.dynamic.m_ppPSConstantBuffers
		[m_TargetState.dynamic.m_nPSConstantBuffers++] =
		(CShaderConstantBufferDx11 *)g_pShaderDeviceDx11->GetConstantBuffer( cbuffer );
}

void CShaderAPIDx11::BindVertexShaderConstantBuffer( ConstantBuffer_t cbuffer )
{
	m_TargetState.dynamic.m_ppVSConstantBuffers
		[m_TargetState.dynamic.m_nVSConstantBuffers++] =
		(CShaderConstantBufferDx11 *)g_pShaderDeviceDx11->GetConstantBuffer( cbuffer );
}

void CShaderAPIDx11::BindGeometryShaderConstantBuffer( ConstantBuffer_t cbuffer )
{
	m_TargetState.dynamic.m_ppGSConstantBuffers
		[m_TargetState.dynamic.m_nGSConstantBuffers++] =
		(CShaderConstantBufferDx11 *)g_pShaderDeviceDx11->GetConstantBuffer( cbuffer );
}

//-----------------------------------------------------------------------------
// Resets the render state
//-----------------------------------------------------------------------------
void CShaderAPIDx11::ResetRenderState( bool bFullReset )
{
	m_TargetState.dynamic.SetDefault();
	m_TargetState.shadow.SetDefault();
	m_TargetState.dynamic.m_pRenderTargetView = GetTexture( m_hBackBuffer ).GetRenderTargetView();
	m_TargetState.dynamic.m_pDepthStencilView = GetTexture( m_hDepthBuffer ).GetDepthStencilView();
	IssueStateChanges( bFullReset );
}

void CShaderAPIDx11::SetRenderTargetEx( int id, ShaderAPITextureHandle_t colorTextureHandle,
					ShaderAPITextureHandle_t depthTextureHandle )
{
	if ( colorTextureHandle == SHADER_RENDERTARGET_BACKBUFFER )
	{
		colorTextureHandle = m_hBackBuffer;
	}
	else if ( colorTextureHandle == SHADER_RENDERTARGET_NONE )
	{
		colorTextureHandle = INVALID_SHADERAPI_TEXTURE_HANDLE;
	}

	if ( depthTextureHandle == SHADER_RENDERTARGET_DEPTHBUFFER )
	{
		depthTextureHandle = m_hDepthBuffer;
	}
	else if ( depthTextureHandle == SHADER_RENDERTARGET_NONE )
	{
		depthTextureHandle = INVALID_SHADERAPI_TEXTURE_HANDLE;
	}

	if ( colorTextureHandle != INVALID_SHADERAPI_TEXTURE_HANDLE )
	{
		CTextureDx11 *pRT = &GetTexture( colorTextureHandle );
		m_TargetState.dynamic.m_pRenderTargetView = pRT->GetRenderTargetView();
	}
	else
	{
		m_TargetState.dynamic.m_pRenderTargetView = NULL;
	}

	if ( depthTextureHandle != INVALID_SHADERAPI_TEXTURE_HANDLE )
	{
		CTextureDx11 *pDT = &GetTexture( depthTextureHandle );
		m_TargetState.dynamic.m_pDepthStencilView = pDT->GetDepthStencilView();
	}
	else
	{
		m_TargetState.dynamic.m_pDepthStencilView = NULL;
	}
}

//-----------------------------------------------------------------------------
// Issues state changes to D3D for states that have been modified
//-----------------------------------------------------------------------------
void CShaderAPIDx11::IssueStateChanges( bool bForce )
{
	// Don't bother committing anything if we're deactivated
	if ( g_pShaderDeviceDx11->IsDeactivated() )
		return;

	float flStart = Plat_FloatTime();

	////Log( "ShaderAPIDx11: Issuing state changes\n" );

	const StatesDx11::DynamicState &targetDynamic = m_TargetState.dynamic;
	const StatesDx11::ShadowState &targetShadow = m_TargetState.shadow;
	
	const StatesDx11::DynamicState &dynamic = m_State.dynamic;
	const StatesDx11::ShadowState &shadow = m_State.shadow;

	// Update transform
	if ( m_ChangedMatrices[MATERIAL_MODEL] ||
	     m_ChangedMatrices[MATERIAL_VIEW] ||
	     m_ChangedMatrices[MATERIAL_PROJECTION] )
	{
		//Log( "\tIssuing transform\n" );
		DoIssueTransform();
	}
	
	// If any of the constant buffers used by the target state have been modified,
	// upload the new data to the GPU.
	//Log( "\tUploading changed constant buffers...\n" );
	DoIssueConstantBufferUpdates();

	bool bStateChanged = false;

	if ( m_TargetState.dynamic.m_pRenderTargetView != m_State.dynamic.m_pRenderTargetView ||
	     m_TargetState.dynamic.m_pDepthStencilView != m_State.dynamic.m_pDepthStencilView )
	{
		DoIssueRenderTargets();
		bStateChanged = true;
	}

	bool bViewportsChanged = bForce || ( targetDynamic.m_nViewportCount != dynamic.m_nViewportCount );
	if ( !bViewportsChanged && targetDynamic.m_nViewportCount > 0 )
	{
		bViewportsChanged = memcmp( targetDynamic.m_pViewports, dynamic.m_pViewports,
					    sizeof( D3D11_VIEWPORT ) * targetDynamic.m_nViewportCount ) != 0;
	}
	if ( bViewportsChanged )
	{
		//Log( "\tIssuing viewports\n" );
		DoIssueViewports();
		bStateChanged = true;
	}

	if ( bForce || memcmp( &targetDynamic.m_InputLayout, &dynamic.m_InputLayout, sizeof( StatesDx11::InputLayoutState ) ) )
	{
		//Log( "\tIssuing input layout\n" );
		DoIssueInputLayout();
		bStateChanged = true;
	}

	bool bVertexBufferChanged = DoIssueVertexBuffer( bForce );
	if ( bVertexBufferChanged )
	{
		//Log( "\tIssued vertex buffer\n" );
		bStateChanged = true;
	}
		

	if ( bForce || memcmp( &targetDynamic.m_IndexBuffer, &dynamic.m_IndexBuffer, sizeof( StatesDx11::IndexBufferState ) ) )
	{
		//Log( "\tIssuing index buffer\n" );
		DoIssueIndexBuffer();
		bStateChanged = true;
	}

	if ( bForce || targetDynamic.m_Topology != dynamic.m_Topology )
	{
		//Log( "\tIssuing topology\n" );
		DoIssueTopology();
		bStateChanged = true;
	}

	if ( bForce || targetDynamic.m_pVertexShader != dynamic.m_pVertexShader )
	{
		//Log( "\tIssuing vertex shader\n" );
		DoIssueVertexShader();
		bStateChanged = true;
	}

	if ( bForce || targetDynamic.m_pGeometryShader != dynamic.m_pGeometryShader )
	{
		//Log( "\tIssuing geometry shader\n" );
		DoIssueGeometryShader();
		bStateChanged = true;
	}

	if ( bForce || targetDynamic.m_pPixelShader != dynamic.m_pPixelShader )
	{
		//Log( "\tIssuing pixel shader\n" );
		DoIssuePixelShader();
		bStateChanged = true;
	}

	bool bCBsChanged = DoIssueConstantBuffers( bForce );
	if ( bCBsChanged )
	{
		//Log( "\tIssued constant buffers\n" );
		bStateChanged = true;
	}
	
	if ( bForce ||
	     memcmp( targetDynamic.m_ppTextureViews,
		     dynamic.m_ppTextureViews,
		     sizeof( ID3D11ShaderResourceView * ) * MAX_DX11_SAMPLERS ) )
	{
		//Log( "\tIssuing textures\n" );
		DoIssueTexture();
		bStateChanged = true;
	}

	if ( bForce ||
	     memcmp( targetDynamic.m_ppSamplers,
		     dynamic.m_ppSamplers,
		     sizeof( ID3D11SamplerState * ) * MAX_DX11_SAMPLERS ) )
	{
		//Log( "\tIssuing samplers\n" );
		DoIssueSampler();
		bStateChanged = true;
	}

	// DepthStencilState is affected by DepthTestAttrib, DepthWriteAttrib, and StencilAttrib
	if ( bForce ||
	     m_State.shadow.DepthStencilStateChanged( m_TargetState.shadow ) )
	{
		//Log( "\tIssuing depth stencil\n" );
		DoIssueDepthStencilState();
		bStateChanged = true;
	}

	// BlendState
	if ( bForce ||
	     m_State.shadow.BlendStateChanged( m_TargetState.shadow ) )
	{
		//Log( "\tIssuing blend\n" );
		DoIssueBlendState();
		bStateChanged = true;
	}

	// Raster state - RenderModeAttrib, CullFaceAttrib, DepthOffsetAttrib, ScissorAttrib, AntialiasAttrib
	if ( bForce ||
	     m_State.shadow.RasterStateChanged( m_TargetState.shadow ) )
	{
		//Log( "\tIssuing raster\n" );
		DoIssueRasterState();
		bStateChanged = true;
	}

	if ( bStateChanged )
	{
		//Log( "Done, state was changed\n" );
		m_State = m_TargetState;
	}
	else
	{
		//Log( "Done, state not changed\n" );
	}

	float flEnd = Plat_FloatTime();

	//Log( "Took %f seconds issuing state changes\n", flEnd - flStart );
}

//------------------------------
// Issue state changes
//------------------------------

FORCEINLINE static void OutputMatrixRow( const DirectX::XMFLOAT4X4 &mat, int r )
{
	Log( "\t%f\t%f\t%f\t%f\n", mat.m[r][0], mat.m[r][1], mat.m[r][2], mat.m[r][3] );
}

FORCEINLINE static void OutputMatrix( const DirectX::XMFLOAT4X4 &mat, const char *pszMatName )
{
	Log( "DX11 %s Matrix:\n", pszMatName );
	
	for ( int i = 0; i < 4; i++ )
	{
		OutputMatrixRow( mat, i );
	}
}

void CShaderAPIDx11::DoIssueTransform()
{
	TransformBuffer_t tmp;
	const DirectX::XMMATRIX model = GetMatrix( MATERIAL_MODEL );
	const DirectX::XMMATRIX view = GetMatrix( MATERIAL_VIEW );
	const DirectX::XMMATRIX projection = GetMatrix( MATERIAL_PROJECTION );
	tmp.modelTransform = DirectX::XMMatrixTranspose( model );
	tmp.viewTransform = DirectX::XMMatrixTranspose( view );
	tmp.projTransform = DirectX::XMMatrixTranspose( projection );

	DirectX::XMMATRIX modelViewProj;
	modelViewProj = DirectX::XMMatrixMultiply( model, view );
	modelViewProj = DirectX::XMMatrixMultiply( modelViewProj, projection );
	modelViewProj = DirectX::XMMatrixTranspose( modelViewProj );
	tmp.modelViewProj = modelViewProj;

	m_ChangedMatrices[MATERIAL_MODEL] = false;
	m_ChangedMatrices[MATERIAL_VIEW] = false;
	m_ChangedMatrices[MATERIAL_PROJECTION] = false;
	ConstantBuffer_t tbuffer = g_pShaderDeviceDx11->GetTransformConstantBuffer();
	IShaderConstantBuffer *pIBuf = (IShaderConstantBuffer *)g_pShaderDeviceDx11->GetConstantBuffer( tbuffer );
	//Log( "\tUpdating transform constant buffer at %p\n", pIBuf->GetBuffer() );
	g_pShaderDeviceDx11->UpdateConstantBuffer( tbuffer, &tmp );

	DirectX::XMFLOAT4X4 flt4x4;
	DirectX::XMStoreFloat4x4( &flt4x4, GetMatrix( MATERIAL_MODEL ) );
	OutputMatrix( flt4x4, "Model" );
	DirectX::XMStoreFloat4x4( &flt4x4, GetMatrix( MATERIAL_VIEW ) );
	OutputMatrix( flt4x4, "View" );
	DirectX::XMStoreFloat4x4( &flt4x4, GetMatrix( MATERIAL_PROJECTION ) );
	OutputMatrix( flt4x4, "Projection" );
	DirectX::XMStoreFloat4x4( &flt4x4, modelViewProj );
	OutputMatrix( flt4x4, "ModelViewProj" );
}

FORCEINLINE static void UploadConstantBuffer( CShaderConstantBufferDx11 *pBuffer )
{
	if ( pBuffer )
	{
		// This will check if the data has been changed.
		// If so, it will upload the new data to the GPU.
		pBuffer->UploadToGPU();
	}
}

void CShaderAPIDx11::DoIssueConstantBufferUpdates()
{
	for ( int i = 0; i < m_TargetState.dynamic.m_nVSConstantBuffers; i++ )
	{
		UploadConstantBuffer( m_TargetState.dynamic.m_ppVSConstantBuffers[i] );
	}
	for ( int i = 0; i < m_TargetState.dynamic.m_nGSConstantBuffers; i++ )
	{
		UploadConstantBuffer( m_TargetState.dynamic.m_ppGSConstantBuffers[i] );
	}
	for ( int i = 0; i < m_TargetState.dynamic.m_nPSConstantBuffers; i++ )
	{
		UploadConstantBuffer( m_TargetState.dynamic.m_ppPSConstantBuffers[i] );
	}
}

void CShaderAPIDx11::DoIssueVertexShader()
{
	D3D11DeviceContext()->VSSetShader( m_TargetState.dynamic.m_pVertexShader, NULL, 0 );
}

void CShaderAPIDx11::DoIssuePixelShader()
{
	D3D11DeviceContext()->PSSetShader( m_TargetState.dynamic.m_pPixelShader, NULL, 0 );
}

void CShaderAPIDx11::DoIssueGeometryShader()
{
	D3D11DeviceContext()->GSSetShader( m_TargetState.dynamic.m_pGeometryShader, NULL, 0 );
}

bool CShaderAPIDx11::DoIssueConstantBuffers( bool bForce )
{
	bool bPSChanged = false;
	bool bVSChanged = false;
	bool bGSChanged = false;

	const StatesDx11::DynamicState &targetDynamic = m_TargetState.dynamic;
	const StatesDx11::DynamicState &dynamic = m_State.dynamic;

	if ( m_TargetState.dynamic.m_pVertexShader &&
		( targetDynamic.m_nVSConstantBuffers != dynamic.m_nVSConstantBuffers ||
		  memcmp( targetDynamic.m_ppVSConstantBuffers, dynamic.m_ppVSConstantBuffers,
			  sizeof( CShaderConstantBufferDx11 * ) * MAX_DX11_CBUFFERS ) ) )
	{
		ID3D11Buffer *ppBufs[MAX_DX11_CBUFFERS];
		for ( int i = 0; i < targetDynamic.m_nVSConstantBuffers; i++ )
		{
			//Log( "\tUsing constant buffer %p for vertex shader\n", targetDynamic.m_ppVSConstantBuffers[i]->GetD3DBuffer() );
			ppBufs[i] = targetDynamic.m_ppVSConstantBuffers[i]->GetD3DBuffer();
		}
		D3D11DeviceContext()->VSSetConstantBuffers( 0, targetDynamic.m_nVSConstantBuffers,
							    ppBufs );

		bVSChanged = true;
	}

	if ( m_TargetState.dynamic.m_pGeometryShader &&
		( targetDynamic.m_nGSConstantBuffers != dynamic.m_nGSConstantBuffers ||
		  memcmp( targetDynamic.m_ppGSConstantBuffers, dynamic.m_ppGSConstantBuffers,
			  sizeof( CShaderConstantBufferDx11 * ) * MAX_DX11_CBUFFERS ) ) )
	{
		ID3D11Buffer *ppBufs[MAX_DX11_CBUFFERS];
		for ( int i = 0; i < targetDynamic.m_nGSConstantBuffers; i++ )
		{
			ppBufs[i] = targetDynamic.m_ppGSConstantBuffers[i]->GetD3DBuffer();
		}
		D3D11DeviceContext()->GSSetConstantBuffers( 0, targetDynamic.m_nGSConstantBuffers,
							    ppBufs );

		bGSChanged = true;
	}

	if ( m_TargetState.dynamic.m_pVertexShader &&
		( targetDynamic.m_nPSConstantBuffers != dynamic.m_nPSConstantBuffers ||
		  memcmp( targetDynamic.m_ppPSConstantBuffers, dynamic.m_ppPSConstantBuffers,
			  sizeof( CShaderConstantBufferDx11 * ) * MAX_DX11_CBUFFERS ) ) )
	{
		ID3D11Buffer *ppBufs[MAX_DX11_CBUFFERS];
		for ( int i = 0; i < targetDynamic.m_nPSConstantBuffers; i++ )
		{
			ppBufs[i] = targetDynamic.m_ppPSConstantBuffers[i]->GetD3DBuffer();
		}
		D3D11DeviceContext()->PSSetConstantBuffers( 0, targetDynamic.m_nPSConstantBuffers,
							    ppBufs );

		bPSChanged = true;
	}

	return bPSChanged || bVSChanged || bGSChanged;
}

void CShaderAPIDx11::DoIssueSampler()
{
	D3D11DeviceContext()->PSSetSamplers( 0, m_TargetState.dynamic.m_nSamplers,
					     m_TargetState.dynamic.m_ppSamplers );
}

void CShaderAPIDx11::DoIssueTexture()
{
	ID3D11DeviceContext *ctx = D3D11DeviceContext();
	ctx->PSSetShaderResources( 0, m_TargetState.dynamic.m_nTextures,
					m_TargetState.dynamic.m_ppTextureViews );
}

void CShaderAPIDx11::DoIssueRasterState()
{
	m_TargetState.dynamic.m_pRasterState = NULL;

	// Clear out the existing state
	if ( m_State.dynamic.m_pRasterState )
	{
		m_State.dynamic.m_pRasterState->Release();
		m_State.dynamic.m_pRasterState = NULL;
	}

	//D3D11_RASTERIZER_DESC desc;
	//GenerateRasterizerDesc( &desc, m_TargetState );

	// NOTE: This does a search for existing matching state objects
	HRESULT hr = D3D11Device()->CreateRasterizerState( &m_TargetState.shadow.rasterizer,
							   &m_TargetState.dynamic.m_pRasterState );
	if ( FAILED( hr ) )
	{
		Warning( "Unable to create rasterizer state object!\n" );
	}

	D3D11DeviceContext()->RSSetState( m_TargetState.dynamic.m_pRasterState );
}

void CShaderAPIDx11::DoIssueBlendState()
{
	m_TargetState.dynamic.m_pBlendState = NULL;

	// Clear out existing blend state
	if ( m_State.dynamic.m_pBlendState )
	{
		m_State.dynamic.m_pBlendState->Release();
		m_State.dynamic.m_pBlendState = NULL;
	}

	//D3D11_BLEND_DESC desc;
	//GenerateBlendDesc( &desc, m_TargetState );

	// NOTE: This does a search for existing matching state objects
	HRESULT hr = D3D11Device()->CreateBlendState( &m_TargetState.shadow.blend, &m_TargetState.dynamic.m_pBlendState );
	if ( FAILED( hr ) )
	{
		Warning( "Unable to create Dx11 blend state object!\n" );
	}

	D3D11DeviceContext()->OMSetBlendState( m_TargetState.dynamic.m_pBlendState,
					       m_TargetState.shadow.blend.BlendColor,
					       m_TargetState.shadow.blend.SampleMask );
}

void CShaderAPIDx11::DoIssueDepthStencilState()
{
	m_TargetState.dynamic.m_pDepthStencilState = NULL;

	// Clear out current state
	if ( m_State.dynamic.m_pDepthStencilState )
	{
		m_State.dynamic.m_pDepthStencilState->Release();
		m_State.dynamic.m_pDepthStencilState = NULL;
	}

	//D3D11_DEPTH_STENCIL_DESC desc;
	//GenerateDepthStencilDesc( &desc, m_TargetState );

	// NOTE: This does a search for existing matching state objects
	HRESULT hr = D3D11Device()->CreateDepthStencilState( &m_TargetState.shadow.depthStencil,
							     &m_TargetState.dynamic.m_pDepthStencilState );
	if ( FAILED( hr ) )
	{
		Warning( "Unable to create depth/stencil object!\n" );
	}

	D3D11DeviceContext()->OMSetDepthStencilState( m_TargetState.dynamic.m_pDepthStencilState,
						      m_TargetState.shadow.depthStencil.StencilRef );
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
		const StatesDx11::VertexBufferState &newState = m_TargetState.dynamic.m_pVertexBuffer[i];
		bool bMatch = !bForce && !memcmp( &newState, &m_State.dynamic.m_pVertexBuffer[i],
						  sizeof( StatesDx11::VertexBufferState ) );
		if ( !bMatch )
		{
			ppVertexBuffers[i] = newState.m_pBuffer;
			pStrides[i] = newState.m_nStride;
			pOffsets[i] = newState.m_nOffset;
			++nBufferCount;
			memcpy( &m_State.dynamic.m_pVertexBuffer[i], &newState, sizeof( StatesDx11::VertexBufferState ) );
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
	StatesDx11::DynamicState &dynamic = m_TargetState.dynamic;
	D3D11DeviceContext()->IASetIndexBuffer( dynamic.m_IndexBuffer.m_pBuffer,
						dynamic.m_IndexBuffer.m_Format,
						dynamic.m_IndexBuffer.m_nOffset );
}

void CShaderAPIDx11::DoIssueInputLayout()
{
	// FIXME: Deal with multiple streams
	ID3D11InputLayout *pInputLayout = g_pShaderDeviceDx11->GetInputLayout(
		m_TargetState.dynamic.m_InputLayout.m_hVertexShader,
		m_TargetState.dynamic.m_InputLayout.m_pVertexDecl[0] );
	D3D11DeviceContext()->IASetInputLayout( pInputLayout );
}

void CShaderAPIDx11::DoIssueTopology()
{
	D3D11DeviceContext()->IASetPrimitiveTopology( m_TargetState.dynamic.m_Topology );
}

void CShaderAPIDx11::DoIssueViewports()
{
	D3D11DeviceContext()->RSSetViewports( m_TargetState.dynamic.m_nViewportCount,
					      m_TargetState.dynamic.m_pViewports );
}

void CShaderAPIDx11::DoIssueRenderTargets()
{
	if ( !m_TargetState.dynamic.m_pRenderTargetView )
	{
		return;
	}

	D3D11DeviceContext()->OMSetRenderTargets(
		1, &m_TargetState.dynamic.m_pRenderTargetView,
		m_TargetState.dynamic.m_pDepthStencilView );
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
	m_TargetState.dynamic.m_nViewportCount = nCount;

	for ( int i = 0; i < nCount; ++i )
	{
		Assert( pViewports[i].m_nVersion == SHADER_VIEWPORT_VERSION );

		D3D11_VIEWPORT &viewport = m_TargetState.dynamic.m_pViewports[i];
		viewport.TopLeftX	 = pViewports[i].m_nTopLeftX;
		viewport.TopLeftY	 = pViewports[i].m_nTopLeftY;
		viewport.Width		 = pViewports[i].m_nWidth;
		viewport.Height		 = pViewports[i].m_nHeight;
		viewport.MinDepth	 = pViewports[i].m_flMinZ;
		viewport.MaxDepth	 = pViewports[i].m_flMaxZ;
	}
	Log( "CShaderAPIDx11::SetViewports\n" );
}

int CShaderAPIDx11::GetViewports( ShaderViewport_t *pViewports, int nMax ) const
{
	int nCount = m_State.dynamic.m_nViewportCount;
	if ( pViewports && nMax )
	{
		nCount = min( nCount, nMax );
		memcpy( pViewports, m_State.dynamic.m_pViewports, nCount * sizeof( ShaderViewport_t ) );
	}
	return nCount;
}

//-----------------------------------------------------------------------------
// Methods related to clearing buffers
//-----------------------------------------------------------------------------
void CShaderAPIDx11::ClearColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
	m_TargetState.dynamic.m_ClearColor[0] = r / 255.0f;
	m_TargetState.dynamic.m_ClearColor[1] = g / 255.0f;
	m_TargetState.dynamic.m_ClearColor[2] = b / 255.0f;
	m_TargetState.dynamic.m_ClearColor[3] = 1.0f;
}

void CShaderAPIDx11::ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	m_TargetState.dynamic.m_ClearColor[0] = r / 255.0f;
	m_TargetState.dynamic.m_ClearColor[1] = g / 255.0f;
	m_TargetState.dynamic.m_ClearColor[2] = b / 255.0f;
	m_TargetState.dynamic.m_ClearColor[3] = a / 255.0f;
}

void CShaderAPIDx11::ClearBuffers( bool bClearColor, bool bClearDepth, bool bClearStencil, int renderTargetWidth, int renderTargetHeight )
{
	// NOTE: State change commit isn't necessary since clearing doesn't use state
	//	IssueStateChanges();

	m_TargetState;

	// FIXME: This implementation is totally bust0red [doesn't guarantee exact color specified]
	if ( bClearColor && D3D11DeviceContext() && m_TargetState.dynamic.m_pRenderTargetView )
	{
		D3D11DeviceContext()->ClearRenderTargetView( m_TargetState.dynamic.m_pRenderTargetView, m_TargetState.dynamic.m_ClearColor );
	}

	if ( bClearDepth && D3D11DeviceContext() && m_TargetState.dynamic.m_pDepthStencilView )
	{
		UINT clearFlags = 0;
		if ( bClearDepth )
		{
			clearFlags |= D3D11_CLEAR_DEPTH;
		}
		if ( bClearStencil )
		{
			clearFlags |= D3D11_CLEAR_STENCIL;
		}
		D3D11DeviceContext()->ClearDepthStencilView( m_TargetState.dynamic.m_pDepthStencilView, clearFlags, 1.0f, 0 );
	}
}

//-----------------------------------------------------------------------------
// Methods related to binding shaders
//-----------------------------------------------------------------------------
void CShaderAPIDx11::BindVertexShader( VertexShaderHandle_t hVertexShader )
{
	ID3D11VertexShader *pVertexShader = g_pShaderDeviceDx11->GetVertexShader( hVertexShader );
	m_TargetState.dynamic.m_pVertexShader = pVertexShader;
	m_TargetState.dynamic.m_InputLayout.m_hVertexShader = hVertexShader;
}

void CShaderAPIDx11::BindGeometryShader( GeometryShaderHandle_t hGeometryShader )
{
	ID3D11GeometryShader *pGeometryShader = g_pShaderDeviceDx11->GetGeometryShader( hGeometryShader );
	m_TargetState.dynamic.m_pGeometryShader = pGeometryShader;
}

void CShaderAPIDx11::BindPixelShader( PixelShaderHandle_t hPixelShader )
{
	ID3D11PixelShader *pPixelShader = g_pShaderDeviceDx11->GetPixelShader( hPixelShader );
	m_TargetState.dynamic.m_pPixelShader = pPixelShader;
}

void CShaderAPIDx11::BindVertexBuffer( int nStreamID, IVertexBuffer *pVertexBuffer, int nOffsetInBytes, int nFirstVertex, int nVertexCount, VertexFormat_t fmt, int nRepetitions )
{
	// FIXME: What to do about repetitions?
	CVertexBufferDx11 *pVertexBufferDx11 = static_cast<CVertexBufferDx11 *>( pVertexBuffer );

	StatesDx11::VertexBufferState state;
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

	m_TargetState.dynamic.m_pVertexBuffer[nStreamID] = state;
	m_TargetState.dynamic.m_InputLayout.m_pVertexDecl[nStreamID] = fmt;
}

void CShaderAPIDx11::BindIndexBuffer( IIndexBuffer *pIndexBuffer, int nOffsetInBytes )
{
	CIndexBufferDx11 *pIndexBufferDx11 = static_cast<CIndexBufferDx11 *>( pIndexBuffer );

	StatesDx11::IndexBufferState state;
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

	m_TargetState.dynamic.m_IndexBuffer = state;
}

//-----------------------------------------------------------------------------
// Unbinds resources because they are about to be deleted
//-----------------------------------------------------------------------------
void CShaderAPIDx11::Unbind( VertexShaderHandle_t hShader )
{
	ID3D11VertexShader *pShader = g_pShaderDeviceDx11->GetVertexShader( hShader );
	Assert( pShader );
	if ( m_TargetState.dynamic.m_pVertexShader == pShader )
	{
		BindVertexShader( VERTEX_SHADER_HANDLE_INVALID );
	}
	//if ( m_State.dynamic.m_pVertexShader == pShader )
	//{
	//	IssueStateChanges();
	//}
}

void CShaderAPIDx11::Unbind( GeometryShaderHandle_t hShader )
{
	ID3D11GeometryShader *pShader = g_pShaderDeviceDx11->GetGeometryShader( hShader );
	Assert( pShader );
	if ( m_TargetState.dynamic.m_pGeometryShader == pShader )
	{
		BindGeometryShader( GEOMETRY_SHADER_HANDLE_INVALID );
	}
	//if ( m_State.dynamic.m_pGeometryShader == pShader )
	//{
	//	IssueStateChanges();
	//}
}

void CShaderAPIDx11::Unbind( PixelShaderHandle_t hShader )
{
	ID3D11PixelShader *pShader = g_pShaderDeviceDx11->GetPixelShader( hShader );
	Assert( pShader );
	if ( m_TargetState.dynamic.m_pPixelShader == pShader )
	{
		BindPixelShader( PIXEL_SHADER_HANDLE_INVALID );
	}
	//if ( m_State.dynamic.m_pPixelShader == pShader )
	//{
	//	IssueStateChanges();
	//}
}

void CShaderAPIDx11::UnbindVertexBuffer( ID3D11Buffer *pBuffer )
{
	Assert( pBuffer );

	for ( int i = 0; i < MAX_DX11_STREAMS; ++i )
	{
		if ( m_TargetState.dynamic.m_pVertexBuffer[i].m_pBuffer == pBuffer )
		{
			BindVertexBuffer( i, NULL, 0, 0, 0, VERTEX_POSITION, 0 );
		}
	}
	//for ( int i = 0; i < MAX_DX11_STREAMS; ++i )
	//{
	//	if ( m_State.dynamic.m_pVertexBuffer[i].m_pBuffer == pBuffer )
	//	{
	//		IssueStateChanges();
	//		break;
	//	}
	//}
}

void CShaderAPIDx11::UnbindIndexBuffer( ID3D11Buffer *pBuffer )
{
	Assert( pBuffer );

	if ( m_TargetState.dynamic.m_IndexBuffer.m_pBuffer == pBuffer )
	{
		BindIndexBuffer( NULL, 0 );
	}
	//if ( m_State.dynamic.m_IndexBuffer.m_pBuffer == pBuffer )
	//{
	//	IssueStateChanges();
	//}
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

	////Log( "Set topology to %i\n", d3dTopology );
	m_TargetState.dynamic.m_Topology = d3dTopology;
}

//-----------------------------------------------------------------------------
// Mesh/Material rendering
//-----------------------------------------------------------------------------
void CShaderAPIDx11::DrawMesh( IMesh *pMesh )
{
	VPROF( "CShaderAPIDx11::DrawMesh" );
	if ( ShaderUtil()->GetConfig().m_bSuppressRendering )
		return;

	Log( "CShaderAPIDx11::DrawMesh %p\n", pMesh );

	m_pMesh = static_cast<CMeshBase *>( pMesh );
	if ( !m_pMesh || !m_pMaterial )
	{
		Warning( "Tried to render mesh with NULL mesh or NULL material!\n" );
		return;
	}
	m_pMaterial->DrawMesh( CompressionType( pMesh->GetVertexFormat() ) );
	m_pMesh = NULL;
}

static int s_nPassesRendered = 0;

// Begins a rendering pass that uses a state snapshot
void CShaderAPIDx11::BeginPass( StateSnapshot_t snapshot )
{
	m_CurrentSnapshot = snapshot;

	// Apply the snapshot state
	if ( snapshot != -1 )
		UseSnapshot( m_CurrentSnapshot );
}

// Renders a single pass of a material
void CShaderAPIDx11::RenderPass( int nPass, int nPassCount )
{
	if ( g_pShaderDeviceDx11->IsDeactivated() )
		return;

	IssueStateChanges();

	// Now actually render

	if ( m_pMesh )
	{
		m_pMesh->RenderPass();
	}
	else
	{
		Assert( 0 );
		RenderPassWithVertexAndIndexBuffers();
	}

	m_CurrentSnapshot = -1;
}

void CShaderAPIDx11::RenderPassWithVertexAndIndexBuffers()
{
	if ( m_State.dynamic.m_Topology == D3D11_PRIMITIVE_TOPOLOGY_POINTLIST )
	{
		Assert( 0 );
	}
	else
	{
		//DrawIndexed(m_State.dynamic.m_IndexBuffer.)
	}
}

// Draws primitives
void CShaderAPIDx11::Draw( MaterialPrimitiveType_t primitiveType, int nFirstIndex, int nIndexCount )
{
	Log( "ShaderAPIDx11: Draw\n" );

	SetTopology( primitiveType );

	IssueStateChanges();

	// FIXME: How do I set the base vertex location!?
	DrawIndexed( nFirstIndex, nIndexCount, 0 );
}

void CShaderAPIDx11::DrawIndexed( int nFirstIndex, int nIndexCount, int nBaseVertexLocation )
{
	D3D11DeviceContext()->DrawIndexed( (UINT)nIndexCount, (UINT)nFirstIndex, (UINT)nBaseVertexLocation );
}

void CShaderAPIDx11::DrawNotIndexed( int nFirstVertex, int nVertCount )
{
	D3D11DeviceContext()->Draw( (UINT)nVertCount, (UINT)nFirstVertex );
}

//-----------------------------------------------------------------------------//

bool CShaderAPIDx11::OnDeviceInit()
{
	// Initialize the mesh manager
	MeshMgr()->Init();

	int w, h;
	g_pShaderDeviceDx11->GetBackBufferDimensions( w, h );
	
	{
		LOCK_SHADERAPI();
		// Create a the back buffer view
		// UNDONE: Should texture creation and access be moved to ShaderDeviceDx11?
		ID3D11Texture2D *pBackBuffer;
		HRESULT hr = D3D11SwapChain()->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID *)&pBackBuffer );
		if ( FAILED( hr ) )
			return FALSE;
		m_hBackBuffer = CreateTextureHandle();
		CTextureDx11 *pTex = &GetTexture( m_hBackBuffer );
		pTex->SetupBackBuffer( w, h, "dx11BackBuffer", pBackBuffer );
	}

	// Create the depth buffer
	m_hDepthBuffer = CreateDepthTexture( IMAGE_FORMAT_NV_DST24, w, h, "dx11DepthBuffer", true );

	ResetRenderState();

	return true;
}

void CShaderAPIDx11::OnDeviceShutdown()
{
	for ( int i = m_Textures.Head(); i != m_Textures.InvalidIndex(); i = m_Textures.Next( i ) )
	{
		CTextureDx11 *pTex = &m_Textures[i];
		pTex->Delete();
	}
	m_Textures.RemoveAll();
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
	if ( IsDeactivated() )
		return false;

	return true;
}

// Used to clear the transition table when we know it's become invalid.
void CShaderAPIDx11::ClearSnapshots()
{
	g_pShaderShadowDx11->m_ShadowStateCache.RemoveAll();
}

// Sets the default *dynamic* state
void CShaderAPIDx11::SetDefaultState()
{
	m_TargetState.dynamic.m_nVSConstantBuffers = 0;
	m_TargetState.dynamic.m_nGSConstantBuffers = 0;
	m_TargetState.dynamic.m_nPSConstantBuffers = 0;

	ZeroMemory( m_TargetState.dynamic.m_ppVSConstantBuffers, sizeof( CShaderConstantBufferDx11 * ) * MAX_DX11_CBUFFERS );
	ZeroMemory( m_TargetState.dynamic.m_ppGSConstantBuffers, sizeof( CShaderConstantBufferDx11 * ) * MAX_DX11_CBUFFERS );
	ZeroMemory( m_TargetState.dynamic.m_ppPSConstantBuffers, sizeof( CShaderConstantBufferDx11 * ) * MAX_DX11_CBUFFERS );

	m_TargetState.dynamic.m_nSamplers = 0;
	m_TargetState.dynamic.m_nTextures = 0;

	ZeroMemory( m_TargetState.dynamic.m_ppSamplers, sizeof( ID3D11SamplerState * ) * MAX_DX11_SAMPLERS );
	ZeroMemory( m_TargetState.dynamic.m_ppTextureViews, sizeof( ID3D11ShaderResourceView * ) * MAX_DX11_SAMPLERS );
	ZeroMemory( m_TargetState.dynamic.m_ppTextures, sizeof( CTextureDx11 * ) * MAX_DX11_SAMPLERS );

	//m_TargetState.dynamic.m_pVertexShader = 0;
	//m_TargetState.dynamic.m_pGeometryShader = 0;
	////m_TargetState.dynamic.m_pPixelShader = 0;
	//m_TargetState.dynamic.m_iVertexShader = -1;
	//m_TargetState.dynamic.m_iGeometryShader = -1;
	//m_TargetState.dynamic.m_iPixelShader = -1;
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

//-----------------------------------------------------------------------------
// Gets the bound morph's vertex format; returns 0 if no morph is bound
//-----------------------------------------------------------------------------
MorphFormat_t CShaderAPIDx11::GetBoundMorphFormat()
{
	return ShaderUtil()->GetBoundMorphFormat();
}

//-----------------------------------------------------------------------------
// What fields in the morph do we actually use?
//-----------------------------------------------------------------------------
MorphFormat_t CShaderAPIDx11::ComputeMorphFormat( int numSnapshots, StateSnapshot_t *pIds ) const
{
	LOCK_SHADERAPI();
	MorphFormat_t format = 0;
	for ( int i = 0; i < numSnapshots; ++i )
	{
		MorphFormat_t fmt = g_pShaderShadowDx11->GetShadowState( pIds[i] ).morphFormat;
		format |= VertexFlags( fmt );
	}
	return format;
}

// Gets the vertex format for a set of snapshot ids
VertexFormat_t CShaderAPIDx11::ComputeVertexFormat( int numSnapshots, StateSnapshot_t *pIds ) const
{
	LOCK_SHADERAPI();
	VertexFormat_t fmt = ComputeVertexUsage( numSnapshots, pIds );
	return fmt;
}

// Gets the vertex format for a set of snapshot ids
VertexFormat_t CShaderAPIDx11::ComputeVertexUsage( int num, StateSnapshot_t *pIds ) const
{
	LOCK_SHADERAPI();
	if ( num == 0 )
		return 0;

	// We don't have to all sorts of crazy stuff if there's only one snapshot
	if ( num == 1 )
	{
		const StatesDx11::ShadowState &state = g_pShaderShadowDx11->GetShadowState( pIds[0] );
		return state.vertexFormat;
	}

	Assert( pIds );

	// Aggregating vertex formats is a little tricky;
	// For example, what do we do when two passes want user data? 
	// Can we assume they are the same? For now, I'm going to
	// just print a warning in debug.

	VertexCompressionType_t compression = VERTEX_COMPRESSION_INVALID;
	int userDataSize = 0;
	int numBones = 0;
	int texCoordSize[VERTEX_MAX_TEXTURE_COORDINATES] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int flags = 0;

	for ( int i = num; --i >= 0; )
	{
		Log( "Applying vertex format from snapshot num %i, %i\n", i, pIds[i] );
		const StatesDx11::ShadowState &state = g_pShaderShadowDx11->GetShadowState( pIds[i] );
		VertexFormat_t fmt = state.vertexFormat;
		flags |= VertexFlags( fmt );

		VertexCompressionType_t newCompression = CompressionType( fmt );
		if ( ( compression != newCompression ) && ( compression != VERTEX_COMPRESSION_INVALID ) )
		{
			Warning( "Encountered a material with two passes that specify different vertex compression types!\n" );
			compression = VERTEX_COMPRESSION_NONE; // Be safe, disable compression
		}

		int newNumBones = NumBoneWeights( fmt );
		if ( ( numBones != newNumBones ) && ( newNumBones != 0 ) )
		{
			if ( numBones != 0 )
			{
				Warning( "Encountered a material with two passes that use different numbers of bones!\n" );
			}
			numBones = newNumBones;
		}

		int newUserSize = UserDataSize( fmt );
		if ( ( userDataSize != newUserSize ) && ( newUserSize != 0 ) )
		{
			if ( userDataSize != 0 )
			{
				Warning( "Encountered a material with two passes that use different user data sizes!\n" );
			}
			userDataSize = newUserSize;
		}

		for ( int j = 0; j < VERTEX_MAX_TEXTURE_COORDINATES; ++j )
		{
			int newSize = TexCoordSize( (TextureStage_t)j, fmt );
			if ( ( texCoordSize[j] != newSize ) && ( newSize != 0 ) )
			{
				if ( texCoordSize[j] != 0 )
				{
					Warning( "Encountered a material with two passes that use different texture coord sizes!\n" );
				}
				if ( texCoordSize[j] < newSize )
				{
					texCoordSize[j] = newSize;
				}
			}
		}
	}

	return MeshMgr()->ComputeVertexFormat( flags, VERTEX_MAX_TEXTURE_COORDINATES,
					       texCoordSize, numBones, userDataSize );
}

// Uses a state snapshot
void CShaderAPIDx11::UseSnapshot( StateSnapshot_t snapshot )
{
	StatesDx11::ShadowState entry = g_pShaderShadowDx11->m_ShadowStateCache.Element( snapshot );
	m_TargetState.shadow = entry;

	ShaderManager()->SetVertexShaderIndex( entry.vertexShaderIndex );
	ShaderManager()->SetVertexShader( entry.vertexShader );

	ShaderManager()->SetPixelShaderIndex( entry.pixelShaderIndex );
	ShaderManager()->SetPixelShader( entry.pixelShader );
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

// Binds a particular material to render with
void CShaderAPIDx11::Bind( IMaterial *pMaterial )
{
	LOCK_SHADERAPI();

	IMaterialInternal *pMatInt = static_cast<IMaterialInternal *>( pMaterial );

	bool bMaterialChanged;
	if ( m_pMaterial && pMatInt && m_pMaterial->InMaterialPage() && pMatInt->InMaterialPage() )
	{
		bMaterialChanged = ( m_pMaterial->GetMaterialPage() != pMatInt->GetMaterialPage() );
	}
	else
	{
		bMaterialChanged = ( m_pMaterial != pMatInt ) || ( m_pMaterial && m_pMaterial->InMaterialPage() ) || ( pMatInt && pMatInt->InMaterialPage() );
	}

	if ( bMaterialChanged )
	{
		FlushBufferedPrimitives();
		m_pMaterial = pMatInt;
	}
}

IMaterialInternal *CShaderAPIDx11::GetBoundMaterial() const
{
	return m_pMaterial;
}

// Cull mode
void CShaderAPIDx11::CullMode( MaterialCullMode_t cullMode )
{
	D3D11_CULL_MODE d3dCull;
	switch ( cullMode )
	{
	case MATERIAL_CULLMODE_CCW:
		d3dCull = D3D11_CULL_BACK;
		break;
	case MATERIAL_CULLMODE_CW:
		d3dCull = D3D11_CULL_FRONT;
		break;
	default:
		d3dCull = D3D11_CULL_NONE;
		break;
	}
	m_TargetState.shadow.rasterizer.CullMode = d3dCull;
}

D3D11_CULL_MODE CShaderAPIDx11::GetCullMode() const
{
	return m_State.shadow.rasterizer.CullMode;
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

void CShaderAPIDx11::SetSkinningMatrices()
{
}

// Gets the lightmap dimensions
void CShaderAPIDx11::GetLightmapDimensions( int *w, int *h )
{
	g_pShaderUtil->GetLightmapDimensions( w, h );
}

// Flushes any primitives that are buffered
void CShaderAPIDx11::FlushBufferedPrimitives()
{
	if ( ShaderUtil() )
	{
		if ( !ShaderUtil()->OnFlushBufferedPrimitives() )
		{
			return;
		}
	}

	LOCK_SHADERAPI();
	// This shouldn't happen in the inner rendering loop!
	Assert( m_pMesh == 0 );

	// NOTE: We've gotta store off the matrix mode because
	// it'll get reset by the default state application caused by the flush
	MaterialMatrixMode_t oldMatMode = m_MatrixMode;

	MeshMgr()->Flush();

	m_MatrixMode = oldMatMode;
}

// Creates/destroys Mesh
IMesh *CShaderAPIDx11::CreateStaticMesh( VertexFormat_t fmt, const char *pTextureBudgetGroup, IMaterial *pMaterial )
{
	return MeshMgr()->CreateStaticMesh( fmt, pTextureBudgetGroup, pMaterial );
}

void CShaderAPIDx11::DestroyStaticMesh( IMesh *mesh )
{
	MeshMgr()->DestroyStaticMesh( mesh );
}

// Gets the dynamic mesh; note that you've got to render the mesh
// before calling this function a second time. Clients should *not*
// call DestroyStaticMesh on the mesh returned by this call.
IMesh *CShaderAPIDx11::GetDynamicMesh( IMaterial *pMaterial, int nHWSkinBoneCount, bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride )
{
	return MeshMgr()->GetDynamicMesh( pMaterial, 0, nHWSkinBoneCount, buffered, pVertexOverride, pIndexOverride );
}

IMesh *CShaderAPIDx11::GetDynamicMeshEx( IMaterial *pMaterial, VertexFormat_t fmt, int nHWSkinBoneCount, bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride )
{
	return MeshMgr()->GetDynamicMesh( pMaterial, fmt, nHWSkinBoneCount, buffered, pVertexOverride, pIndexOverride );
}

IVertexBuffer *CShaderAPIDx11::GetDynamicVertexBuffer( IMaterial *pMaterial, bool buffered )
{
	return MeshMgr()->GetDynamicVertexBuffer( pMaterial, buffered );
}

IIndexBuffer *CShaderAPIDx11::GetDynamicIndexBuffer( IMaterial *pMaterial, bool buffered )
{
	return MeshMgr()->GetDynamicIndexBuffer( pMaterial, buffered );
}

IMesh *CShaderAPIDx11::GetFlexMesh()
{
	return MeshMgr()->GetFlexMesh();
}

bool CShaderAPIDx11::IsDeactivated() const
{
	return g_pShaderDeviceDx11->IsDeactivated();
}

// stuff related to matrix stacks
//
// Note Dx11 doesn't have a matrix stack, you just supply
// the matrices to the shader in a constant buffer, so we will
// not break compatibility with Dx9 and just emulate the behavior.

bool CShaderAPIDx11::MatrixIsChanging() const
{
	if ( IsDeactivated() )
	{
		return false;
	}

	return false;
}

void CShaderAPIDx11::HandleMatrixModified()
{
	char materialname[20];
	switch ( m_MatrixMode )
	{
	case MATERIAL_MODEL:
		sprintf( materialname, "Model" );
		break;
	case MATERIAL_VIEW:
		sprintf( materialname, "View" );
		break;
	case MATERIAL_PROJECTION:
		sprintf( materialname, "Projection" );
		break;
	default:
		sprintf( materialname, "Other: %i", m_MatrixMode );
		break;
	}
	//Log( "Matrix modified: %s\n", materialname );
	m_ChangedMatrices[m_MatrixMode] = true;
}

DirectX::XMMATRIX &CShaderAPIDx11::GetMatrix( MaterialMatrixMode_t mode )
{
	CUtlStack<MatrixItemDx11_t> &curStack = m_MatrixStacks[mode];
	if ( !curStack.Count() )
	{
		return DirectX::XMMatrixIdentity();
	}

	return m_MatrixStacks[mode].Top().m_Matrix;
}

DirectX::XMMATRIX &CShaderAPIDx11::GetCurrentMatrix()
{
	return m_pCurMatrixItem->m_Matrix;
}

DirectX::XMMATRIX CShaderAPIDx11::GetMatrixCopy( MaterialMatrixMode_t mode ) const
{
	const CUtlStack<MatrixItemDx11_t> &curStack = m_MatrixStacks[mode];
	if ( !curStack.Count() )
	{
		return DirectX::XMMatrixIdentity();
	}

	return m_MatrixStacks[mode].Top().m_Matrix;
}

DirectX::XMMATRIX CShaderAPIDx11::GetCurrentMatrixCopy() const
{
	return m_pCurMatrixItem->m_Matrix;
}

void CShaderAPIDx11::MatrixMode( MaterialMatrixMode_t matrixMode )
{
	Assert( m_MatrixStacks[matrixMode].Count() );
	m_MatrixMode = matrixMode;
	m_pCurMatrixItem = &m_MatrixStacks[matrixMode].Top();
}

void CShaderAPIDx11::PushMatrix()
{
	// Does nothing in Dx11
	//GetCurrentMatrix() = DirectX::XMMatrixTranspose( GetCurrentMatrix() );

	CUtlStack<MatrixItemDx11_t> &curStack = m_MatrixStacks[m_MatrixMode];
	Assert( curStack.Count() );
	int iNew = m_MatrixStacks[m_MatrixMode].Push();
	curStack[iNew] = curStack[iNew - 1];
	m_pCurMatrixItem = &m_MatrixStacks[m_MatrixMode].Top();

	HandleMatrixModified();
}

void CShaderAPIDx11::PopMatrix()
{
	Assert( m_MatrixStacks[m_MatrixMode].Count() > 1 );
	m_MatrixStacks[m_MatrixMode].Pop();
	m_pCurMatrixItem = &m_MatrixStacks[m_MatrixMode].Top();

	HandleMatrixModified();
}

void CShaderAPIDx11::LoadMatrix( float *m )
{
	DirectX::XMFLOAT4X4 flt4x4( m );
	GetCurrentMatrix() = DirectX::XMLoadFloat4x4( &flt4x4 );
	HandleMatrixModified();
}

void CShaderAPIDx11::MultMatrix( float *m )
{
	DirectX::XMFLOAT4X4 flt4x4( m );
	GetCurrentMatrix() = DirectX::XMMatrixMultiply(
		GetCurrentMatrix(), DirectX::XMLoadFloat4x4( &flt4x4 ) );
	HandleMatrixModified();
}

void CShaderAPIDx11::MultMatrixLocal( float *m )
{
	// DX11FIXME: Local multiply
	DirectX::XMFLOAT4X4 flt4x4( m );
	GetCurrentMatrix() = DirectX::XMMatrixMultiply(
		DirectX::XMLoadFloat4x4( &flt4x4 ), GetCurrentMatrix() );
	HandleMatrixModified();
}

void CShaderAPIDx11::GetMatrix( MaterialMatrixMode_t matrixMode, float *dst )
{
	DirectX::XMFLOAT4X4 flt4x4;
	DirectX::XMStoreFloat4x4( &flt4x4, GetMatrix( matrixMode ) );
	memcpy( dst, &flt4x4, sizeof( DirectX::XMFLOAT4X4 ) );
}

void CShaderAPIDx11::GetMatrix( MaterialMatrixMode_t matrixMode, DirectX::XMMATRIX &mat )
{
	mat = GetMatrix( matrixMode );
}

void CShaderAPIDx11::LoadIdentity( void )
{
	m_MatrixStacks;
	m_pCurMatrixItem->m_Matrix = DirectX::XMMatrixIdentity();
	HandleMatrixModified();
}

void CShaderAPIDx11::LoadCameraToWorld( void )
{
	DirectX::XMVECTOR det;
	DirectX::XMMATRIX inv;
	inv = DirectX::XMMatrixInverse( &det, GetMatrix( MATERIAL_VIEW ) );

	// Kill translation
	// DX11FIXME
	//inv.r[3].m128_f32[0] = inv.r[3].m128_f32[1] = inv.r[3].m128_f32[2] = 0.0f;
	inv = DirectX::XMMatrixMultiply( inv, DirectX::XMMatrixTranslation( 0, 0, 0 ) );

	m_pCurMatrixItem->m_Matrix = inv;

	HandleMatrixModified();
}

// Get the current camera position in world space.
void CShaderAPIDx11::GetWorldSpaceCameraPosition( float *pPos ) const
{
	DirectX::XMFLOAT4X4 flt4x4;
	const DirectX::XMMATRIX &view = GetMatrixCopy( MATERIAL_VIEW );
	DirectX::XMStoreFloat4x4( &flt4x4, view );
	memcpy( pPos, &flt4x4, sizeof( DirectX::XMFLOAT4X4 ) );
}

void CShaderAPIDx11::Ortho( double left, double top, double right, double bottom, double zNear, double zFar )
{
	DirectX::XMMATRIX mat = DirectX::XMMatrixOrthographicOffCenterRH( left, right, bottom, top, zNear, zFar );

	// DX11FIXME: Local multiply
	GetCurrentMatrix() = DirectX::XMMatrixMultiply( mat, GetCurrentMatrix() );

	HandleMatrixModified();
}

void CShaderAPIDx11::PerspectiveX( double fovx, double aspect, double zNear, double zFar )
{
	float width = 2 * zNear * tan( fovx * M_PI / 360.0 );
	float height = width / aspect;
	DirectX::XMMATRIX mat;
	mat = DirectX::XMMatrixPerspectiveRH( width, height, zNear, zFar );

	// DX11FIXME: Local multiply
	GetCurrentMatrix() = DirectX::XMMatrixMultiply( mat, GetCurrentMatrix() );

	HandleMatrixModified();
}

void CShaderAPIDx11::PerspectiveOffCenterX( double fovx, double aspect, double zNear, double zFar, double bottom, double top, double left, double right )
{
	float width = 2 * zNear * tan( fovx * M_PI / 360.0 );
	float height = width / aspect;

	// bottom, top, left, right are 0..1 so convert to -1..1
	float flFrontPlaneLeft = -( width / 2.0f ) * ( 1.0f - left ) + left * ( width / 2.0f );
	float flFrontPlaneRight = -( width / 2.0f ) * ( 1.0f - right ) + right * ( width / 2.0f );
	float flFrontPlaneBottom = -( height / 2.0f ) * ( 1.0f - bottom ) + bottom * ( height / 2.0f );
	float flFrontPlaneTop = -( height / 2.0f ) * ( 1.0f - top ) + top * ( height / 2.0f );

	DirectX::XMMATRIX mat;
	mat = DirectX::XMMatrixPerspectiveOffCenterRH( flFrontPlaneLeft, flFrontPlaneRight, flFrontPlaneBottom,
						       flFrontPlaneTop, zNear, zFar );

	// DX11FIXME: Local multiply
	GetCurrentMatrix() = DirectX::XMMatrixMultiply( mat, GetCurrentMatrix() );

	HandleMatrixModified();
}

void CShaderAPIDx11::PickMatrix( int x, int y, int width, int height )
{
	ShaderViewport_t viewport;
	GetViewports( &viewport, 1 );

	int vx = viewport.m_nTopLeftX;
	int vy = viewport.m_nTopLeftX;
	int vwidth = viewport.m_nWidth;
	int vheight = viewport.m_nHeight;

	// Compute the location of the pick region in projection space...
	float px = 2.0 * (float)( x - vx ) / (float)vwidth - 1;
	float py = 2.0 * (float)( y - vy ) / (float)vheight - 1;
	float pw = 2.0 * (float)width / (float)vwidth;
	float ph = 2.0 * (float)height / (float)vheight;

	// we need to translate (px, py) to the origin
	// and scale so (pw,ph) -> (2, 2)
	DirectX::XMMATRIX mat;
	mat = DirectX::XMMatrixTranslation( -2.0f * py / ph, -2.0f * px / pw, 0.0f );
	mat = DirectX::XMMatrixMultiply( mat, DirectX::XMMatrixScaling( 2.0f / pw, 2.0f / ph, 1.0f ) );

	// DX11FIXME: Local multiply
	GetCurrentMatrix() = DirectX::XMMatrixMultiply( mat, GetCurrentMatrix() );

	HandleMatrixModified();
}

void CShaderAPIDx11::Rotate( float angle, float x, float y, float z )
{
	DirectX::XMVECTOR axis;
	axis = DirectX::XMVectorSet( x, y, z, 0 );

	// DX11FIXME: Local multiply
	GetCurrentMatrix() = DirectX::XMMatrixMultiply( DirectX::XMMatrixRotationAxis( axis, M_PI * angle / 180.0f ), GetCurrentMatrix() );

	HandleMatrixModified();
}

void CShaderAPIDx11::Translate( float x, float y, float z )
{
	// DX11FIXME: Local multiply
	GetCurrentMatrix() = DirectX::XMMatrixMultiply( DirectX::XMMatrixTranslation( x, y, z ), GetCurrentMatrix() );
	HandleMatrixModified();
}

void CShaderAPIDx11::Scale( float x, float y, float z )
{
	// DX11FIXME: Local multiply
	GetCurrentMatrix() = DirectX::XMMatrixMultiply( DirectX::XMMatrixScaling( x, y, z ), GetCurrentMatrix() );
	HandleMatrixModified();
}

void CShaderAPIDx11::ScaleXY( float x, float y )
{
	// DX11FIXME: Local multiply
	GetCurrentMatrix() = DirectX::XMMatrixMultiply( DirectX::XMMatrixScaling( x, y, 1.0f ), GetCurrentMatrix() );
	HandleMatrixModified();
}

// Fog methods...
void CShaderAPIDx11::FogMode( MaterialFogMode_t fogMode )
{
	m_DynamicState.m_FogMode = fogMode;
}

void CShaderAPIDx11::FogStart( float fStart )
{
	m_DynamicState.m_flFogStart = fStart;
}

void CShaderAPIDx11::FogEnd( float fEnd )
{
	m_DynamicState.m_flFogStart = fEnd;
}

void CShaderAPIDx11::SetFogZ( float fogZ )
{
	m_DynamicState.m_flFogZ = fogZ;
}

void CShaderAPIDx11::FogMaxDensity( float flMaxDensity )
{
	m_DynamicState.m_flFogMaxDensity = flMaxDensity;
}

void CShaderAPIDx11::GetFogDistances( float *fStart, float *fEnd, float *fFogZ )
{
	*fStart = m_DynamicState.m_flFogStart;
	*fEnd = m_DynamicState.m_flFogEnd;
	*fFogZ = m_DynamicState.m_flFogZ;
}

void CShaderAPIDx11::SceneFogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPIDx11::SceneFogMode( MaterialFogMode_t fogMode )
{
}

void CShaderAPIDx11::GetSceneFogColor( unsigned char *rgb )
{
	rgb[0] = 0;
	rgb[1] = 0;
	rgb[2] = 0;
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

// Sets the vertex and pixel shaders
void CShaderAPIDx11::SetVertexShaderIndex( int vshIndex )
{
	ShaderManager()->SetVertexShaderIndex( vshIndex );
	ShaderManager()->SetVertexShader( m_TargetState.shadow.vertexShader );
}

void CShaderAPIDx11::SetPixelShaderIndex( int pshIndex )
{
	ShaderManager()->SetPixelShaderIndex( pshIndex );
	ShaderManager()->SetPixelShader( m_TargetState.shadow.pixelShader );
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
	CTextureDx11 *pTex = &GetTexture( textureHandle );
	int iTex = m_TargetState.dynamic.m_nTextures++;
	int iSamp = m_TargetState.dynamic.m_nSamplers++;
	m_TargetState.dynamic.m_ppTextures[iTex] = pTex;
	m_TargetState.dynamic.m_ppTextureViews[iTex] = pTex->GetView();
	m_TargetState.dynamic.m_ppSamplers[iSamp] = pTex->GetSamplerState();
}

void CShaderAPIDx11::UnbindTexture( ShaderAPITextureHandle_t textureHandle )
{
	CTextureDx11 *pTex = &GetTexture( textureHandle );

	// Unbind the texture

	int iTex = -1;
	for ( int i = 0; i < m_TargetState.dynamic.m_nTextures; i++ )
	{
		if ( m_TargetState.dynamic.m_ppTextureViews[i] == pTex->GetView() )
		{
			iTex = i;
			break;
		}
	}

	if ( iTex == -1 )
		return;

	m_TargetState.dynamic.m_ppTextureViews[iTex] = NULL;
	for ( int i = iTex; i < m_TargetState.dynamic.m_nTextures - 1; i++ )
	{
		m_TargetState.dynamic.m_ppTextureViews[i] =
			m_TargetState.dynamic.m_ppTextureViews[i + 1];
	}
	m_TargetState.dynamic.m_ppTextureViews[m_TargetState.dynamic.m_nTextures--] = NULL;

	// Unbind the matching sampler

	int iSampler = -1;
	for ( int i = 0; i < m_TargetState.dynamic.m_nSamplers; i++ )
	{
		if ( m_TargetState.dynamic.m_ppSamplers[i] == pTex->GetSamplerState() )
		{
			iSampler = i;
			break;
		}
	}

	if ( iSampler == -1 )
		return;

	m_TargetState.dynamic.m_ppSamplers[iSampler] = NULL;
	for ( int i = iSampler; i < m_TargetState.dynamic.m_nSamplers - 1; i++ )
	{
		m_TargetState.dynamic.m_ppSamplers[i] =
			m_TargetState.dynamic.m_ppSamplers[i + 1];
	}
	m_TargetState.dynamic.m_ppSamplers[m_TargetState.dynamic.m_nSamplers--] = NULL;
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

void CShaderAPIDx11::AdvanceCurrentTextureCopy( ShaderAPITextureHandle_t texture )
{
	// May need to switch textures....
	CTextureDx11 &tex = GetTexture( texture );
	if ( tex.m_NumCopies > 1 )
	{
		if ( ++tex.m_CurrentCopy >= tex.m_NumCopies )
			tex.m_CurrentCopy = 0;

		// When the current copy changes, we need to make sure this texture
		// isn't bound to any stages any more; thereby guaranteeing the new
		// copy will be re-bound.
		UnbindTexture( texture );
	}
}

//-----------------------------------------------------------------------------
// Texture image upload
//
// level: mipmap level we are writing to
// cubeFace: face of the cubemap/array texture we are writing to
// dstFormat: unused
// zOffset: not sure
// width: image width
// height image height
// srcFormat: format of the source image data
// bSrcIsTiled: is the source image a tiled image
// imageData: pointer to the beginning of the source image data
//-----------------------------------------------------------------------------
void CShaderAPIDx11::TexImage2D( int level, int cubeFace, ImageFormat dstFormat, int zOffset, int width, int height,
				 ImageFormat srcFormat, bool bSrcIsTiled, void *imageData )
{
	LOCK_SHADERAPI();
	Assert( imageData );
	ShaderAPITextureHandle_t hModifyTexture = m_ModifyTextureHandle;
	if ( !m_Textures.IsValidIndex( hModifyTexture ) )
	{
		Log( "Invalid modify texture handle!\n" );
		return;

	}

	//if ( zOffset != 0 )
		//DebuggerBreak();
		

	Assert( ( width <= g_pHardwareConfig->Caps().m_MaxTextureWidth ) &&
		( height <= g_pHardwareConfig->Caps().m_MaxTextureHeight ) );

	// Blow off mip levels if we don't support mipmapping
	if ( !g_pHardwareConfig->SupportsMipmapping() && ( level > 0 ) )
	{
		Log( "Trying to image mip but we don't support mips!\n" );
		return;
	}

	// This test here just makes sure we don't try to download mipmap levels
	// if we weren't able to create them in the first place
	CTextureDx11 &tex = GetTexture( hModifyTexture );
	if ( level >= tex.m_NumLevels )
	{
		Log( "level >= tex.m_NumLevels\n" );
		return;
	}

	// May need to switch textures....
	if ( tex.m_SwitchNeeded )
	{
		AdvanceCurrentTextureCopy( hModifyTexture );
		tex.m_SwitchNeeded = false;
	}

	CTextureDx11::TextureLoadInfo_t info;
	info.m_TextureHandle = hModifyTexture;
	info.m_pTexture = GetD3DTexture( hModifyTexture );
	info.m_nLevel = level;
	info.m_nCopy = tex.m_CurrentCopy;
	info.m_CubeFaceID = (D3D11_TEXTURECUBE_FACE)cubeFace;
	info.m_nWidth = width;
	info.m_nHeight = height;
	info.m_nZOffset = zOffset;
	info.m_SrcFormat = srcFormat;
	info.m_pSrcData = (unsigned char *)imageData;
	tex.LoadTexImage( info );
}

void CShaderAPIDx11::TexSubImage2D( int level, int cubeFace, int xOffset, int yOffset, int zOffset, int width, int height,
				    ImageFormat srcFormat, int srcStride, bool bSrcIsTiled, void *imageData )
{
	Log( "TexSubImage2D!\n" );

	LOCK_SHADERAPI();
	Assert( imageData );
	ShaderAPITextureHandle_t hModifyTexture = m_ModifyTextureHandle;
	if ( !m_Textures.IsValidIndex( hModifyTexture ) )
	{
		Log( "Invalid modify texture handle!\n" );
		return;

	}

	//if ( zOffset != 0 )
		//DebuggerBreak();

	// Blow off mip levels if we don't support mipmapping
	if ( !g_pHardwareConfig->SupportsMipmapping() && ( level > 0 ) )
	{
		Log( "Trying to image mip but we don't support mips!\n" );
		return;
	}

	CTextureDx11 &tex = GetTexture( hModifyTexture );

	// NOTE: This can only be done with procedural textures if this method is
	// being used to download the entire texture, cause last frame's partial update
	// may be in a completely different texture! Sadly, I don't have all of the
	// information I need, but I can at least check a couple things....
#ifdef _DEBUG
	if ( tex.m_NumCopies > 1 )
	{
		Assert( ( xOffset == 0 ) && ( yOffset == 0 ) );
	}
#endif

	// This test here just makes sure we don't try to download mipmap levels
	// if we weren't able to create them in the first place
	if ( level >= tex.m_NumLevels )
	{
		Log( "level >= tex.m_NumLevels\n" );
		return;
	}

	// May need to switch textures....
	if ( tex.m_SwitchNeeded )
	{
		AdvanceCurrentTextureCopy( hModifyTexture );
		tex.m_SwitchNeeded = false;
	}

	CTextureDx11::TextureLoadInfo_t info;
	info.m_TextureHandle = hModifyTexture;
	info.m_pTexture = GetD3DTexture( hModifyTexture );
	info.m_nLevel = level;
	info.m_nCopy = tex.m_CurrentCopy;
	info.m_CubeFaceID = (D3D11_TEXTURECUBE_FACE)cubeFace;
	info.m_nWidth = width;
	info.m_nHeight = height;
	info.m_nZOffset = zOffset;
	info.m_SrcFormat = srcFormat;
	info.m_pSrcData = (unsigned char *)imageData;
	tex.LoadTexImage( info, xOffset, yOffset, srcStride );
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
	CTextureDx11 **arrTxp = new CTextureDx11 *[count];

	for ( int idxFrame = 0; idxFrame < count; ++idxFrame )
	{
		arrTxp[idxFrame]	  = &GetTexture( pHandles[idxFrame] );
		CTextureDx11 *pTexture	  = arrTxp[idxFrame];
		pTexture->SetupTexture2D( width, height, depth, count, idxFrame, flags,
					  numCopies, numMipLevels, dstImageFormat );

	}

	delete[] arrTxp;
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

//-----------------------------------------------------------------------------
// Selection mode methods
//-----------------------------------------------------------------------------
int CShaderAPIDx11::SelectionMode( bool selectionMode )
{
	LOCK_SHADERAPI();
	int numHits = m_NumHits;
	if ( m_InSelectionMode )
	{
		WriteHitRecord();
	}
	m_InSelectionMode = selectionMode;
	m_pCurrSelectionRecord = m_pSelectionBuffer;
	m_NumHits = 0;
	return numHits;
}

bool CShaderAPIDx11::IsInSelectionMode() const
{
	return m_InSelectionMode;
}

void CShaderAPIDx11::SelectionBuffer( unsigned int *pBuffer, int size )
{
	LOCK_SHADERAPI();
	Assert( !m_InSelectionMode );
	Assert( pBuffer && size );
	m_pSelectionBufferEnd = pBuffer + size;
	m_pSelectionBuffer = pBuffer;
	m_pCurrSelectionRecord = pBuffer;
}

void CShaderAPIDx11::ClearSelectionNames()
{
	LOCK_SHADERAPI();
	if ( m_InSelectionMode )
	{
		WriteHitRecord();
	}
	m_SelectionNames.Clear();
}

void CShaderAPIDx11::LoadSelectionName( int name )
{
	LOCK_SHADERAPI();
	if ( m_InSelectionMode )
	{
		WriteHitRecord();
		Assert( m_SelectionNames.Count() > 0 );
		m_SelectionNames.Top() = name;
	}
}

void CShaderAPIDx11::PushSelectionName( int name )
{
	LOCK_SHADERAPI();
	if ( m_InSelectionMode )
	{
		WriteHitRecord();
		m_SelectionNames.Push( name );
	}
}

void CShaderAPIDx11::PopSelectionName()
{
	LOCK_SHADERAPI();
	if ( m_InSelectionMode )
	{
		WriteHitRecord();
		m_SelectionNames.Pop();
	}
}

void CShaderAPIDx11::WriteHitRecord()
{
	FlushBufferedPrimitives();

	if ( m_SelectionNames.Count() && ( m_SelectionMinZ != FLT_MAX ) )
	{
		Assert( m_pCurrSelectionRecord + m_SelectionNames.Count() + 3 < m_pSelectionBufferEnd );
		*m_pCurrSelectionRecord++ = m_SelectionNames.Count();
		*m_pCurrSelectionRecord++ = (int)( (double)m_SelectionMinZ * (double)0xFFFFFFFF );
		*m_pCurrSelectionRecord++ = (int)( (double)m_SelectionMaxZ * (double)0xFFFFFFFF );
		for ( int i = 0; i < m_SelectionNames.Count(); ++i )
		{
			*m_pCurrSelectionRecord++ = m_SelectionNames[i];
		}

		++m_NumHits;
	}

	m_SelectionMinZ = FLT_MAX;
	m_SelectionMaxZ = FLT_MIN;
}

// We hit somefin in selection mode
void CShaderAPIDx11::RegisterSelectionHit( float minz, float maxz )
{
	if ( minz < 0 )
		minz = 0;
	if ( maxz > 1 )
		maxz = 1;
	if ( m_SelectionMinZ > minz )
		m_SelectionMinZ = minz;
	if ( m_SelectionMaxZ < maxz )
		m_SelectionMaxZ = maxz;
}


// Use this to get the mesh builder that allows us to modify vertex data
CMeshBuilder *CShaderAPIDx11::GetVertexModifyBuilder()
{
	return &m_ModifyBuilder;
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
	LOCK_SHADERAPI();
	MeshMgr()->DestroyVertexBuffers();
	// After a map is shut down, we switch to using smaller dynamic VBs
	// (VGUI shouldn't need much), so that we have more memory free during map loading
	m_nDynamicVBSize = bExitingLevel ? DYNAMIC_VERTEX_BUFFER_MEMORY_SMALL : DYNAMIC_VERTEX_BUFFER_MEMORY;
}

int CShaderAPIDx11::GetCurrentDynamicVBSize( void )
{
	return m_nDynamicVBSize;
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

void CShaderAPIDx11::SyncToken( const char *pToken )
{
}

// Stencils

void CShaderAPIDx11::SetStencilEnable( bool onoff )
{
	m_TargetState.shadow.depthStencil.StencilEnable = onoff;
}

void CShaderAPIDx11::SetStencilFailOperation( StencilOperation_t op )
{
	m_TargetState.shadow.depthStencil.FrontFace.StencilFailOp = (D3D11_STENCIL_OP)op;
}

void CShaderAPIDx11::SetStencilZFailOperation( StencilOperation_t op )
{
	m_TargetState.shadow.depthStencil.FrontFace.StencilDepthFailOp = (D3D11_STENCIL_OP)op;
}

void CShaderAPIDx11::SetStencilPassOperation( StencilOperation_t op )
{
	m_TargetState.shadow.depthStencil.FrontFace.StencilPassOp = (D3D11_STENCIL_OP)op;
}

void CShaderAPIDx11::SetStencilCompareFunction( StencilComparisonFunction_t cmpfn )
{
	m_TargetState.shadow.depthStencil.FrontFace.StencilFunc = (D3D11_COMPARISON_FUNC)cmpfn;
}

void CShaderAPIDx11::SetStencilReferenceValue( int ref )
{
	m_TargetState.shadow.depthStencil.StencilRef = ref;
}

void CShaderAPIDx11::SetStencilTestMask( uint32 msk )
{
	m_TargetState.shadow.depthStencil.StencilReadMask = msk;
}

void CShaderAPIDx11::SetStencilWriteMask( uint32 msk )
{
	m_TargetState.shadow.depthStencil.StencilWriteMask = msk;
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

bool CShaderAPIDx11::SetMode( void *hwnd, int nAdapter, const ShaderDeviceInfo_t &mode )
{
	return g_pShaderDeviceMgr->SetMode( hwnd, nAdapter, mode ) != NULL;
}


//--------------------------------------------------------------------
// Occlusion queries
//--------------------------------------------------------------------

ShaderAPIOcclusionQuery_t CShaderAPIDx11::CreateOcclusionQueryObject( void )
{
	return g_pShaderDeviceDx11->CreateOcclusionQuery();
}

void CShaderAPIDx11::DestroyOcclusionQueryObject( ShaderAPIOcclusionQuery_t handle )
{
	g_pShaderDeviceDx11->DestroyOcclusionQuery( handle );
}

void CShaderAPIDx11::BeginOcclusionQueryDrawing( ShaderAPIOcclusionQuery_t handle )
{
	if ( handle != INVALID_SHADERAPI_OCCLUSION_QUERY_HANDLE )
	{
		D3D11DeviceContext()->Begin( (ID3D11Query *)handle );
	}
}

void CShaderAPIDx11::EndOcclusionQueryDrawing( ShaderAPIOcclusionQuery_t handle )
{
	if ( handle != INVALID_SHADERAPI_OCCLUSION_QUERY_HANDLE )
	{
		D3D11DeviceContext()->End( (ID3D11Query *)handle );
	}
}

int CShaderAPIDx11::OcclusionQuery_GetNumPixelsRendered( ShaderAPIOcclusionQuery_t handle, bool bFlush )
{
	if ( handle == INVALID_SHADERAPI_OCCLUSION_QUERY_HANDLE )
	{
		return OCCLUSION_QUERY_RESULT_ERROR;
	}

	uint nPixels;
	HRESULT hr = D3D11DeviceContext()->GetData( (ID3D11Query *)handle, &nPixels, sizeof( nPixels ),
						    bFlush ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH );
	if ( FAILED( hr ) )
	{
		return OCCLUSION_QUERY_RESULT_ERROR;
	}

	if ( hr == S_FALSE ) // not ready yet
	{
		return OCCLUSION_QUERY_RESULT_PENDING;
	}

	return (int)nPixels;
}


//------------------------------------------------------------------------------------
// UNUSED/UNSUPPORTED FUNCTIONS!!!
//------------------------------------------------------------------------------------

// Lightmap texture binding
void CShaderAPIDx11::BindLightmap( TextureStage_t stage )
{
	// Unused
}

void CShaderAPIDx11::BindBumpLightmap( TextureStage_t stage )
{
	// Unused
}

void CShaderAPIDx11::BindFullbrightLightmap( TextureStage_t stage )
{
	// Unused
}

void CShaderAPIDx11::BindWhite( TextureStage_t stage )
{
	// Unused
}

void CShaderAPIDx11::BindBlack( TextureStage_t stage )
{
	// Unused
}

void CShaderAPIDx11::BindGrey( TextureStage_t stage )
{
	// Unused
}

void CShaderAPIDx11::SetTextureTransformDimension( TextureStage_t textureStage, int dimension, bool projected )
{
	Warning( "Unsupported CShaderAPIDx11::SetTextureTransformDimension() called!\n" );
}

void CShaderAPIDx11::SetBumpEnvMatrix( TextureStage_t textureStage, float m00, float m01, float m10, float m11 )
{
	Warning( "Unsupported CShaderAPIDx11::SetBumpEnvMatrix() called!\n" );
}

void CShaderAPIDx11::SetAmbientLight( float r, float g, float b )
{
	Warning( "Unsupported CShaderAPIDx11::SetAmbientLight() called!\n" );
}

//-----------------------------------------------------------------------------
// Methods related to state objects
//-----------------------------------------------------------------------------
void CShaderAPIDx11::SetRasterState( const ShaderRasterState_t &state )
{
	Warning( "Unsupported CShaderAPIDx11::SetRasterState() called!\n" );
}

// The shade mode
void CShaderAPIDx11::ShadeMode( ShaderShadeMode_t mode )
{
	Warning( "Unsupported CShaderAPIDx11::ShadeMode() called!\n" );
}

void CShaderAPIDx11::TexSetPriority( int priority )
{
	Warning( "Unsupported CShaderAPIDx11::SetTexPriority() called!\n" );
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

// Special system flat normal map binding.
void CShaderAPIDx11::BindFlatNormalMap( TextureStage_t stage )
{
	// Unused
}

void CShaderAPIDx11::BindNormalizationCubeMap( TextureStage_t stage )
{
	// Unused
}

void CShaderAPIDx11::BindSignedNormalizationCubeMap( TextureStage_t stage )
{
	// Unused
}

void CShaderAPIDx11::BindFBTexture( TextureStage_t stage, int textureIndex )
{
	// Unused
}

// Render state for the ambient light cube (vertex shaders)
void CShaderAPIDx11::SetVertexShaderStateAmbientLightCube()
{
	Warning( "Unsupported CShaderAPIDx11::SetVertexShaderStateAmbientLightCube() called!\n" );
}