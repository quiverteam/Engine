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
#include "shaderapi/commandbuffer.h"
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
#include "materialsystem/IShader.h"
#include "../stdshaders/cpp_shader_constant_register_map.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

//-------------------------------------------------------------------
// Common constant buffers, grouped by frequency of update.
// NOTE: These need to match the cbuffers in common_cbuffers_fxc.h!!!
//-------------------------------------------------------------------

// In order of most frequent to least frequent...

// Constants that can be expected to change for each material.
ALIGN16 struct PerMaterial_CBuffer_t
{
	DirectX::XMFLOAT4 cShadowTweaks;
	DirectX::XMFLOAT4 cLightScale;
	DirectX::XMFLOAT4 cConstants1;
	DirectX::XMFLOAT4 cModulationColor;
	DirectX::XMFLOAT4 cAlphaTestRef;
};

ALIGN16 struct DX11LightInfo_t
{
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT4 dir;
	DirectX::XMFLOAT4 pos;
	DirectX::XMFLOAT4 spotParams;
	DirectX::XMFLOAT4 atten;
};

// Constants that can be expected to change for each model.
ALIGN16 struct PerModel_CBuffer_t
{
	DirectX::XMMATRIX cModelViewProj;
	DirectX::XMMATRIX cViewModel;
	DirectX::XMMATRIX cModel[53];
	DirectX::XMFLOAT4 cFlexWeights[512];
	// Only cFlexScale.x is used
	// It is a binary value used to switch on/off the addition of the flex delta stream
	DirectX::XMFLOAT4 cFlexScale;
	Vector4D cLightEnabled;
	DirectX::XMFLOAT4 cLightCountRegister;
	DirectX::XMFLOAT4 cAmbientCube[6];
	// Four lights x 5 constants each = 20 constants
	DX11LightInfo_t cLightInfo[4];
};

int x = sizeof( PerModel_CBuffer_t );

// Constants that can be expected to change each frame.
ALIGN16 struct PerFrame_CBuffer_t
{
	DirectX::XMMATRIX cViewProj;
	DirectX::XMFLOAT4 cEyePos;
	DirectX::XMFLOAT4 cFlashlightPos;
};

// Constants that don't change per-material, per-model, or per-frame.
// These are expected to be changed whenever and apply to all materials.
ALIGN16 struct PerScene_CBuffer_t
{
	DirectX::XMMATRIX cFlashlightWorldToTexture;
	DirectX::XMFLOAT4 cFlashlightScreenScale;
	DirectX::XMFLOAT4 cFlashlightColor;
	DirectX::XMFLOAT4 cFlashlightAttenuationFactors;
	DirectX::XMFLOAT4 cLinearFogColor;
	DirectX::XMFLOAT4 cFogParams;
	DirectX::XMFLOAT4 cFogColor;
	float cFogZ;
};

static PerFrame_CBuffer_t s_PerFrameConstants;
static PerModel_CBuffer_t s_PerModelConstants;
static PerScene_CBuffer_t s_PerSceneConstants;
static PerMaterial_CBuffer_t s_PerMaterialConstants;

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
	m_ModifyTextureLockedFace = -1;
	m_bResettingRenderState = false;
	m_TargetState = StatesDx11::RenderState();
	m_State = m_TargetState;
	m_TargetShaderState = StatesDx11::ShaderState();
	m_ShaderState = m_TargetShaderState;
	m_bSelectionMode = false;
	m_bFlashlightStateChanged = false;
	m_pFlashlightDepthTexture = NULL;

	memset( IntRenderingParameters, 0, sizeof( IntRenderingParameters ) );
	memset( FloatRenderingParameters, 0, sizeof( FloatRenderingParameters ) );
	memset( VectorRenderingParameters, 0, sizeof( VectorRenderingParameters ) );
}

CShaderAPIDx11::~CShaderAPIDx11()
{
}

void CShaderAPIDx11::UpdateConstantBuffer( ConstantBufferHandle_t cbuffer, void *pNewData )
{
	g_pShaderDeviceDx11->UpdateConstantBuffer( cbuffer, pNewData );
}

ConstantBufferHandle_t CShaderAPIDx11::GetInternalConstantBuffer( int type )
{
	switch ( type )
	{
	case SHADER_CONSTANTBUFFER_PERFRAME:
		return m_hPerFrameConstants;
	case SHADER_CONSTANTBUFFER_PERMODEL:
		return m_hPerModelConstants;
	case SHADER_CONSTANTBUFFER_PERMATERIAL:
		return m_hPerMaterialConstants;
	case SHADER_CONSTANTBUFFER_PERSCENE:
		return m_hPerSceneConstants;
	default:
		return CONSTANT_BUFFER_HANDLE_INVALID;
	}
}

void CShaderAPIDx11::BindPixelShaderConstantBuffer( int slot, ConstantBufferHandle_t cbuffer )
{
	StatesDx11::ConstantBufferSlot_t def;
	def.m_pBuffer = (CShaderConstantBufferDx11 *)cbuffer;
	def.slot = slot;

	m_TargetState.dynamic.m_pPSConstantBuffers
		[m_TargetState.dynamic.m_nPSConstantBuffers++] = def;
}

void CShaderAPIDx11::BindVertexShaderConstantBuffer( int slot, ConstantBufferHandle_t cbuffer )
{
	StatesDx11::ConstantBufferSlot_t def;
	def.m_pBuffer = (CShaderConstantBufferDx11 *)cbuffer;
	def.slot = slot;

	m_TargetState.dynamic.m_pVSConstantBuffers
		[m_TargetState.dynamic.m_nVSConstantBuffers++] = def;
}

void CShaderAPIDx11::BindGeometryShaderConstantBuffer( int slot, ConstantBufferHandle_t cbuffer )
{
	StatesDx11::ConstantBufferSlot_t def;
	def.m_pBuffer = (CShaderConstantBufferDx11 *)cbuffer;
	def.slot = slot;

	m_TargetState.dynamic.m_pGSConstantBuffers
		[m_TargetState.dynamic.m_nGSConstantBuffers++] = def;
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

	m_TargetShaderState.SetDefault();
	m_pCurMatrixItem = &m_TargetShaderState.m_MatrixStacks[0].Top();

	IssueStateChanges( bFullReset );
}

void CShaderAPIDx11::SetRenderTargetEx( int id, ShaderAPITextureHandle_t colorTextureHandle,
					ShaderAPITextureHandle_t depthTextureHandle )
{
	// GR - need to flush batched geometry
	FlushBufferedPrimitives();

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

	//FlushBufferedPrimitives();

	//float flStart = Plat_FloatTime();

	////Log( "ShaderAPIDx11: Issuing state changes\n" );

	const StatesDx11::DynamicState &targetDynamic = m_TargetState.dynamic;
	const StatesDx11::ShadowState &targetShadow = m_TargetState.shadow;
	
	const StatesDx11::DynamicState &dynamic = m_State.dynamic;
	const StatesDx11::ShadowState &shadow = m_State.shadow;

	DoIssueShaderState( bForce );
	
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

	bool bPixelSamplers = bForce ||
		targetDynamic.m_nSamplers != dynamic.m_nSamplers ||
		memcmp( targetDynamic.m_pSamplers,
			dynamic.m_pSamplers,
			sizeof( StatesDx11::TextureSlot_t ) * MAX_DX11_SAMPLERS );
	bool bVertexSamplers = bForce || 
		targetDynamic.m_nVSSamplers != dynamic.m_nVSSamplers ||
		memcmp( targetDynamic.m_pVSSamplers,
			dynamic.m_pVSSamplers,
			sizeof( StatesDx11::TextureSlot_t ) * MAX_DX11_SAMPLERS );
	if ( bPixelSamplers || bVertexSamplers )
	{
		//Log( "\tIssuing textures\n" );
		DoIssueSampler( bPixelSamplers, bVertexSamplers );
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

	//float flEnd = Plat_FloatTime();

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

void CShaderAPIDx11::SortLights( int *index )
{
	m_TargetShaderState.light.m_NumLights = 0;
	for ( int i = 0; i < MAX_NUM_LIGHTS; i++ )
	{
		const LightDesc_t &light = m_TargetShaderState.light.m_Lights[i];
		LightType_t type = light.m_Type;
		int j = m_TargetShaderState.light.m_NumLights;
		if ( type != MATERIAL_LIGHT_DISABLE )
		{
			while ( --j >= 0 )
			{
				if ( m_TargetShaderState.light.m_Lights[j].m_Type <= type )
					break;

				// shift...
				m_TargetShaderState.light.m_Lights[j + 1] = m_TargetShaderState.light.m_Lights[j];
				index[j + 1] = index[j];
			}
			++j;

			m_TargetShaderState.light.m_Lights[j] = light;
			index[j] = i;
			++m_TargetShaderState.light.m_NumLights;
		}
	}
}

FORCEINLINE static float ShadowAttenFromState( FlashlightState_t const &state )
{
	// DX10 requires some hackery due to sRGB/blend ordering change from DX9, which makes the shadows too light
	if ( g_pHardwareConfig->UsesSRGBCorrectBlending() )
		return state.m_flShadowAtten * 0.1f; // magic number

	return state.m_flShadowAtten;
}

FORCEINLINE static float ShadowFilterFromState( FlashlightState_t const &state )
{
	return state.m_flShadowFilterSize / state.m_flShadowMapResolution;
}

FORCEINLINE static void HashShadow2DJitter( const float fJitterSeed, float *fU, float *fV )
{
	const int nTexRes = 32;
	int nSeed = fmod( fJitterSeed, 1.0f ) * nTexRes * nTexRes;

	int nRow = nSeed / nTexRes;
	int nCol = nSeed % nTexRes;

	// Div and mod to get an individual texel in the fTexRes x fTexRes grid
	*fU = nRow / (float)nTexRes;	// Row
	*fV = nCol / (float)nTexRes;	// Column
}

void CShaderAPIDx11::DoIssueShaderState( bool bForce )
{
	//
	// Transforms
	//

	bool bViewChanged = m_TargetShaderState.m_ChangedMatrices[MATERIAL_VIEW];
	bool bProjChanged = m_TargetShaderState.m_ChangedMatrices[MATERIAL_PROJECTION];
	bool bModelChanged = m_TargetShaderState.m_ChangedMatrices[MATERIAL_MODEL];

	bool bChanged = false;

	if ( bForce || bViewChanged || bProjChanged || bModelChanged )
	{
		const DirectX::XMMATRIX model = GetMatrix( MATERIAL_MODEL );
		const DirectX::XMMATRIX view = GetMatrix( MATERIAL_VIEW );
		const DirectX::XMMATRIX projection = GetMatrix( MATERIAL_PROJECTION );

		if ( bForce || bViewChanged || bProjChanged )
		{
			// Store new ViewProjection matrix.
			DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply( view, projection );
			viewProj = DirectX::XMMatrixTranspose( viewProj );
			s_PerFrameConstants.cViewProj = viewProj;
		}
		
		// Any of the matrices being changed warrants a new ModelViewProjection.
		DirectX::XMMATRIX viewModel = DirectX::XMMatrixMultiply( model, view );
		DirectX::XMMATRIX modelViewProj = DirectX::XMMatrixMultiply( viewModel, projection );
		modelViewProj = DirectX::XMMatrixTranspose( modelViewProj );
		s_PerModelConstants.cModelViewProj = modelViewProj;
		
		if ( bForce || bViewChanged || bModelChanged )
		{
			// Store new ViewModel matrix.
			viewModel = DirectX::XMMatrixTranspose( viewModel );
			s_PerModelConstants.cViewModel = viewModel;
		}

		if ( bForce || bViewChanged )
		{
			// Store new view position.
			DirectX::XMVECTOR scale, rot, viewTranslation;
			DirectX::XMMatrixDecompose( &scale, &rot, &viewTranslation, view );
			DirectX::XMStoreFloat4( &s_PerFrameConstants.cEyePos, viewTranslation );
		}

		m_TargetShaderState.m_ChangedMatrices[MATERIAL_MODEL] = false;
		m_TargetShaderState.m_ChangedMatrices[MATERIAL_VIEW] = false;
		m_TargetShaderState.m_ChangedMatrices[MATERIAL_PROJECTION] = false;

		bChanged = true;
	}

	//
	// Lighting
	//

	if ( bForce || m_ShaderState.light.IsLightChanged( m_TargetShaderState.light ) )
	{
		int lightIndex[MAX_NUM_LIGHTS];
		memset( lightIndex, 0, sizeof( lightIndex ) );
		SortLights( lightIndex );

		s_PerModelConstants.cLightCountRegister.x = m_TargetShaderState.light.m_NumLights;

		for ( int i = 0; i < m_TargetShaderState.light.m_NumLights; i++ )
		{
			const LightDesc_t &light = m_TargetShaderState.light.m_Lights[i];
			s_PerModelConstants.cLightEnabled[i] = light.m_Type != MATERIAL_LIGHT_DISABLE;

			// The first one is the light color ( and light type code )
			float w = ( light.m_Type == MATERIAL_LIGHT_DIRECTIONAL ) ? 1.0f : 0.0f;
			s_PerModelConstants.cLightInfo[i].color =
				DirectX::XMFLOAT4( light.m_Color.x, light.m_Color.y, light.m_Color.z, w );

			// The next constant holds the light direction ( and light type code )
			w = ( light.m_Type == MATERIAL_LIGHT_SPOT ) ? 1.0f : 0.0f;
			s_PerModelConstants.cLightInfo[i].dir =
				DirectX::XMFLOAT4( light.m_Direction.x, light.m_Direction.y, light.m_Direction.z, w );

			// The next constant holds the light position
			s_PerModelConstants.cLightInfo[i].pos =
				DirectX::XMFLOAT4( light.m_Position.x, light.m_Position.y, light.m_Position.z, 1.0f );

			// The next constant holds exponent, stopdot, stopdot2, 1 / (stopdot - stopdot2)
			if ( light.m_Type == MATERIAL_LIGHT_SPOT )
			{
				float stopdot = cos( light.m_Theta * 0.5f );
				float stopdot2 = cos( light.m_Phi * 0.5f );
				float oodot = ( stopdot > stopdot2 ) ? 1.0f / ( stopdot - stopdot2 ) : 0.0f;
				s_PerModelConstants.cLightInfo[i].spotParams =
					DirectX::XMFLOAT4( light.m_Falloff, stopdot, stopdot2, oodot );
			}
			else
			{
				s_PerModelConstants.cLightInfo[i].spotParams =
					DirectX::XMFLOAT4( 0, 1, 1, 1 );
			}

			// The last constant holds atten0, atten1, atten2
			s_PerModelConstants.cLightInfo[i].atten =
				DirectX::XMFLOAT4( light.m_Attenuation0, light.m_Attenuation1, light.m_Attenuation2, 0.0f );
		}
		bChanged = true;
	}

	if ( bForce || m_ShaderState.light.IsAmbientChanged( m_TargetShaderState.light ) )
	{
		memcpy( s_PerModelConstants.cAmbientCube, m_TargetShaderState.light.m_AmbientLightCube, sizeof( DirectX::XMFLOAT4 ) * 6 );
		bChanged = true;
	}

	//
	// Fog
	//

	if ( bForce || m_ShaderState.fog.IsChanged( m_TargetShaderState.fog ) )
	{
		float ooFogRange = 1.0f;
		float fStart = m_TargetShaderState.fog.m_flFogStart;
		float fEnd = m_TargetShaderState.fog.m_flFogEnd;
		// Check for divide by zero
		if ( fStart != fEnd )
		{
			ooFogRange = 1.0f / ( fEnd - fStart );
		}

		s_PerSceneConstants.cFogParams.x = ooFogRange * fEnd;
		s_PerSceneConstants.cFogParams.y = 1.0f;
		s_PerSceneConstants.cFogParams.z = 1.0f - clamp( m_TargetShaderState.fog.m_flFogMaxDensity, 0.0f, 1.0f );
		s_PerSceneConstants.cFogParams.w = ooFogRange;
		s_PerSceneConstants.cFogZ = m_TargetShaderState.fog.m_flFogZ;
		s_PerSceneConstants.cFogColor.x = m_TargetShaderState.fog.m_FogColor[0];
		s_PerSceneConstants.cFogColor.y = m_TargetShaderState.fog.m_FogColor[1];
		s_PerSceneConstants.cFogColor.z = m_TargetShaderState.fog.m_FogColor[2];
		s_PerSceneConstants.cFogColor.w = 1.0f;
		bChanged = true;
	}

	//
	// Skinning
	//

	if ( true )//bForce || m_ShaderState.bone.IsChanged( m_TargetShaderState.bone ) )
	{
		// Load the model matrix from the matrix stack into the
		// first bone matrix.
		DirectX::XMMATRIX model = GetMatrix( MATERIAL_MODEL );
		// Model matrix is row major, but bone matrices are
		// stored column major.
		model = DirectX::XMMatrixTranspose( model );
		m_TargetShaderState.bone.m_BoneMatrix[0] = model;
		m_TargetShaderState.bone.m_MaxBoneLoaded++;
		int matricesLoaded = max( 1, m_TargetShaderState.bone.m_MaxBoneLoaded );
		m_TargetShaderState.bone.m_MaxBoneLoaded = 0;

		// Copy bone matrices into cModel constant
		memcpy( s_PerModelConstants.cModel, m_TargetShaderState.bone.m_BoneMatrix,
			sizeof( DirectX::XMMATRIX ) * matricesLoaded );

		bChanged = true;
	}
	

	//
	// Material constants
	//
	s_PerMaterialConstants.cModulationColor = DirectX::XMFLOAT4( m_TargetShaderState.m_ConstantColor[0],
								     m_TargetShaderState.m_ConstantColor[1],
								     m_TargetShaderState.m_ConstantColor[2],
								     m_TargetShaderState.m_ConstantColor[3] );

	//
	// Flashlight
	//
	if ( m_bFlashlightStateChanged )
	{
		float flFlashlightScale = 0.25f;
		if ( !g_pHardwareConfig->GetHDREnabled() )
		{
			// Non-HDR path requires 2.0 flashlight
			flFlashlightScale = 2.0f;
		}
		// DX10 requires some hackery due to sRGB/blend ordering change from DX9
		if ( g_pHardwareConfig->UsesSRGBCorrectBlending() )
		{
			flFlashlightScale *= 2.5f; // Magic number that works well on the NVIDIA 8800
		}
		float const *pFlashlightColor = m_FlashlightState.m_Color;
		float vPsConst[4] = { flFlashlightScale * pFlashlightColor[0], flFlashlightScale * pFlashlightColor[1],
			flFlashlightScale * pFlashlightColor[2], pFlashlightColor[3] };
		vPsConst[3] = 0.0f; // This will be added to N.L before saturate to force a 1.0 N.L term
		s_PerSceneConstants.cFlashlightColor = DirectX::XMFLOAT4( vPsConst );
		DirectX::XMFLOAT4X4 flashlightWorldToTexture4x4 = DirectX::XMFLOAT4X4( m_FlashlightWorldToTexture.Base() );
		s_PerSceneConstants.cFlashlightWorldToTexture = DirectX::XMLoadFloat4x4( &flashlightWorldToTexture4x4 );
		// Dimensions of screen, used for screen-space noise map sampling
		float vScreenScale[4] = { 1280.0f / 32.0f, 720.0f / 32.0f, 0, 0 };
		int nWidth, nHeight;
		GetBackBufferDimensions( nWidth, nHeight );
		vScreenScale[0] = (float)nWidth / 32.0f;
		vScreenScale[1] = (float)nHeight / 32.0f;
		s_PerSceneConstants.cFlashlightScreenScale = DirectX::XMFLOAT4( vScreenScale );
		// Tweaks associated with a given flashlight
		float tweaks[4];
		tweaks[0] = m_FlashlightState.m_flShadowFilterSize / m_FlashlightState.m_flShadowMapResolution;
		tweaks[1] = ShadowAttenFromState( m_FlashlightState );
		HashShadow2DJitter( m_FlashlightState.m_flShadowJitterSeed, &tweaks[2], &tweaks[3] );
		s_PerMaterialConstants.cShadowTweaks = DirectX::XMFLOAT4( tweaks );
		s_PerFrameConstants.cFlashlightPos = DirectX::XMFLOAT4( m_FlashlightState.m_vecLightOrigin[0],
									m_FlashlightState.m_vecLightOrigin[1],
									m_FlashlightState.m_vecLightOrigin[2],
									1.0f );
		s_PerSceneConstants.cFlashlightAttenuationFactors = DirectX::XMFLOAT4(
			m_FlashlightState.m_fConstantAtten,
			m_FlashlightState.m_fLinearAtten,
			m_FlashlightState.m_fQuadraticAtten,
			m_FlashlightState.m_FarZ
		);

		m_bFlashlightStateChanged = false;
	}

	//
	// Supply the new constants.
	//
	( (IShaderConstantBuffer *)m_hPerMaterialConstants )->Update( &s_PerMaterialConstants );
	( (IShaderConstantBuffer *)m_hPerModelConstants )->Update( &s_PerModelConstants );
	( (IShaderConstantBuffer *)m_hPerFrameConstants )->Update( &s_PerFrameConstants );
	( (IShaderConstantBuffer *)m_hPerSceneConstants )->Update( &s_PerSceneConstants );

	if ( bChanged )
	{
		m_ShaderState = m_TargetShaderState;
	}
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
		UploadConstantBuffer( m_TargetState.dynamic.m_pVSConstantBuffers[i].m_pBuffer );
	}
	for ( int i = 0; i < m_TargetState.dynamic.m_nGSConstantBuffers; i++ )
	{
		UploadConstantBuffer( m_TargetState.dynamic.m_pGSConstantBuffers[i].m_pBuffer );
	}
	for ( int i = 0; i < m_TargetState.dynamic.m_nPSConstantBuffers; i++ )
	{
		UploadConstantBuffer( m_TargetState.dynamic.m_pPSConstantBuffers[i].m_pBuffer );
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
		  memcmp( targetDynamic.m_pVSConstantBuffers, dynamic.m_pVSConstantBuffers,
			  sizeof( StatesDx11::ConstantBufferSlot_t ) * MAX_DX11_CBUFFERS ) ) )
	{
		for ( int i = 0; i < targetDynamic.m_nVSConstantBuffers; i++ )
		{
			const StatesDx11::ConstantBufferSlot_t &slotdef = targetDynamic.m_pVSConstantBuffers[i];
			ID3D11Buffer *pBuf = slotdef.m_pBuffer->GetD3DBuffer();
			D3D11DeviceContext()->VSSetConstantBuffers( slotdef.slot, 1, &pBuf );
		}
		
		bVSChanged = true;
	}

	if ( m_TargetState.dynamic.m_pGeometryShader &&
		( targetDynamic.m_nGSConstantBuffers != dynamic.m_nGSConstantBuffers ||
		  memcmp( targetDynamic.m_pGSConstantBuffers, dynamic.m_pGSConstantBuffers,
			  sizeof( StatesDx11::ConstantBufferSlot_t ) * MAX_DX11_CBUFFERS ) ) )
	{
		for ( int i = 0; i < targetDynamic.m_nGSConstantBuffers; i++ )
		{
			const StatesDx11::ConstantBufferSlot_t &slotdef = targetDynamic.m_pGSConstantBuffers[i];
			ID3D11Buffer *pBuf = slotdef.m_pBuffer->GetD3DBuffer();
			D3D11DeviceContext()->GSSetConstantBuffers( slotdef.slot, 1, &pBuf );
		}

		bGSChanged = true;
	}

	if ( m_TargetState.dynamic.m_pPixelShader &&
		( targetDynamic.m_nPSConstantBuffers != dynamic.m_nPSConstantBuffers ||
		  memcmp( targetDynamic.m_pPSConstantBuffers, dynamic.m_pPSConstantBuffers,
			  sizeof( StatesDx11::ConstantBufferSlot_t ) * MAX_DX11_CBUFFERS ) ) )
	{
		for ( int i = 0; i < targetDynamic.m_nPSConstantBuffers; i++ )
		{
			const StatesDx11::ConstantBufferSlot_t &slotdef = targetDynamic.m_pPSConstantBuffers[i];
			ID3D11Buffer *pBuf = slotdef.m_pBuffer->GetD3DBuffer();
			D3D11DeviceContext()->PSSetConstantBuffers( slotdef.slot, 1, &pBuf );
		}

		bGSChanged = true;
	}

	return bPSChanged || bVSChanged || bGSChanged;
}

void CShaderAPIDx11::DoIssueSampler( bool bPixel, bool bVertex )
{
	if ( bPixel )
	{
		for ( int i = 0; i < m_TargetState.dynamic.m_nSamplers; i++ )
		{
			const StatesDx11::TextureSlot_t &slotdef = m_TargetState.dynamic.m_pSamplers[i];
			if ( !slotdef.m_pTexture )
				continue;
			ID3D11ShaderResourceView *pView = slotdef.m_pTexture->GetView();
			ID3D11SamplerState *pSampler = slotdef.m_pTexture->GetSamplerState();
			D3D11DeviceContext()->PSSetShaderResources( slotdef.slot, 1, &pView );
			D3D11DeviceContext()->PSSetSamplers( slotdef.slot, 1, &pSampler );
		}
	}
	
	if ( bVertex )
	{
		for ( int i = 0; i < m_TargetState.dynamic.m_nVSSamplers; i++ )
		{
			const StatesDx11::TextureSlot_t &slotdef = m_TargetState.dynamic.m_pVSSamplers[i];
			if ( !slotdef.m_pTexture )
				continue;
			ID3D11ShaderResourceView *pView = slotdef.m_pTexture->GetView();
			ID3D11SamplerState *pSampler = slotdef.m_pTexture->GetSamplerState();
			D3D11DeviceContext()->VSSetShaderResources( slotdef.slot, 1, &pView );
			D3D11DeviceContext()->VSSetSamplers( slotdef.slot, 1, &pSampler );
		}
	}
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
	StatesDx11::DynamicState &dynamic = m_TargetState.dynamic;

	ID3D11InputLayout *pInputLayout = g_pShaderDeviceDx11->GetInputLayout(
		dynamic.m_InputLayout.m_hVertexShader,
		dynamic.m_InputLayout.m_pVertexDecl[0], dynamic.m_InputLayout.m_bStaticLit,
		dynamic.m_InputLayout.m_bUsingFlex, dynamic.m_InputLayout.m_bUsingMorph );

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
	//Log( "CShaderAPIDx11::SetViewports\n" );
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

	// State changed... need to flush the dynamic buffer
	FlushBufferedPrimitives();

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

void CShaderAPIDx11::BindVertexBuffer( int nStreamID, IVertexBuffer *pVertexBuffer, int nOffsetInBytes,
				       int nFirstVertex, int nVertexCount, VertexFormat_t fmt, int nRepetitions )
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

void CShaderAPIDx11::SetUsingExtraVertexBuffers( bool bStaticLit, bool bUsingFlex, bool bUsingMorph )
{
	m_TargetState.dynamic.m_InputLayout.m_bStaticLit = bStaticLit;
	m_TargetState.dynamic.m_InputLayout.m_bUsingFlex = bUsingFlex;
	m_TargetState.dynamic.m_InputLayout.m_bUsingMorph = bUsingMorph;
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
	if ( m_State.dynamic.m_pVertexShader == pShader )
	{
		IssueStateChanges();
	}
}

void CShaderAPIDx11::Unbind( GeometryShaderHandle_t hShader )
{
	ID3D11GeometryShader *pShader = g_pShaderDeviceDx11->GetGeometryShader( hShader );
	Assert( pShader );
	if ( m_TargetState.dynamic.m_pGeometryShader == pShader )
	{
		BindGeometryShader( GEOMETRY_SHADER_HANDLE_INVALID );
	}
	if ( m_State.dynamic.m_pGeometryShader == pShader )
	{
		IssueStateChanges();
	}
}

void CShaderAPIDx11::Unbind( PixelShaderHandle_t hShader )
{
	ID3D11PixelShader *pShader = g_pShaderDeviceDx11->GetPixelShader( hShader );
	Assert( pShader );
	if ( m_TargetState.dynamic.m_pPixelShader == pShader )
	{
		BindPixelShader( PIXEL_SHADER_HANDLE_INVALID );
	}
	if ( m_State.dynamic.m_pPixelShader == pShader )
	{
		IssueStateChanges();
	}
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
	for ( int i = 0; i < MAX_DX11_STREAMS; ++i )
	{
		if ( m_State.dynamic.m_pVertexBuffer[i].m_pBuffer == pBuffer )
		{
			IssueStateChanges();
			break;
		}
	}
}

void CShaderAPIDx11::UnbindIndexBuffer( ID3D11Buffer *pBuffer )
{
	Assert( pBuffer );

	if ( m_TargetState.dynamic.m_IndexBuffer.m_pBuffer == pBuffer )
	{
		BindIndexBuffer( NULL, 0 );
	}
	if ( m_State.dynamic.m_IndexBuffer.m_pBuffer == pBuffer )
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

	////Log( "Set topology to %i\n", d3dTopology );
	if ( d3dTopology != m_State.dynamic.m_Topology )
	{
		FlushBufferedPrimitives();
	}
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

	//Log( "CShaderAPIDx11::DrawMesh %p\n", pMesh );

	m_pMesh = static_cast<CMeshBase *>( pMesh );
	if ( !m_pMesh || !m_pMaterial )
	{
		Warning( "Tried to render mesh with NULL mesh or NULL material!\n" );
		return;
	}
	SetUsingExtraVertexBuffers( m_pMesh->HasColorMesh(), m_pMesh->HasFlexMesh(), m_pMaterial->IsUsingVertexID() );
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

	//IssueStateChanges();

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
	//Log( "ShaderAPIDx11: Draw\n" );

	SetTopology( primitiveType );

	IssueStateChanges();

	// FIXME: How do I set the base vertex location!?
	DrawIndexed( nFirstIndex, nIndexCount, 0 );
}

void CShaderAPIDx11::DrawIndexed( int nFirstIndex, int nIndexCount, int nBaseVertexLocation )
{
	Assert( m_State.dynamic.m_pVertexShader != NULL );
	D3D11DeviceContext()->DrawIndexed( (UINT)nIndexCount, (UINT)nFirstIndex, (UINT)nBaseVertexLocation );
	//g_pShaderDeviceDx11->Present();
	//Log( "Presented" );
	//Log( "\n" );
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
		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		D3D11SwapChain()->GetDesc( &swapChainDesc );
		HRESULT hr = D3D11SwapChain()->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID *)&pBackBuffer );
		if ( FAILED( hr ) )
			return FALSE;
		m_hBackBuffer = CreateTextureHandle();
		CTextureDx11 *pTex = &GetTexture( m_hBackBuffer );
		pTex->SetupBackBuffer( w, h, "dx11BackBuffer", pBackBuffer, pTex->GetImageFormat( swapChainDesc.BufferDesc.Format ) );
	}

	// Create the depth buffer
	m_hDepthBuffer = CreateDepthTexture( IMAGE_FORMAT_NV_DST24, w, h, "dx11DepthBuffer", true );

	m_hPerMaterialConstants = g_pShaderDeviceDx11->CreateConstantBuffer( sizeof( PerMaterial_CBuffer_t ) );
	m_hPerFrameConstants = g_pShaderDeviceDx11->CreateConstantBuffer( sizeof( PerFrame_CBuffer_t ) );
	m_hPerModelConstants = g_pShaderDeviceDx11->CreateConstantBuffer( sizeof( PerModel_CBuffer_t ) );
	m_hPerSceneConstants = g_pShaderDeviceDx11->CreateConstantBuffer( sizeof( PerScene_CBuffer_t ) );
	

	ResetRenderState( true );

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
	LOCK_SHADERAPI();
	FlushBufferedPrimitives();
	g_pShaderShadowDx11->m_ShadowStateCache.RemoveAll();
	ResetRenderState( true );
}

// Sets the default *dynamic* state
void CShaderAPIDx11::SetDefaultState()
{
	m_TargetState.dynamic.m_nVSConstantBuffers = 0;
	m_TargetState.dynamic.m_nGSConstantBuffers = 0;
	m_TargetState.dynamic.m_nPSConstantBuffers = 0;

	ZeroMemory( m_TargetState.dynamic.m_pVSConstantBuffers, sizeof( StatesDx11::ConstantBufferSlot_t ) * MAX_DX11_CBUFFERS );
	ZeroMemory( m_TargetState.dynamic.m_pGSConstantBuffers, sizeof( StatesDx11::ConstantBufferSlot_t ) * MAX_DX11_CBUFFERS );
	ZeroMemory( m_TargetState.dynamic.m_pPSConstantBuffers, sizeof( StatesDx11::ConstantBufferSlot_t ) * MAX_DX11_CBUFFERS );

	m_TargetState.dynamic.m_nSamplers = 0;
	ZeroMemory( m_TargetState.dynamic.m_pSamplers, sizeof( StatesDx11::TextureSlot_t ) * MAX_DX11_SAMPLERS );

	m_TargetState.dynamic.m_pVertexShader = 0;
	m_TargetState.dynamic.m_pGeometryShader = 0;
	m_TargetState.dynamic.m_pPixelShader = 0;
	m_TargetState.dynamic.m_iVertexShader = -1;
	m_TargetState.dynamic.m_iGeometryShader = -1;
	m_TargetState.dynamic.m_iPixelShader = -1;
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
		//Log( "Applying vertex format from snapshot num %i, %i\n", i, pIds[i] );
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
}

// Sets the color to modulate by
void CShaderAPIDx11::Color3f( float r, float g, float b )
{
	m_TargetShaderState.m_ConstantColor[0] = r;
	m_TargetShaderState.m_ConstantColor[1] = g;
	m_TargetShaderState.m_ConstantColor[2] = b;
	m_TargetShaderState.m_ConstantColor[3] = 1.0f;
}

void CShaderAPIDx11::Color3fv( float const *pColor )
{
	m_TargetShaderState.m_ConstantColor[0] = pColor[0];
	m_TargetShaderState.m_ConstantColor[1] = pColor[1];
	m_TargetShaderState.m_ConstantColor[2] = pColor[2];
	m_TargetShaderState.m_ConstantColor[3] = 1.0f;
}

void CShaderAPIDx11::Color4f( float r, float g, float b, float a )
{
	m_TargetShaderState.m_ConstantColor[0] = r;
	m_TargetShaderState.m_ConstantColor[1] = g;
	m_TargetShaderState.m_ConstantColor[2] = b;
	m_TargetShaderState.m_ConstantColor[3] = a;
}

void CShaderAPIDx11::Color4fv( float const *pColor )
{
	m_TargetShaderState.m_ConstantColor[0] = pColor[0];
	m_TargetShaderState.m_ConstantColor[1] = pColor[1];
	m_TargetShaderState.m_ConstantColor[2] = pColor[2];
	m_TargetShaderState.m_ConstantColor[3] = pColor[3];
}

// Faster versions of color
void CShaderAPIDx11::Color3ub( unsigned char r, unsigned char g, unsigned char b )
{
	m_TargetShaderState.m_ConstantColor[0] = r / 255.0f;
	m_TargetShaderState.m_ConstantColor[1] = g / 255.0f;
	m_TargetShaderState.m_ConstantColor[2] = b / 255.0f;
	m_TargetShaderState.m_ConstantColor[3] = 1.0f;
}

void CShaderAPIDx11::Color3ubv( unsigned char const *rgb )
{
	m_TargetShaderState.m_ConstantColor[0] = rgb[0] / 255.0f;
	m_TargetShaderState.m_ConstantColor[1] = rgb[1] / 255.0f;
	m_TargetShaderState.m_ConstantColor[2] = rgb[2] / 255.0f;
	m_TargetShaderState.m_ConstantColor[3] = 1.0f;
}

void CShaderAPIDx11::Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	m_TargetShaderState.m_ConstantColor[0] = r / 255.0f;
	m_TargetShaderState.m_ConstantColor[1] = g / 255.0f;
	m_TargetShaderState.m_ConstantColor[2] = b / 255.0f;
	m_TargetShaderState.m_ConstantColor[3] = a / 255.0f;
}

void CShaderAPIDx11::Color4ubv( unsigned char const *rgba )
{
	m_TargetShaderState.m_ConstantColor[0] = rgba[0] / 255.0f;
	m_TargetShaderState.m_ConstantColor[1] = rgba[1] / 255.0f;
	m_TargetShaderState.m_ConstantColor[2] = rgba[2] / 255.0f;
	m_TargetShaderState.m_ConstantColor[3] = rgba[3] / 255.0f;
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
	if ( m_State.shadow.rasterizer.CullMode != d3dCull )
	{
		FlushBufferedPrimitives();
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

	m_TargetShaderState.light.m_Lights[lightNum] = desc;
	
}

void CShaderAPIDx11::SetAmbientLightCube( Vector4D cube[6] )
{
	LOCK_SHADERAPI();

	memcpy( m_TargetShaderState.light.m_AmbientLightCube, cube, 6 * sizeof( Vector4D ) );
}

// Get lights
int CShaderAPIDx11::GetMaxLights( void ) const
{
	return HardwareConfig()->MaxNumLights();
}

const LightDesc_t &CShaderAPIDx11::GetLight( int lightNum ) const
{
	return m_TargetShaderState.light.m_Lights[lightNum];
}

void CShaderAPIDx11::SetSkinningMatrices()
{
}

float CShaderAPIDx11::GetLightMapScaleFactor() const
{
	switch ( HardwareConfig()->GetHDRType() )
	{

	case HDR_TYPE_FLOAT:
		return 1.0;
		break;

	case HDR_TYPE_INTEGER:
		return 16.0;

	case HDR_TYPE_NONE:
	default:
		return GammaToLinearFullRange( 2.0 );	// light map scale
	}
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
	//Assert( m_pMesh == 0 );

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

bool CShaderAPIDx11::MatrixIsChanging()
{
	if ( IsDeactivated() )
	{
		return false;
	}

	if ( m_MatrixMode == MATERIAL_MODEL || m_MatrixMode == MATERIAL_VIEW || m_MatrixMode == MATERIAL_PROJECTION )
	{
		FlushBufferedPrimitives();
	}

	return true;
}

void CShaderAPIDx11::HandleMatrixModified()
{
	m_TargetShaderState.m_ChangedMatrices[m_MatrixMode] = true;
}

DirectX::XMMATRIX &CShaderAPIDx11::GetMatrix( MaterialMatrixMode_t mode )
{
	CUtlStack<StatesDx11::MatrixItem_t> &curStack = m_TargetShaderState.m_MatrixStacks[mode];
	if ( !curStack.Count() )
	{
		return DirectX::XMMatrixIdentity();
	}

	return m_TargetShaderState.m_MatrixStacks[mode].Top().m_Matrix;
}

DirectX::XMMATRIX &CShaderAPIDx11::GetCurrentMatrix()
{
	return m_pCurMatrixItem->m_Matrix;
}

DirectX::XMMATRIX CShaderAPIDx11::GetMatrixCopy( MaterialMatrixMode_t mode ) const
{
	const CUtlStack<StatesDx11::MatrixItem_t> &curStack = m_TargetShaderState.m_MatrixStacks[mode];
	if ( !curStack.Count() )
	{
		return DirectX::XMMatrixIdentity();
	}

	return m_TargetShaderState.m_MatrixStacks[mode].Top().m_Matrix;
}

DirectX::XMMATRIX CShaderAPIDx11::GetCurrentMatrixCopy() const
{
	return m_pCurMatrixItem->m_Matrix;
}

void CShaderAPIDx11::MatrixMode( MaterialMatrixMode_t matrixMode )
{
	Assert( m_TargetShaderState.m_MatrixStacks[matrixMode].Count() );
	m_MatrixMode = matrixMode;
	m_pCurMatrixItem = &m_TargetShaderState.m_MatrixStacks[matrixMode].Top();
}

void CShaderAPIDx11::PushMatrix()
{
	// Does nothing in Dx11
	//GetCurrentMatrix() = DirectX::XMMatrixTranspose( GetCurrentMatrix() );

	CUtlStack<StatesDx11::MatrixItem_t> &curStack = m_TargetShaderState.m_MatrixStacks[m_MatrixMode];
	Assert( curStack.Count() );
	int iNew = m_TargetShaderState.m_MatrixStacks[m_MatrixMode].Push();
	curStack[iNew] = curStack[iNew - 1];
	m_pCurMatrixItem = &m_TargetShaderState.m_MatrixStacks[m_MatrixMode].Top();

	HandleMatrixModified();
}

void CShaderAPIDx11::PopMatrix()
{
	MatrixIsChanging();

	Assert( m_TargetShaderState.m_MatrixStacks[m_MatrixMode].Count() > 1 );
	m_TargetShaderState.m_MatrixStacks[m_MatrixMode].Pop();
	m_pCurMatrixItem = &m_TargetShaderState.m_MatrixStacks[m_MatrixMode].Top();

	HandleMatrixModified();
}

void CShaderAPIDx11::LoadMatrix( float *m )
{
	MatrixIsChanging();

	DirectX::XMFLOAT4X4 flt4x4( m );
	GetCurrentMatrix() = DirectX::XMLoadFloat4x4( &flt4x4 );
	HandleMatrixModified();
}

void CShaderAPIDx11::LoadBoneMatrix( int boneIndex, const float *m )
{
	VMatrix boneMatrix;
	boneMatrix.Init( *(matrix3x4_t *)m );

	DirectX::XMFLOAT4X4 flt4x4( boneMatrix.Base() );
	DirectX::XMMATRIX &mat = m_TargetShaderState.bone.m_BoneMatrix[boneIndex];
	mat = DirectX::XMLoadFloat4x4( &flt4x4 );
	if ( boneIndex > m_TargetShaderState.bone.m_MaxBoneLoaded )
	{
		m_TargetShaderState.bone.m_MaxBoneLoaded = boneIndex;
	}
	if ( boneIndex == 0 )
	{
		MatrixMode( MATERIAL_MODEL );
		VMatrix transpose;
		MatrixTranspose( boneMatrix, transpose );
		LoadMatrix( transpose.Base() );
	}
}

void CShaderAPIDx11::MultMatrix( float *m )
{
	MatrixIsChanging();
	DirectX::XMFLOAT4X4 flt4x4( m );
	GetCurrentMatrix() = DirectX::XMMatrixMultiply(
		GetCurrentMatrix(), DirectX::XMLoadFloat4x4( &flt4x4 ) );
	HandleMatrixModified();
}

void CShaderAPIDx11::MultMatrixLocal( float *m )
{
	MatrixIsChanging();
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
	MatrixIsChanging();
	m_pCurMatrixItem->m_Matrix = DirectX::XMMatrixIdentity();
	HandleMatrixModified();
}

void CShaderAPIDx11::LoadCameraToWorld( void )
{
	MatrixIsChanging();

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
	MatrixIsChanging();

	DirectX::XMMATRIX mat = DirectX::XMMatrixOrthographicOffCenterRH( left, right, bottom, top, zNear, zFar );

	// DX11FIXME: Local multiply
	GetCurrentMatrix() = DirectX::XMMatrixMultiply( mat, GetCurrentMatrix() );

	HandleMatrixModified();
}

void CShaderAPIDx11::PerspectiveX( double fovx, double aspect, double zNear, double zFar )
{
	MatrixIsChanging();

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
	MatrixIsChanging();

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
	MatrixIsChanging();

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
	MatrixIsChanging();

	DirectX::XMVECTOR axis;
	axis = DirectX::XMVectorSet( x, y, z, 0 );

	// DX11FIXME: Local multiply
	GetCurrentMatrix() = DirectX::XMMatrixMultiply( DirectX::XMMatrixRotationAxis( axis, M_PI * angle / 180.0f ), GetCurrentMatrix() );

	HandleMatrixModified();
}

void CShaderAPIDx11::Translate( float x, float y, float z )
{
	MatrixIsChanging();

	// DX11FIXME: Local multiply
	GetCurrentMatrix() = DirectX::XMMatrixMultiply( DirectX::XMMatrixTranslation( x, y, z ), GetCurrentMatrix() );
	HandleMatrixModified();
}

void CShaderAPIDx11::Scale( float x, float y, float z )
{
	MatrixIsChanging();

	// DX11FIXME: Local multiply
	GetCurrentMatrix() = DirectX::XMMatrixMultiply( DirectX::XMMatrixScaling( x, y, z ), GetCurrentMatrix() );
	HandleMatrixModified();
}

void CShaderAPIDx11::ScaleXY( float x, float y )
{
	MatrixIsChanging();

	// DX11FIXME: Local multiply
	GetCurrentMatrix() = DirectX::XMMatrixMultiply( DirectX::XMMatrixScaling( x, y, 1.0f ), GetCurrentMatrix() );
	HandleMatrixModified();
}

// Fog methods...
void CShaderAPIDx11::FogMode( MaterialFogMode_t fogMode )
{
	m_TargetShaderState.fog.m_FogMode = fogMode;
}

void CShaderAPIDx11::FogStart( float fStart )
{
	m_TargetShaderState.fog.m_flFogStart = fStart;
}

void CShaderAPIDx11::FogEnd( float fEnd )
{
	m_TargetShaderState.fog.m_flFogStart = fEnd;
}

void CShaderAPIDx11::SetFogZ( float fogZ )
{
	m_TargetShaderState.fog.m_flFogZ = fogZ;
}

void CShaderAPIDx11::FogMaxDensity( float flMaxDensity )
{
	m_TargetShaderState.fog.m_flFogMaxDensity = flMaxDensity;
}

void CShaderAPIDx11::GetFogDistances( float *fStart, float *fEnd, float *fFogZ )
{
	*fStart = m_TargetShaderState.fog.m_flFogStart;
	*fEnd = m_TargetShaderState.fog.m_flFogEnd;
	*fFogZ = m_TargetShaderState.fog.m_flFogZ;
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

// Sets the *dynamic* vertex and pixel shaders
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

	StatesDx11::TextureSlot_t slotdef;
	slotdef.slot = (int)stage;
	slotdef.m_pTexture = pTex;

	m_TargetState.dynamic.m_pSamplers[m_TargetState.dynamic.m_nSamplers++] = slotdef;
}

void CShaderAPIDx11::UnbindTexture( ShaderAPITextureHandle_t textureHandle )
{
	CTextureDx11 *pTex = &GetTexture( textureHandle );

	// Unbind the sampler

	int iSampler = -1;
	for ( int i = 0; i < m_TargetState.dynamic.m_nSamplers; i++ )
	{
		if ( m_TargetState.dynamic.m_pSamplers[i].m_pTexture == pTex )
		{
			iSampler = i;
			break;
		}
	}

	if ( iSampler == -1 )
		return;

	for ( int i = iSampler; i < m_TargetState.dynamic.m_nSamplers - 1; i++ )
	{
		m_TargetState.dynamic.m_pSamplers[i] =
			m_TargetState.dynamic.m_pSamplers[i + 1];
	}
	m_TargetState.dynamic.m_nSamplers--;
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
	info.m_pView = tex.GetView();
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
	//Log( "TexSubImage2D!\n" );

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
	info.m_pView = tex.GetView();
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
	LOCK_SHADERAPI();

	Assert( m_ModifyTextureHandle > 0 );

	ShaderAPITextureHandle_t hTexture = m_ModifyTextureHandle;
	if ( !m_Textures.IsValidIndex( hTexture ) )
		return false;

	// Blow off mip levels if we don't support mipmapping
	if ( !g_pHardwareConfig->SupportsMipmapping() && ( level > 0 ) )
		return false;

	// This test here just makes sure we don't try to download mipmap levels
	// if we weren't able to create them in the first place
	CTextureDx11 &tex = GetTexture( hTexture );
	if ( level >= tex.m_NumLevels )
	{
		return false;
	}

	// May need to switch textures....
	if ( tex.m_SwitchNeeded )
	{
		AdvanceCurrentTextureCopy( hTexture );
		tex.m_SwitchNeeded = false;
	}

	bool ret = tex.Lock( tex.m_CurrentCopy, level, cubeFaceID, xOffset, yOffset,
			     width, height, false, writer );
	if ( ret )
	{
		m_ModifyTextureLockedLevel = level;
		m_ModifyTextureLockedFace = cubeFaceID;
	}

	return ret;
}

void CShaderAPIDx11::TexUnlock()
{
	LOCK_SHADERAPI();

	if ( m_ModifyTextureLockedLevel >= 0 )
	{
		CTextureDx11 &tex = GetTexture( m_ModifyTextureHandle );
		tex.Unlock( tex.m_CurrentCopy, m_ModifyTextureLockedLevel,
			    m_ModifyTextureLockedFace );

		m_ModifyTextureLockedLevel = -1;
	}
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
	Log( "Create Texture with format: %s\n", ImageLoader::GetName( dstImageFormat ) );
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

	Log( "Creating Textures with format: %s\n", ImageLoader::GetName( dstImageFormat ) );

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
	LOCK_SHADERAPI();

	if ( !bClearColor && !bClearDepth )
		return;

	FlushBufferedPrimitives();

	ShaderUtil()->DrawClearBufferQuad( m_TargetState.dynamic.m_ClearColor[0] * 255,
					   m_TargetState.dynamic.m_ClearColor[1] * 255,
					   m_TargetState.dynamic.m_ClearColor[2] * 255,
					   m_TargetState.dynamic.m_ClearColor[3] * 255,
					   bClearColor, bClearDepth );

	FlushBufferedPrimitives();
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
	LOCK_SHADERAPI();
	if ( m_TargetShaderState.bone.m_NumBones != numBones )
	{
		FlushBufferedPrimitives();
		m_TargetShaderState.bone.m_NumBones = numBones;
	}
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
	return m_TargetShaderState.bone.m_NumBones;
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

// Rendering parameters

void CShaderAPIDx11::SetFloatRenderingParameter( int parm_number, float value )
{
	LOCK_SHADERAPI();
	if ( parm_number < ARRAYSIZE( FloatRenderingParameters ) )
		FloatRenderingParameters[parm_number] = value;
}

void CShaderAPIDx11::SetIntRenderingParameter( int parm_number, int value )
{
	LOCK_SHADERAPI();
	if ( parm_number < ARRAYSIZE( IntRenderingParameters ) )
		IntRenderingParameters[parm_number] = value;
}

void CShaderAPIDx11::SetVectorRenderingParameter( int parm_number, Vector const &value )
{
	LOCK_SHADERAPI();
	if ( parm_number < ARRAYSIZE( VectorRenderingParameters ) )
		VectorRenderingParameters[parm_number] = value;
}

float CShaderAPIDx11::GetFloatRenderingParameter( int parm_number ) const
{
	LOCK_SHADERAPI();
	if ( parm_number < ARRAYSIZE( FloatRenderingParameters ) )
		return FloatRenderingParameters[parm_number];
	else
		return 0.0;
}

int CShaderAPIDx11::GetIntRenderingParameter( int parm_number ) const
{
	LOCK_SHADERAPI();
	if ( parm_number < ARRAYSIZE( IntRenderingParameters ) )
		return IntRenderingParameters[parm_number];
	else
		return 0;
}

Vector CShaderAPIDx11::GetVectorRenderingParameter( int parm_number ) const
{
	LOCK_SHADERAPI();
	if ( parm_number < ARRAYSIZE( VectorRenderingParameters ) )
		return VectorRenderingParameters[parm_number];
	else
		return Vector( 0, 0, 0 );
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

int CShaderAPIDx11::CompareSnapshots( StateSnapshot_t snapshot0, StateSnapshot_t snapshot1 )
{
	LOCK_SHADERAPI();

	const StatesDx11::ShadowState &shadow0 = g_pShaderShadowDx11->GetShadowState( snapshot0 );
	const StatesDx11::ShadowState &shadow1 = g_pShaderShadowDx11->GetShadowState( snapshot1 );

	return memcmp( &shadow0, &shadow1, sizeof( StatesDx11::ShadowState ) );
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

	uint64 nPixels;
	HRESULT hr = D3D11DeviceContext()->GetData( (ID3D11Query *)handle, &nPixels, sizeof( uint64 ),
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

void CShaderAPIDx11::BindStandardTexture( Sampler_t stage, StandardTextureId_t id )
{
	ShaderUtil()->BindStandardTexture( stage, id );
}

void CShaderAPIDx11::BindVertexTexture( VertexTextureSampler_t stage, ShaderAPITextureHandle_t hTexture )
{
	CTextureDx11 *pTex = &GetTexture( hTexture );

	StatesDx11::TextureSlot_t slotdef;
	slotdef.slot = (int)stage;
	slotdef.m_pTexture = pTex;

	m_TargetState.dynamic.m_pVSSamplers[m_TargetState.dynamic.m_nVSSamplers++] = slotdef;
}

void CShaderAPIDx11::BindStandardVertexTexture( VertexTextureSampler_t stage, StandardTextureId_t id )
{
	ShaderUtil()->BindStandardVertexTexture( stage, id );
}

template<class T> FORCEINLINE T GetData( uint8 const *pData )
{
	return *( reinterpret_cast<T const *>( pData ) );
}

void CShaderAPIDx11::ExecuteCommandBuffer( uint8 *pCmdBuf )
{
	uint8 *pReturnStack[20];
	uint8 **pSP = &pReturnStack[ARRAYSIZE( pReturnStack )];
	uint8 *pLastCmd;
	for ( ;;)
	{
		uint8 *pCmd = pCmdBuf;
		int nCmd = GetData<int>( pCmdBuf );
		switch ( nCmd )
		{
		case CBCMD_END:
		{
			if ( pSP == &pReturnStack[ARRAYSIZE( pReturnStack )] )
				return;
			else
			{
				// pop pc
				pCmdBuf = *( pSP++ );
				break;
			}
		}

		case CBCMD_JUMP:
			pCmdBuf = GetData<uint8 *>( pCmdBuf + sizeof( int ) );
			break;

		case CBCMD_JSR:
		{
			Assert( pSP > &( pReturnStack[0] ) );
			ExecuteCommandBuffer( GetData<uint8 *>( pCmdBuf + sizeof( int ) ) );
			pCmdBuf = pCmdBuf + sizeof( int ) + sizeof( uint8 * );
			break;
		}
		case CBCMD_BIND_STANDARD_TEXTURE:
		{
			int nSampler = GetData<int>( pCmdBuf + sizeof( int ) );
			int nTextureID = GetData<int>( pCmdBuf + 2 * sizeof( int ) );
			pCmdBuf += 3 * sizeof( int );
			ShaderUtil()->BindStandardTexture( (Sampler_t)nSampler, (StandardTextureId_t)nTextureID );
			break;
		}

		case CBCMD_BIND_SHADERAPI_TEXTURE_HANDLE:
		{
			int nSampler = GetData<int>( pCmdBuf + sizeof( int ) );
			ShaderAPITextureHandle_t hTexture = GetData<ShaderAPITextureHandle_t>( pCmdBuf + 2 * sizeof( int ) );
			Assert( hTexture != INVALID_SHADERAPI_TEXTURE_HANDLE );
			pCmdBuf += 2 * sizeof( int ) + sizeof( ShaderAPITextureHandle_t );
			BindTexture( (Sampler_t)nSampler, hTexture );
			break;
		}

		case CBCMD_SET_PSHINDEX:
		{
			int nIdx = GetData<int>( pCmdBuf + sizeof( int ) );
			SetPixelShaderIndex( nIdx );
			pCmdBuf += 2 * sizeof( int );
			break;
		}

		case CBCMD_SET_VSHINDEX:
		{
			int nIdx = GetData<int>( pCmdBuf + sizeof( int ) );
			SetVertexShaderIndex( nIdx );
			pCmdBuf += 2 * sizeof( int );
			break;
		}
#ifndef NDEBUG
		default:
		{
			Assert( 0 );
		}
#endif
		}
		pLastCmd = pCmd;
	}
}

bool CShaderAPIDx11::ShouldWriteDepthToDestAlpha() const
{
	return IsPC() && g_pHardwareConfig->SupportsPixelShaders_2_b() &&
		( m_TargetShaderState.fog.m_FogMode != MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) &&
		( GetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA ) != 0 );
}

void CShaderAPIDx11::ClearVertexAndPixelShaderRefCounts()
{
	LOCK_SHADERAPI();
	ShaderManager()->ClearVertexAndPixelShaderRefCounts();
}

void CShaderAPIDx11::PurgeUnusedVertexAndPixelShaders()
{
	LOCK_SHADERAPI();
	ShaderManager()->PurgeUnusedVertexAndPixelShaders();
}

void CShaderAPIDx11::SetFlashlightState( const FlashlightState_t &state, const VMatrix &worldToTexture )
{
	LOCK_SHADERAPI();
	SetFlashlightStateEx( state, worldToTexture, NULL );
}

void CShaderAPIDx11::SetFlashlightStateEx( const FlashlightState_t &state, const VMatrix &worldToTexture, ITexture *pFlashlightDepthTexture )
{
	LOCK_SHADERAPI();
	FlushBufferedPrimitives();
	m_FlashlightState = state;
	m_FlashlightWorldToTexture = worldToTexture;
	m_pFlashlightDepthTexture = pFlashlightDepthTexture;
	m_bFlashlightStateChanged = true;
}

const FlashlightState_t &CShaderAPIDx11::GetFlashlightState( VMatrix &worldToTexture ) const
{
	worldToTexture = m_FlashlightWorldToTexture;
	return m_FlashlightState;
}

const FlashlightState_t &CShaderAPIDx11::GetFlashlightStateEx( VMatrix &worldToTexture, ITexture **pFlashlightDepthTexture ) const
{
	worldToTexture = m_FlashlightWorldToTexture;
	*pFlashlightDepthTexture = m_pFlashlightDepthTexture;
	return m_FlashlightState;
}

#define MAX_LIGHTS 4

void CShaderAPIDx11::GetDX9LightState( LightState_t *state ) const
{
	// hack . . do this a cheaper way.
	if ( m_TargetShaderState.light.m_AmbientLightCube[0][0] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[0][1] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[0][2] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[1][0] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[1][1] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[1][2] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[2][0] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[2][1] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[2][2] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[3][0] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[3][1] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[3][2] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[4][0] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[4][1] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[4][2] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[5][0] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[5][1] == 0.0f &&
	     m_TargetShaderState.light.m_AmbientLightCube[5][2] == 0.0f )
	{
		state->m_bAmbientLight = false;
	}
	else
	{
		state->m_bAmbientLight = true;
	}

	Assert( m_pMesh );
	Assert( m_TargetShaderState.light.m_NumLights <= 4 );

	if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
	{
		Assert( m_TargetShaderState.light.m_NumLights <= MAX_LIGHTS );		// 2b hardware gets four lights
	}
	else
	{
		Assert( m_TargetShaderState.light.m_NumLights <= ( MAX_LIGHTS - 2 ) );	// 2.0 hardware gets two less
	}

	state->m_nNumLights = m_TargetShaderState.light.m_NumLights;
	state->m_bStaticLight = m_pMesh->HasColorMesh();
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
	//Warning( "Unsupported CShaderAPIDx11::SetTextureTransformDimension() called!\n" );
}

void CShaderAPIDx11::SetBumpEnvMatrix( TextureStage_t textureStage, float m00, float m01, float m10, float m11 )
{
	//Warning( "Unsupported CShaderAPIDx11::SetBumpEnvMatrix() called!\n" );
}

void CShaderAPIDx11::SetAmbientLight( float r, float g, float b )
{
	//Warning( "Unsupported CShaderAPIDx11::SetAmbientLight() called!\n" );
}

//-----------------------------------------------------------------------------
// Methods related to state objects
//-----------------------------------------------------------------------------
void CShaderAPIDx11::SetRasterState( const ShaderRasterState_t &state )
{
	//Warning( "Unsupported CShaderAPIDx11::SetRasterState() called!\n" );
}

// The shade mode
void CShaderAPIDx11::ShadeMode( ShaderShadeMode_t mode )
{
	//Warning( "Unsupported CShaderAPIDx11::ShadeMode() called!\n" );
}

void CShaderAPIDx11::TexSetPriority( int priority )
{
	//Warning( "Unsupported CShaderAPIDx11::SetTexPriority() called!\n" );
}

// Sets the constant register for vertex and pixel shaders
void CShaderAPIDx11::SetVertexShaderConstant( int var, float const *pVec, int numConst, bool bForce )
{
}

void CShaderAPIDx11::SetPixelShaderConstant( int var, float const *pVec, int numConst, bool bForce )
{
}

void CShaderAPIDx11::SetPixelShaderFogParams( int psReg )
{
}

void CShaderAPIDx11::InvalidateDelayedShaderConstants( void )
{
	//Warning( "Unsupported CShaderAPIDx11::InvalidateDelayedShaderConstants() called!\n" );
}

//Set's the linear->gamma conversion textures to use for this hardware for both srgb writes enabled and disabled(identity)
void CShaderAPIDx11::SetLinearToGammaConversionTextures( ShaderAPITextureHandle_t hSRGBWriteEnabledTexture, ShaderAPITextureHandle_t hIdentityTexture )
{
	//Warning( "Unsupported CShaderAPIDx11::SetLinearToGammaConversionTextures() called!\n" );
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
	//Warning( "Unsupported CShaderAPIDx11::SetVertexShaderStateAmbientLightCube() called!\n" );
}