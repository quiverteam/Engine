#pragma once

#include "shaderapi/ishaderdevice.h"
#include "shaderapi/ishadershadow.h"
#include "shaderapi/ishaderapi.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imorph.h"
#include <utlstack.h>

#include <d3d11.h>

#include "Dx11Global.h"

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

namespace StatesDx11
{
	// These states below describe the D3D pipeline state.

	//
	// Things that are set once on SHADOW_STATE and don't
	// change per-mesh or per-frame (in DYNAMIC_STATE)
	//

	struct DepthStencilState : public D3D11_DEPTH_STENCIL_DESC
	{
		int StencilRef;

		void SetDefault()
		{
			DepthEnable = TRUE;
			DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			DepthFunc = D3D11_COMPARISON_LESS;
			StencilEnable = FALSE;
			StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
			StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
			const D3D11_DEPTH_STENCILOP_DESC defaultStencilOp =
			{ D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS };
			FrontFace = defaultStencilOp;
			BackFace = defaultStencilOp;

			StencilRef = 0;
		}
	};

	struct BlendState : public D3D11_BLEND_DESC
	{
		float BlendColor[4];
		uint SampleMask;

		void SetDefault()
		{
			AlphaToCoverageEnable = FALSE;
			IndependentBlendEnable = FALSE;
			const D3D11_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
			{
			    FALSE,
			    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
			    D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
			    D3D11_COLOR_WRITE_ENABLE_ALL,
			};
			for ( UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i )
				RenderTarget[i] = defaultRenderTargetBlendDesc;

			BlendColor[0] = BlendColor[1] = BlendColor[2] = BlendColor[3] = 1.0f;
			SampleMask = 1;
		}
	};

	struct RasterState : public D3D11_RASTERIZER_DESC
	{
		void SetDefault()
		{
			FillMode = D3D11_FILL_SOLID;
			CullMode = D3D11_CULL_BACK;
			FrontCounterClockwise = FALSE;
			DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
			DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
			SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			DepthClipEnable = TRUE;
			ScissorEnable = FALSE;
			MultisampleEnable = FALSE;
			AntialiasedLineEnable = FALSE;
		}
	};

	struct ConstantBufferState
	{
		int m_MaxSlot;
		ID3D11Buffer *m_ppBuffers[MAX_DX11_CBUFFERS];

		bool operator==( const ConstantBufferState &other ) const
		{
			if ( m_MaxSlot != other.m_MaxSlot )
			{
				return false;
			}

			return memcmp( m_ppBuffers, other.m_ppBuffers, sizeof( m_ppBuffers ) ) == 0;
		}

		void SetDefault()
		{
			m_MaxSlot = -1;
			memset( m_ppBuffers, 0, sizeof( m_ppBuffers ) );
		}
	};

	struct ShadowStateDesc
	{
		BlendState blend;
		DepthStencilState depthStencil;
		RasterState rasterizer;

		ConstantBufferState vsConstantBuffers;
		ConstantBufferState gsConstantBuffers;
		ConstantBufferState psConstantBuffers;

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
		int staticGeometryShaderIndex;

		// Vertex data used by this snapshot
		// Note that the vertex format actually used will be the
		// aggregate of the vertex formats used by all snapshots in a material
		VertexFormat_t vertexFormat;

		// Morph data used by this snapshot
		// Note that the morph format actually used will be the
		// aggregate of the morph formats used by all snapshots in a material
		MorphFormat_t morphFormat;

		bool BlendStateChanged( const ShadowStateDesc &other )
		{
			return FastMemCompare( &blend, &other.blend, sizeof( BlendState ) ) != 0;
		}

		bool DepthStencilStateChanged( const ShadowStateDesc &other )
		{
			return FastMemCompare( &depthStencil, &other.depthStencil, sizeof( DepthStencilState ) ) != 0;
		}

		bool RasterStateChanged( const ShadowStateDesc &other )
		{
			return FastMemCompare( &rasterizer, &other.rasterizer, sizeof( RasterState ) ) != 0;
		}

		// Sets the default shadow state
		void SetDefault()
		{
			vsConstantBuffers.SetDefault();
			gsConstantBuffers.SetDefault();
			psConstantBuffers.SetDefault();

			depthStencil.SetDefault();
			rasterizer.SetDefault();
			blend.SetDefault();

			bEnableAlphaTest = false;
			alphaTestFunc = SHADER_ALPHAFUNC_GEQUAL;
			alphaTestRef = 0.0f;

			vertexShader = -1;
			staticVertexShaderIndex = 0;
			pixelShader = -1;
			staticPixelShaderIndex = 0;
			geometryShader = -1;
			staticGeometryShaderIndex = 0;

			vertexFormat = VERTEX_FORMAT_UNKNOWN;
			morphFormat = 0;

			bLighting = false;
			bVertexBlend = false;
			bConstantColor = false;
		}

		ShadowStateDesc()
		{
			SetDefault();
		}
	};

	struct ShadowState
	{
		ShadowStateDesc desc;

		ID3D11BlendState *m_pBlendState;
		ID3D11DepthStencilState *m_pDepthStencilState;
		ID3D11RasterizerState *m_pRasterState;

		unsigned int m_iVSConstantBufferState;
		unsigned int m_iPSConstantBufferState;
		unsigned int m_iGSConstantBufferState;

		bool operator==( const ShadowState &other ) const
		{
			return memcmp( &desc, &other.desc, sizeof( ShadowStateDesc ) ) == 0;
		}

		ShadowState()
		{
			m_pBlendState = NULL;
			m_pDepthStencilState = NULL;
			m_pRasterState = NULL;
			m_iVSConstantBufferState = 0;
			m_iPSConstantBufferState = 0;
			m_iGSConstantBufferState = 0;
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

		ID3D11VertexShader *m_pVertexShader;
		int m_iVertexShader;
		ID3D11GeometryShader *m_pGeometryShader;
		int m_iGeometryShader;
		ID3D11PixelShader *m_pPixelShader;
		int m_iPixelShader;
		
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
		const ShadowState *shadow;
		DynamicState dynamic;

		void SetDefault()
		{
			shadow = NULL;
			dynamic.SetDefault();
		}

		void operator=( const RenderState &other )
		{
			shadow = other.shadow;
			memcpy( &dynamic, &other.dynamic, sizeof( DynamicState ) );
		}
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
		DirectX::XMFLOAT3X4A m_BoneMatrix[NUM_MODEL_TRANSFORMS];
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

			memset( bone.m_BoneMatrix, 0, sizeof( bone.m_BoneMatrix ) );
			//for ( int i = 0; i < NUM_MODEL_TRANSFORMS; i++ )
			//{
			//	bone.m_BoneMatrix[i] = DirectX::XMMatrixIdentity();
			//	bone.m_BoneMatrix[i] = DirectX::XMMatrixTranspose(
			//		bone.m_BoneMatrix[i]
			//	);
			//}
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