#pragma once

#include "shaderapi/ishaderdevice.h"
#include "shaderapi/ishadershadow.h"
#include "shaderapi/ishaderapi.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imorph.h"
#include <utlstack.h>

#include <d3d11.h>

class CShaderConstantBufferDx11;
class CTextureDx11;

//-----------------------------------------------------------------------------
// DX11 enumerations that don't appear to exist
//-----------------------------------------------------------------------------
#define MAX_DX11_VIEWPORTS 16
#define MAX_DX11_STREAMS 16
#define MAX_DX11_CBUFFERS	15
#define MAX_DX11_SAMPLERS	16

FORCEINLINE static D3D11_BLEND TranslateD3D11BlendFunc( ShaderBlendFactor_t blend )
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

namespace ShadowStatesDx11
{

	struct AlphaTestAttrib
	{
		bool bEnable;
		ShaderAlphaFunc_t alphaFunc;
		float alphaTestRef;

		AlphaTestAttrib()
		{
			bEnable = false;
			alphaFunc = SHADER_ALPHAFUNC_LESS;
			alphaTestRef = 0.0f;
		}
	};

	struct TransparencyAttrib
	{
		bool bTranslucent;

		TransparencyAttrib()
		{
			bTranslucent = false;
		}
	};

	struct ColorAttrib
	{
		bool bEnable;

		ColorAttrib()
		{
			bEnable = false;
		}
	};

	struct LightAttrib
	{
		bool bEnable;

		LightAttrib()
		{
			bEnable = false;
		}
	};

	struct VertexBlendAttrib
	{
		bool bEnable;

		VertexBlendAttrib()
		{
			bEnable = false;
		}
	};
}


namespace StatesDx11
{
	// These states below describe the D3D pipeline state.

	//
	// Things that are set once on SHADOW_STATE and don't
	// change per-mesh or per-frame (in DYNAMIC_STATE)
	//

	struct DepthStencilState : public CD3D11_DEPTH_STENCIL_DESC
	{
		int StencilRef;
	};

	struct BlendState : public CD3D11_BLEND_DESC
	{
		float BlendColor[4];
		uint SampleMask;
	};

	struct RasterState : public CD3D11_RASTERIZER_DESC
	{
	};

	struct ShadowState
	{
		BlendState blend;
		DepthStencilState depthStencil;
		RasterState rasterizer;

		bool bEnableAlphaTest;
		ShaderAlphaFunc_t alphaTestFunc;
		float alphaTestRef;

		bool bLighting;
		bool bConstantColor;
		bool bVertexBlend;

		VertexShader_t vertexShader;
		int staticVertexShaderIndex;
		PixelShader_t pixelShader;
		int staticPixelShaderIndex;
		GeometryShader_t geometryShader;
		int geometryShaderIndex;

		// Vertex data used by this snapshot
		// Note that the vertex format actually used will be the
		// aggregate of the vertex formats used by all snapshots in a material
		VertexFormat_t vertexFormat;

		// Morph data used by this snapshot
		// Note that the morph format actually used will be the
		// aggregate of the morph formats used by all snapshots in a material
		MorphFormat_t morphFormat;

		bool BlendStateChanged( const ShadowState &other )
		{
			return memcmp( &blend, &other.blend, sizeof( BlendState ) ) != 0;
		}

		bool DepthStencilStateChanged( const ShadowState &other )
		{
			return memcmp( &depthStencil, &other.depthStencil, sizeof( DepthStencilState ) ) != 0;
		}

		bool RasterStateChanged( const ShadowState &other )
		{
			return memcmp( &rasterizer, &other.rasterizer, sizeof( RasterState ) ) != 0;
		}

		// Sets the default shadow state
		void SetDefault()
		{
			ZeroMemory( this, sizeof( ShadowState ) );

			depthStencil.DepthEnable = true;
			depthStencil.DepthFunc = D3D11_COMPARISON_LESS;
			depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

			blend.BlendColor[0] = blend.BlendColor[1] =
				blend.BlendColor[2] = blend.BlendColor[3] = 1.0f;
			blend.SampleMask = 1;

			rasterizer.FillMode = D3D11_FILL_SOLID;
			rasterizer.CullMode = D3D11_CULL_NONE;
			rasterizer.FrontCounterClockwise = TRUE; // right-hand rule

			D3D11_RENDER_TARGET_BLEND_DESC *rtbDesc = &blend.RenderTarget[0];
			rtbDesc->SrcBlend = D3D11_BLEND_ONE;
			rtbDesc->DestBlend = D3D11_BLEND_ZERO;
			rtbDesc->BlendOp = D3D11_BLEND_OP_ADD;
			rtbDesc->SrcBlendAlpha = D3D11_BLEND_ONE;
			rtbDesc->DestBlendAlpha = D3D11_BLEND_ZERO;
			rtbDesc->BlendOpAlpha = D3D11_BLEND_OP_ADD;
			rtbDesc->RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			alphaTestFunc = SHADER_ALPHAFUNC_GEQUAL;

			vertexShader = -1;
			pixelShader = -1;
			geometryShader = -1;
		}

		ShadowState()
		{
			SetDefault();
		}

		bool operator==( const ShadowState &other ) const
		{
			return memcmp( this, &other, sizeof( ShadowState ) ) == 0;
		}
	};

	//
	// Things that can change per-mesh or per-frame (in DYNAMIC_STATE)
	//

	struct IndexBufferState
	{
		ID3D11Buffer *m_pBuffer;
		DXGI_FORMAT m_Format;
		UINT m_nOffset;

		bool operator!=( const IndexBufferState &src ) const
		{
			return memcmp( this, &src, sizeof( IndexBufferState ) ) != 0;
		}
	};

	struct VertexBufferState
	{
		ID3D11Buffer *m_pBuffer;
		UINT m_nStride;
		UINT m_nOffset;
	};

	struct InputLayoutState
	{
		VertexShaderHandle_t m_hVertexShader;
		VertexFormat_t m_pVertexDecl[MAX_DX11_STREAMS];
		bool m_bStaticLit;
		bool m_bUsingFlex;
		bool m_bUsingMorph;
	};

	struct RenderTargetState
	{
		ShaderAPITextureHandle_t m_RenderTarget;
		ShaderAPITextureHandle_t m_DepthTarget;
	};

	struct DynamicState
	{
		int m_nViewportCount;
		D3D11_VIEWPORT m_pViewports[MAX_DX11_VIEWPORTS];
		FLOAT m_ClearColor[4];

		InputLayoutState m_InputLayout;
		IndexBufferState m_IndexBuffer;
		VertexBufferState m_pVertexBuffer[MAX_DX11_STREAMS];

		D3D11_PRIMITIVE_TOPOLOGY m_Topology;

		ID3D11BlendState *m_pBlendState;
		ID3D11DepthStencilState *m_pDepthStencilState;
		ID3D11RasterizerState *m_pRasterState;

		ID3D11VertexShader *m_pVertexShader;
		int m_iVertexShader;
		ID3D11GeometryShader *m_pGeometryShader;
		int m_iGeometryShader;
		ID3D11PixelShader *m_pPixelShader;
		int m_iPixelShader;

		ID3D11Buffer *m_pVSConstantBuffers[MAX_DX11_CBUFFERS];
		int m_MaxVSConstantBufferSlot;
		ID3D11Buffer *m_pGSConstantBuffers[MAX_DX11_CBUFFERS];
		int m_MaxGSConstantBufferSlot;
		ID3D11Buffer *m_pPSConstantBuffers[MAX_DX11_CBUFFERS];
		int m_MaxPSConstantBufferSlot;
		
		ID3D11ShaderResourceView *m_pVSSRVs[MAX_DX11_SAMPLERS];
		ID3D11SamplerState *m_pVSSamplers[MAX_DX11_SAMPLERS];
		int m_MaxVSSamplerSlot;

		ID3D11ShaderResourceView *m_pSRVs[MAX_DX11_SAMPLERS];
		ID3D11SamplerState *m_pSamplers[MAX_DX11_SAMPLERS];
		int m_MaxSamplerSlot;

		ID3D11RenderTargetView *m_pRenderTargetView;
		ID3D11DepthStencilView *m_pDepthStencilView;

		// Sets the default dynamic state
		void SetDefault()
		{
			ZeroMemory( this, sizeof( DynamicState ) );
			m_iVertexShader = -1;
			m_iPixelShader = -1;
			m_iGeometryShader = -1;
			m_Topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
			m_MaxVSConstantBufferSlot = -1;
			m_MaxGSConstantBufferSlot = -1;
			m_MaxPSConstantBufferSlot = -1;
			m_MaxVSSamplerSlot = -1;
			m_MaxSamplerSlot = -1;
		}

		DynamicState()
		{
			SetDefault();
		}
	};

	struct RenderState
	{
		ShadowState shadow;
		DynamicState dynamic;
	};

	// These states describe the dynamic shader state.
	// (the state of constants that we provide to shaders)

	struct MatrixItem_t
	{
		DirectX::XMMATRIX m_Matrix;
		int m_Flags;
	};

	struct FogState
	{
		MaterialFogMode_t m_FogMode;
		float m_flFogStart;
		float m_flFogEnd;
		float m_flFogZ;
		float m_flFogMaxDensity;
		float m_FogColor[3];

		bool m_bFogChanged;
	};

	struct LightState
	{
		LightDesc_t m_Lights[MAX_NUM_LIGHTS];
		LightType_t m_LightType[MAX_NUM_LIGHTS];
		int m_NumLights;
		bool m_bLightChanged;

		VectorAligned m_AmbientLightCube[6];
		bool m_bAmbientChanged;
	};

	struct BoneState
	{
		DirectX::XMMATRIX m_BoneMatrix[NUM_MODEL_TRANSFORMS];
		int m_MaxBoneLoaded;
		int m_NumBones;

		bool m_bBonesChanged;
	};

	struct MorphState
	{
		int m_nFirstWeight;
		int m_nCount;
		MorphWeight_t m_pWeights[512];
		int m_nMaxWeightLoaded;

		bool m_bMorphChanged;
	};

	// State that is set through constant buffers and used
	// by shaders.
	struct ShaderState
	{
		FogState fog;
		LightState light;
		BoneState bone;
		MorphState morph;

		Vector4D m_ConstantColor;
		bool m_bConstantColorChanged;

		CUtlStack<MatrixItem_t> m_MatrixStacks[NUM_MATRIX_MODES];
		bool m_ChangedMatrices[NUM_MATRIX_MODES];

		void SetDefault()
		{
			ZeroMemory( this, sizeof( ShaderState ) );

			fog.m_FogColor[0] = 1.0f;
			fog.m_FogColor[1] = 1.0f;
			fog.m_FogColor[2] = 1.0f;
			fog.m_flFogMaxDensity = -1.0f;
			fog.m_flFogZ = 0.0f;
			fog.m_flFogEnd = -1;
			fog.m_flFogStart = -1;
			fog.m_FogMode = MATERIAL_FOG_NONE;

			m_ConstantColor.Init( 1.0f, 1.0f, 1.0f, 1.0f );

			for ( int i = 0; i < NUM_MODEL_TRANSFORMS; i++ )
			{
				bone.m_BoneMatrix[i] = DirectX::XMMatrixIdentity();
				bone.m_BoneMatrix[i] = DirectX::XMMatrixTranspose(
					bone.m_BoneMatrix[i]
				);
			}
			bone.m_bBonesChanged = true;

			for ( int i = 0; i < MAX_NUM_LIGHTS; i++ )
			{
				light.m_Lights[i].m_Type = MATERIAL_LIGHT_DISABLE;
			}
			light.m_bLightChanged = true;
			light.m_bAmbientChanged = true;

			for ( int i = 0; i < NUM_MATRIX_MODES; i++ )
			{
				m_MatrixStacks[i].Clear();
				m_MatrixStacks[i].Push();
				m_MatrixStacks[i].Top().m_Matrix = DirectX::XMMatrixIdentity();
				m_MatrixStacks[i].Top().m_Flags = 0;//( MATRIXDX11_DIRTY | MATRIXDX11_IDENTITY );
				m_ChangedMatrices[i] = true;
			}

			morph.m_bMorphChanged = true;
		}

		ShaderState()
		{
			SetDefault();
		}
	};
}