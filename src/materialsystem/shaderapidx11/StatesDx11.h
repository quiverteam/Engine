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

enum DX11StateChangeFlags_t
{
	STATE_CHANGED_NONE		= 0,
	STATE_CHANGED_SAMPLERS		= 1 << 0,
	STATE_CHANGED_VERTEXSAMPLERS	= 1 << 1,
	STATE_CHANGED_VIEWPORTS		= 1 << 2,
	STATE_CHANGED_VERTEXBUFFER	= 1 << 3,
	STATE_CHANGED_INPUTLAYOUT	= 1 << 4,
	STATE_CHANGED_INDEXBUFFER	= 1 << 5,
	STATE_CHANGED_RENDERTARGET	= 1 << 6,
	STATE_CHANGED_DEPTHBUFFER	= 1 << 7,
	STATE_CHANGED_VERTEXSHADER	= 1 << 8,
	STATE_CHANGED_GEOMETRYSHADER	= 1 << 9,
	STATE_CHANGED_PIXELSHADER	= 1 << 10,
	STATE_CHANGED_TOPOLOGY		= 1 << 11,
	STATE_CHANGED_RASTERIZER	= 1 << 12,
	STATE_CHANGED_BLEND		= 1 << 13,
	STATE_CHANGED_DEPTHSTENCIL	= 1 << 14,
	STATE_CHANGED_VSCONSTANTBUFFERS = 1 << 15,
	STATE_CHANGED_GSCONSTANTBUFFERS = 1 << 16,
	STATE_CHANGED_PSCONSTANTBUFFERS = 1 << 17,
	STATE_CHANGED_TEXTURES		= 1 << 18,
	STATE_CHANGED_VERTEXTEXTURES	= 1 << 19,
};

namespace StatesDx11
{
	// These states below describe the D3D pipeline state.

	//
	// Things that are set once on SHADOW_STATE and don't
	// change per-mesh or per-frame (in DYNAMIC_STATE)
	//

	struct DepthStencilDesc : public D3D11_DEPTH_STENCIL_DESC
	{
		int StencilRef;

		ID3D11DepthStencilState *m_pD3DState;

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
			m_pD3DState = NULL;
		}

		bool operator==( const DepthStencilDesc &other ) const
		{
			return memcmp( this, &other, sizeof( DepthStencilDesc ) - sizeof( m_pD3DState ) ) == 0;
		}
	};

	struct BlendDesc : public D3D11_BLEND_DESC
	{
		float BlendColor[4];
		uint SampleMask;

		ID3D11BlendState *m_pD3DState;

		bool BlendEnabled() const
		{
			return (bool)RenderTarget[0].BlendEnable;
		}

		void SetDefault()
		{
			m_pD3DState = NULL;
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

		bool operator==( const BlendDesc &other ) const
		{
			return memcmp( this, &other, sizeof( BlendDesc ) - sizeof( m_pD3DState ) ) == 0;
		}
	};

	struct RasterDesc : public D3D11_RASTERIZER_DESC
	{
		ID3D11RasterizerState *m_pD3DState;

		void SetDefault()
		{
			m_pD3DState = NULL;
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

		bool operator==( const RasterDesc &other ) const
		{
			return memcmp( this, &other, sizeof( RasterDesc ) - sizeof( m_pD3DState ) ) == 0;
		}
	};

	struct ConstantBufferDesc
	{
		int m_MaxSlot;
		ID3D11Buffer *m_ppBuffers[MAX_DX11_CBUFFERS];

		bool operator==( const ConstantBufferDesc &other ) const
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
		BlendDesc blend;
		DepthStencilDesc depthStencil;
		RasterDesc rasterizer;

		ConstantBufferDesc vsConstantBuffers;
		ConstantBufferDesc gsConstantBuffers;
		ConstantBufferDesc psConstantBuffers;

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

		// Set this to something other than SHADER_FOGMODE_DISABLED
		// to override the scene fog color
		ShaderFogMode_t fogMode;
		bool disableFogGammaCorrection;

		bool Equals( const ShadowStateDesc &other ) const
		{
			if ( vertexShader != other.vertexShader					||
				staticVertexShaderIndex != other.staticVertexShaderIndex	||
				pixelShader != other.pixelShader				||
				staticPixelShaderIndex != other.staticPixelShaderIndex		||
				geometryShader != other.geometryShader				||
				staticGeometryShaderIndex != other.staticGeometryShaderIndex	||
				vertexFormat != other.vertexFormat				||
				morphFormat != other.morphFormat				||
				fogMode != other.fogMode					||
				disableFogGammaCorrection != other.disableFogGammaCorrection )
			{
				return false;
			}

			return ( blend == other.blend &&
					depthStencil == other.depthStencil &&
					rasterizer == other.rasterizer &&
					vsConstantBuffers == other.vsConstantBuffers &&
					gsConstantBuffers == other.gsConstantBuffers &&
					psConstantBuffers == other.psConstantBuffers );
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

			vertexShader = -1;
			staticVertexShaderIndex = 0;
			pixelShader = -1;
			staticPixelShaderIndex = 0;
			geometryShader = -1;
			staticGeometryShaderIndex = 0;

			vertexFormat = VERTEX_FORMAT_UNKNOWN;
			morphFormat = 0;

			fogMode = SHADER_FOGMODE_FOGCOLOR;
		}

		ShadowStateDesc()
		{
			SetDefault();
		}
	};

	struct ShadowState
	{
		ShadowStateDesc desc;

		unsigned int m_iVSConstantBufferState;
		unsigned int m_iPSConstantBufferState;
		unsigned int m_iGSConstantBufferState;

		unsigned int m_iBlendState;
		unsigned int m_iDepthStencilState;
		unsigned int m_iRasterState;

		bool operator==( const ShadowState &other ) const
		{
			return desc.Equals( other.desc );
		}

		ShadowState()
		{
			m_iBlendState = 0;
			m_iDepthStencilState = 0;
			m_iRasterState = 0;
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

		int m_MaxVertexBuffer;
		int m_PrevMaxVertexBuffer;
		ID3D11Buffer *m_pVertexBuffer[MAX_DX11_STREAMS];
		UINT m_pVBStrides[MAX_DX11_STREAMS];
		UINT m_pVBOffsets[MAX_DX11_STREAMS];

		D3D11_PRIMITIVE_TOPOLOGY m_Topology;

		ID3D11VertexShader *m_pVertexShader;
		ID3D11GeometryShader *m_pGeometryShader;
		ID3D11PixelShader *m_pPixelShader;

		ID3D11RenderTargetView *m_pRenderTargetView;
		ID3D11DepthStencilView *m_pDepthStencilView;

		int m_MaxVSSampler;
		int m_MaxPSSampler;
		int m_PrevMaxVSSampler;
		int m_PrevMaxPSSampler;
		ID3D11ShaderResourceView *m_ppVertexTextures[MAX_DX11_SAMPLERS];
		ID3D11SamplerState *m_ppVertexSamplers[MAX_DX11_SAMPLERS];
		ID3D11ShaderResourceView *m_ppTextures[MAX_DX11_SAMPLERS];
		ID3D11SamplerState *m_ppSamplers[MAX_DX11_SAMPLERS];

		void Reset()
		{
			ZeroMemory( this, sizeof( DynamicState ) );
			// We always need viewports
			m_nViewportCount = 1;
			m_pViewports[0].Width = 640;
			m_pViewports[0].Height = 480;
			m_pViewports[0].MinDepth = 0;
			m_pViewports[0].MaxDepth = 1;
			m_MaxPSSampler = -1;
			m_MaxVSSampler = -1;
			m_PrevMaxPSSampler = -1;
			m_PrevMaxVSSampler = -1;
			m_MaxVertexBuffer = -1;
			m_PrevMaxVertexBuffer = -1;
			m_Topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}

		DynamicState()
		{
			Reset();
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

		Vector4D m_ToneMappingScale;
		bool m_bToneMappingScaleChanged;

		CUtlStack<MatrixItem_t> m_MatrixStacks[NUM_MATRIX_MODES];
		bool m_ChangedMatrices[NUM_MATRIX_MODES];

		void SetDefault()
		{
			ZeroMemory( this, sizeof( ShaderState ) );

			m_ToneMappingScale.Init( 1, 1, 1, 1 );
			m_bToneMappingScaleChanged = true;

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