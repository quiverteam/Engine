#pragma once

#include "shaderapi/ishaderdevice.h"
#include "shaderapi/ishadershadow.h"
#include "shaderapi/ishaderapi.h"
#include "materialsystem/imaterialsystem.h"

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
		int vertexShaderIndex;
		PixelShader_t pixelShader;
		int pixelShaderIndex;
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

		CShaderConstantBufferDx11 *m_ppVSConstantBuffers[MAX_DX11_CBUFFERS];
		int m_nVSConstantBuffers;
		CShaderConstantBufferDx11 *m_ppGSConstantBuffers[MAX_DX11_CBUFFERS];
		int m_nGSConstantBuffers;
		CShaderConstantBufferDx11 *m_ppPSConstantBuffers[MAX_DX11_CBUFFERS];
		int m_nPSConstantBuffers;
		
		ID3D11SamplerState *m_ppSamplers[MAX_DX11_SAMPLERS];
		int m_nSamplers;
		ID3D11ShaderResourceView *m_ppTextureViews[MAX_DX11_SAMPLERS];
		int m_nTextures;
		CTextureDx11 *m_ppTextures[MAX_DX11_SAMPLERS];

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
}