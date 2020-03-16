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
#include "sky_hdr_compressed_ps40.inc"
#include "sky_hdr_compressed_rgbs_ps40.inc"

CREATE_CONSTANT_BUFFER( Sky_HDR )
{
	// Vertex shader
	Vector4D vTextureSizeInfo;
	Vector4D mBaseTexCoordTransform[2];

	// Pixel shadere
	Vector4D vInputScale;
};

#include "ConVar.h"

static ConVar mat_use_compressed_hdr_textures( "mat_use_compressed_hdr_textures", "1" );

DEFINE_FALLBACK_SHADER( Sky, Sky_HDR_DX11 )

BEGIN_VS_SHADER( Sky_HDR_DX11, "Help for Sky_HDR_DX11 shader" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( HDRBASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "base texture when running with HDR enabled" )
		SHADER_PARAM( HDRCOMPRESSEDTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "base texture (compressed) for hdr compression method A" )
		SHADER_PARAM( HDRCOMPRESSEDTEXTURE0, SHADER_PARAM_TYPE_TEXTURE, "", "compressed base texture0 for hdr compression method B" )
		SHADER_PARAM( HDRCOMPRESSEDTEXTURE1, SHADER_PARAM_TYPE_TEXTURE, "", "compressed base texture1 for hdr compression method B" )
		SHADER_PARAM( HDRCOMPRESSEDTEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "", "compressed base texture2 for hdr compression method B" )
		SHADER_PARAM_OVERRIDE( COLOR, SHADER_PARAM_TYPE_VEC3, "[ 1 1 1]", "color multiplier", SHADER_PARAM_NOT_EDITABLE )
	END_SHADER_PARAMS

	DECLARE_CONSTANT_BUFFER( Sky_HDR )

	SHADER_INIT_GLOBAL
	{
		INIT_CONSTANT_BUFFER( Sky_HDR );
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
		{
			return "Sky_DX11";
		}
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS( MATERIAL_VAR_NOFOG );
		SET_FLAGS( MATERIAL_VAR_IGNOREZ );
	}
	SHADER_INIT
	{
		if (params[HDRCOMPRESSEDTEXTURE]->IsDefined() && (mat_use_compressed_hdr_textures.GetBool() ) )
		{
			LoadTexture( HDRCOMPRESSEDTEXTURE );
		}
		else
		{
			if (params[HDRCOMPRESSEDTEXTURE0]->IsDefined())
			{
				LoadTexture( HDRCOMPRESSEDTEXTURE0 );
				if (params[HDRCOMPRESSEDTEXTURE1]->IsDefined())
				{
					LoadTexture( HDRCOMPRESSEDTEXTURE1 );
				}
				if (params[HDRCOMPRESSEDTEXTURE2]->IsDefined())
				{
					LoadTexture( HDRCOMPRESSEDTEXTURE2 );
				}
			}
			else
			{
				if (params[HDRBASETEXTURE]->IsDefined())
				{
					LoadTexture( HDRBASETEXTURE );
				}
			}
		}
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

			if ( (params[HDRCOMPRESSEDTEXTURE]->IsDefined()) &&
				 mat_use_compressed_hdr_textures.GetBool() )
			{
				DECLARE_STATIC_PIXEL_SHADER( sky_hdr_compressed_rgbs_ps40 );
				SET_STATIC_PIXEL_SHADER( sky_hdr_compressed_rgbs_ps40 );
			}
			else
			{
				if (params[HDRCOMPRESSEDTEXTURE0]->IsDefined())
				{
					pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
					pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
					
					DECLARE_STATIC_PIXEL_SHADER( sky_hdr_compressed_ps40 );
					SET_STATIC_PIXEL_SHADER( sky_hdr_compressed_ps40 );
				}
				else
				{			
					DECLARE_STATIC_PIXEL_SHADER( sky_ps40 );
					SET_STATIC_PIXEL_SHADER( sky_ps40 );
				}
			}

			pShaderShadow->EnableAlphaWrites( true );
		}

		DYNAMIC_STATE
		{
			DECLARE_DYNAMIC_VERTEX_SHADER( sky_vs40 );
			SET_DYNAMIC_VERTEX_SHADER( sky_vs40 );

			ALIGN16 Sky_HDR_CBuffer_t constants;
			memset( &constants, 0, sizeof( Sky_HDR_CBuffer_t ) );

			// Texture coord transform
			StoreVertexShaderTextureTransform( constants.mBaseTexCoordTransform, BASETEXTURETRANSFORM );

			BindVertexShaderConstantBuffer( 0,
							SHADER_CONSTANTBUFFER_PERMODEL );
			BindVertexShaderConstantBuffer( 1,
							SHADER_CONSTANTBUFFER_PERFRAME );
			BindVertexShaderConstantBuffer( 2,
							SHADER_CONSTANTBUFFER_PERSCENE );
			BindVertexShaderConstantBuffer( 3, CONSTANT_BUFFER( Sky_HDR ) );

			Vector4D vInputScale( 1, 1, 1, 1 );
			if ( params[COLOR]->IsDefined() )
			{
				params[COLOR]->GetVecValue( vInputScale.Base(), 3 );
			}
			if (
				params[HDRCOMPRESSEDTEXTURE]->IsDefined() &&
				mat_use_compressed_hdr_textures.GetBool()
				)
			{

				// set up data needs for pixel shader interpolation
				ITexture *txtr=params[HDRCOMPRESSEDTEXTURE]->GetTextureValue();
				float w = txtr->GetActualWidth();
				float h = txtr->GetActualHeight();
				float FUDGE = 0.01 / max( w, h );					// per ATI
				constants.vTextureSizeInfo.Init( 0.5 / w - FUDGE, 0.5 / h - FUDGE, w, h );

				BindTexture( SHADER_SAMPLER0, HDRCOMPRESSEDTEXTURE, FRAME );
				vInputScale[0]*=8.0;
				vInputScale[1]*=8.0;
				vInputScale[2]*=8.0;
				constants.vInputScale = vInputScale;

				BindPixelShaderConstantBuffer( 0, CONSTANT_BUFFER( Sky_HDR ) );

				DECLARE_DYNAMIC_PIXEL_SHADER( sky_hdr_compressed_rgbs_ps40 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, pShaderAPI->ShouldWriteDepthToDestAlpha() );
				SET_DYNAMIC_PIXEL_SHADER( sky_hdr_compressed_rgbs_ps40 );
			}
			else
			{

				if (params[HDRCOMPRESSEDTEXTURE0]->IsDefined() )
				{
					BindTexture( SHADER_SAMPLER0, HDRCOMPRESSEDTEXTURE0, FRAME );
					BindTexture( SHADER_SAMPLER1, HDRCOMPRESSEDTEXTURE1, FRAME );
					BindTexture( SHADER_SAMPLER2, HDRCOMPRESSEDTEXTURE2, FRAME );
					DECLARE_DYNAMIC_PIXEL_SHADER( sky_hdr_compressed_ps40 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, pShaderAPI->ShouldWriteDepthToDestAlpha() );
					SET_DYNAMIC_PIXEL_SHADER( sky_hdr_compressed_ps40 );

				}
				else
				{

					BindTexture( SHADER_SAMPLER0, HDRBASETEXTURE, FRAME );
					ITexture *txtr=params[HDRBASETEXTURE]->GetTextureValue();
					ImageFormat fmt=txtr->GetImageFormat();
					if (
						(fmt==IMAGE_FORMAT_RGBA16161616) ||
						( (fmt==IMAGE_FORMAT_RGBA16161616F) && 
						  (g_pHardwareConfig->GetHDRType()==HDR_TYPE_INTEGER))
						)
					{
						vInputScale[0]*=16.0;
						vInputScale[1]*=16.0;
						vInputScale[2]*=16.0;
					}

					constants.vInputScale = vInputScale;

					BindPixelShaderConstantBuffer( 0, CONSTANT_BUFFER( Sky_HDR ) );

					DECLARE_DYNAMIC_PIXEL_SHADER( sky_ps40 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, pShaderAPI->ShouldWriteDepthToDestAlpha() );
					SET_DYNAMIC_PIXEL_SHADER( sky_ps40 );
				}
			}

			UPDATE_CONSTANT_BUFFER( Sky_HDR, constants );
		}
		Draw( );
	}
END_SHADER
