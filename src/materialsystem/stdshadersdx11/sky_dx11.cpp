//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//
#include "BaseVSShader.h"
#include "sky_vs40.inc"
#include "sky_ps40.inc"

#include "ConVar.h"

CREATE_CONSTANT_BUFFER( Sky )
{
	// Vertex shader
	Vector4D vTextureSizeInfo;
	Vector4D mBaseTexCoordTransform[2];

	// Pixel shadere
	Vector4D vInputScale;
};

BEGIN_VS_SHADER( Sky_DX11, "Help for Sky_DX11 shader" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( COLOR, SHADER_PARAM_TYPE_VEC3, "[ 1 1 1]", "color multiplier", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( ALPHA, SHADER_PARAM_TYPE_FLOAT, "1.0", "unused", SHADER_PARAM_NOT_EDITABLE )
	END_SHADER_PARAMS

	DECLARE_CONSTANT_BUFFER( Sky )

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS( MATERIAL_VAR_NOFOG );
		SET_FLAGS( MATERIAL_VAR_IGNOREZ );
	}
	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );
		}
	}
	SHADER_INIT_GLOBAL
	{
		INIT_CONSTANT_BUFFER( Sky );
	}
	SHADER_DRAW
	{
		SHADOW_STATE
		{
			SetInitialShadowState();

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, NULL, 0 );

			DECLARE_STATIC_VERTEX_SHADER( sky_vs40 );
			SET_STATIC_VERTEX_SHADER( sky_vs40 );

			DECLARE_STATIC_PIXEL_SHADER( sky_ps40 );
			SET_STATIC_PIXEL_SHADER( sky_ps40 );

			pShaderShadow->EnableAlphaWrites( true );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

			ALIGN16 Sky_CBuffer_t constants;
			memset( &constants, 0, sizeof( Sky_CBuffer_t ) );

			// Texture coord transform
			StoreVertexShaderTextureTransform( constants.mBaseTexCoordTransform, BASETEXTURETRANSFORM );

			if (params[COLOR]->IsDefined())
			{
				params[COLOR]->GetVecValue( constants.vInputScale.Base(), 3 );
			}
			ITexture *txtr = params[BASETEXTURE]->GetTextureValue();
			ImageFormat fmt = txtr->GetImageFormat();
			if (
				( fmt == IMAGE_FORMAT_RGBA16161616 ) ||
				( ( fmt == IMAGE_FORMAT_RGBA16161616F ) &&
				( g_pHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER ) )
				)
			{
				constants.vInputScale[0]*=16.0;
				constants.vInputScale[1]*=16.0;
				constants.vInputScale[2]*=16.0;
			}

			UPDATE_CONSTANT_BUFFER( Sky, constants );

			BindVertexShaderConstantBuffer( 0, SHADER_CONSTANTBUFFER_PERMODEL );
			BindVertexShaderConstantBuffer( 1, SHADER_CONSTANTBUFFER_PERFRAME );
			BindVertexShaderConstantBuffer( 2, SHADER_CONSTANTBUFFER_PERSCENE );
			BindVertexShaderConstantBuffer( 3, CONSTANT_BUFFER( Sky ) );

			BindPixelShaderConstantBuffer( 0, CONSTANT_BUFFER( Sky ) );

			DECLARE_DYNAMIC_VERTEX_SHADER( sky_vs40 );
			SET_DYNAMIC_VERTEX_SHADER( sky_vs40 );			

			DECLARE_DYNAMIC_PIXEL_SHADER( sky_ps40 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, pShaderAPI->ShouldWriteDepthToDestAlpha() );
			SET_DYNAMIC_PIXEL_SHADER( sky_ps40 );
		}
		Draw( );
	}

END_SHADER

