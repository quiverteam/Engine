#include "BaseVSShader.h"

#include "unlitgeneric_ps40.inc"
#include "unlitgeneric_vs40.inc"

BEGIN_VS_SHADER(UnlitGeneric, "Unlit shader")
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ALBEDO, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "albedo (Base texture with no baked lighting)" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if ( params[ALBEDO]->IsDefined() )
			LoadTexture( ALBEDO );
		if ( params[BASETEXTURE]->IsDefined() )
			LoadTexture( BASETEXTURE );
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
			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0 );

			DECLARE_STATIC_VERTEX_SHADER( unlitgeneric_vs40 );
			SET_STATIC_VERTEX_SHADER( unlitgeneric_vs40 );

			DECLARE_STATIC_PIXEL_SHADER( unlitgeneric_ps40 );
			SET_STATIC_PIXEL_SHADER( unlitgeneric_ps40 );
		}
		DYNAMIC_STATE
		{
			if ( params[BASETEXTURE]->IsTexture() )
				BindTexture( SHADER_SAMPLER0, BASETEXTURE );
			BindVertexShaderConstantBuffer(
				GetInternalConstantBuffer( SHADER_INTERNAL_CONSTANTBUFFER_TRANSFORM ) );

			DECLARE_DYNAMIC_VERTEX_SHADER( unlitgeneric_vs40 );
			SET_DYNAMIC_VERTEX_SHADER( unlitgeneric_vs40 );

			DECLARE_DYNAMIC_PIXEL_SHADER( unlitgeneric_ps40 );
			SET_DYNAMIC_PIXEL_SHADER( unlitgeneric_ps40 );
		}

		Draw();
	}

END_SHADER