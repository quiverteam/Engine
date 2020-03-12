
#if 1
#include "BaseVSShader.h"

#include "unlitgeneric_ps40.inc"
#include "unlitgeneric_vs40.inc"

BEGIN_VS_SHADER(UnlitGeneric, "Unlit shader")
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ALBEDO, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "albedo (Base texture with no baked lighting)" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		//if ( params[ALBEDO]->IsDefined() )
		//	LoadTexture( ALBEDO );
		if ( params[BASETEXTURE]->IsDefined() )
			LoadTexture( BASETEXTURE );
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			unsigned int flags = VERTEX_POSITION;
			int numTexCoords = 1;
			

			SetDefaultBlendingShadowState( BASETEXTURE );

			if ( IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) )
			{
				flags |= VERTEX_COLOR;
			}

			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0 );

			DECLARE_STATIC_VERTEX_SHADER( unlitgeneric_vs40 );
			SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) );
			SET_STATIC_VERTEX_SHADER( unlitgeneric_vs40 );

			DECLARE_STATIC_PIXEL_SHADER( unlitgeneric_ps40 );
			SET_STATIC_PIXEL_SHADER( unlitgeneric_ps40 );
		}
		DYNAMIC_STATE
		{
			if ( params[BASETEXTURE]->IsTexture() )
				BindTexture( SHADER_SAMPLER0, BASETEXTURE );
			BindInternalVertexShaderConstantBuffers();
			BindPixelShaderConstantBuffer(
				0,
				GetInternalConstantBuffer( SHADER_CONSTANTBUFFER_PERMATERIAL ) );

			float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			ComputeModulationColor( color );
			pShaderAPI->Color4f( color[0], color[1], color[2], color[3] );

			DECLARE_DYNAMIC_VERTEX_SHADER( unlitgeneric_vs40 );
			SET_DYNAMIC_VERTEX_SHADER( unlitgeneric_vs40 );

			DECLARE_DYNAMIC_PIXEL_SHADER( unlitgeneric_ps40 );
			SET_DYNAMIC_PIXEL_SHADER( unlitgeneric_ps40 );
		}

		Draw();
	}

END_SHADER
#endif
