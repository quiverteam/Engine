#pragma once

#include "shaderapi/ishaderdevice.h"
#include "shaderapi/ishadershadow.h"
#include "shaderapi/ishaderapi.h"
#include "materialsystem/imaterialsystem.h"

#define MAX_DX11_CBUFFERS	15
#define MAX_DX11_SAMPLERS	16

namespace StatesDx11
{
	// The shader state is not part of the shadow state.
	struct ShaderAttrib
	{
		ConstantBufferHandle_t constantBuffers[MAX_DX11_CBUFFERS];
		int numConstantBuffers;

		VertexShader_t vertexShader;
		PixelShader_t pixelShader;
		GeometryShader_t geometryShader;

		int vertexShaderIndex;
		int pixelShaderIndex;
		int geometryShaderIndex;

		// Vertex data used by this snapshot
		// Note that the vertex format actually used will be the
		// aggregate of the vertex formats used by all snapshots in a material
		VertexFormat_t vertexFormat;

		// Morph data used by this snapshot
		// Note that the morph format actually used will be the
		// aggregate of the morph formats used by all snapshots in a material
		MorphFormat_t morphFormat;

		ShaderAttrib()
		{
			memset( constantBuffers, 0, sizeof( ConstantBufferHandle_t ) * MAX_DX11_CBUFFERS );
			numConstantBuffers = 0;
			vertexShader = 0;
			pixelShader = 0;
			geometryShader = 0;
			vertexShaderIndex = -1;
			pixelShaderIndex = -1;
			geometryShaderIndex = -1;
			vertexFormat = 0;
			morphFormat = 0;
		}

		void AddConstantBuffer( ConstantBufferHandle_t hBuffer )
		{
			constantBuffers[numConstantBuffers++] = hBuffer;
		}
	};

	//
	// Below are states part of the shadow state.
	//

	struct SamplerAttrib
	{
		bool samplers[MAX_DX11_SAMPLERS];

		SamplerAttrib()
		{
			memset( samplers, 0, MAX_DX11_SAMPLERS );
		}
	};

	struct StencilAttrib
	{
		bool bStencilEnable;
		ShaderStencilOp_t stencilPassOp;
		ShaderStencilOp_t stencilFailOp;
		ShaderStencilOp_t stencilDepthFailOp;
		ShaderStencilFunc_t stencilFunc;
		uint8 stencilReadMask;
		uint8 stencilWriteMask;
		uint stencilRef;

		StencilAttrib()
		{
			bStencilEnable = false;
			stencilPassOp = SHADER_STENCILOP_KEEP;
			stencilFailOp = SHADER_STENCILOP_KEEP;
			stencilDepthFailOp = SHADER_STENCILOP_KEEP;
			stencilFunc = SHADER_STENCILFUNC_ALWAYS;
			stencilReadMask = 0;
			stencilWriteMask = 0;
			stencilRef = 0;
		}
	};

	struct DepthTestAttrib
	{
		bool bEnableDepthTest;
		ShaderDepthFunc_t depthFunc;

		DepthTestAttrib()
		{
			bEnableDepthTest = true;
			depthFunc = SHADER_DEPTHFUNC_NEARER;
		}
	};

	struct DepthOffsetAttrib
	{
		bool bEnable;
		int offset;

		DepthOffsetAttrib()
		{
			bEnable = false;
			offset = 0;
		}
	};

	struct DepthWriteAttrib
	{
		bool bEnableDepthWrite;

		DepthWriteAttrib()
		{
			bEnableDepthWrite = true;
		}
	};

	struct ColorBlendAttrib
	{
		bool bBlendEnable;

		ShaderBlendFactor_t srcBlend;
		ShaderBlendFactor_t destBlend;
		ShaderBlendOp_t blendOp;

		ShaderBlendFactor_t srcBlendAlpha;
		ShaderBlendFactor_t destBlendAlpha;
		ShaderBlendOp_t blendOpAlpha;

		float blendColor[4];

		uint sampleMask;

		ColorBlendAttrib()
		{
			bBlendEnable = false;
			srcBlend = SHADER_BLEND_ONE;
			destBlend = SHADER_BLEND_ZERO;
			blendOp = SHADER_BLENDOP_ADD;
			srcBlendAlpha = SHADER_BLEND_ONE;
			destBlendAlpha = SHADER_BLEND_ZERO;
			blendOpAlpha = SHADER_BLENDOP_ADD;
			memset( blendColor, 0, sizeof( float ) * 4 );
		}
	};

	struct ColorWriteAttrib
	{
		enum
		{
			COLORWRITE_OFF = 0,
			COLORWRITE_RED = 1 << 0,
			COLORWRITE_GREEN = 1 << 1,
			COLORWRITE_BLUE = 1 << 2,
			COLORWRITE_ALPHA = 1 << 3,

			COLORWRITE_RGB = COLORWRITE_RED | COLORWRITE_GREEN | COLORWRITE_BLUE,
			COLORWRITE_RGBA = COLORWRITE_RGB | COLORWRITE_ALPHA,
		};

		uint8 colorWriteMask;

		ColorWriteAttrib()
		{
			colorWriteMask = COLORWRITE_RGBA;
		}
	};

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

	struct AntialiasAttrib
	{
		bool bMultisampleEnable;

		AntialiasAttrib()
		{
			bMultisampleEnable = false;
		}
	};

	struct RenderModeAttrib
	{
		ShaderPolyMode_t polyMode;
		ShaderPolyModeFace_t faceMode;

		RenderModeAttrib()
		{
			polyMode = SHADER_POLYMODE_FILL;
			faceMode = SHADER_POLYMODEFACE_FRONT;
		}
	};

	struct CullFaceAttrib
	{
		MaterialCullMode_t cullMode;

		CullFaceAttrib()
		{
			cullMode = MATERIAL_CULLMODE_CCW;
		}
	};

	struct ScissorAttrib
	{
		bool bScissorEnable;

		ScissorAttrib()
		{
			bScissorEnable = false;
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

	struct ShadowRenderState
	{
		SamplerAttrib samplerAttrib;
		StencilAttrib stencilAttrib;
		AntialiasAttrib antialiasAttrib;
		CullFaceAttrib cullFaceAttrib;
		ScissorAttrib scissorAttrib;
		AlphaTestAttrib alphaTestAttrib;
		ColorWriteAttrib colorWriteAttrib;
		ColorBlendAttrib colorBlendAttrib;
		DepthTestAttrib depthTestAttrib;
		DepthWriteAttrib depthWriteAttrib;
		DepthOffsetAttrib depthOffsetAttrib;
		RenderModeAttrib renderModeAttrib;
		TransparencyAttrib transparencyAttrib;
		ColorAttrib colorAttrib;
		LightAttrib lightAttrib;
		VertexBlendAttrib vertexBlendAttrib;

		ShadowRenderState()
		{
		}
	};

	struct ShadowShaderState
	{
		ShaderAttrib shaderAttrib;

		ShadowShaderState()
		{
		}
	};

	struct RenderState
	{
		ShadowRenderState shadowState;
		ShadowShaderState shaderState;

		RenderState()
		{
		}
	};
}