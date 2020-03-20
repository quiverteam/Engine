//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "shader_register_map.h"
#include "vertexlitgeneric_dx11_helper.h"

#include "vertexlit_and_unlit_generic_vs40.inc"
#include "decalmodulate_ps40.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( DecalModulate, DecalModulate_DX11 )

//extern ConVar r_flashlight_version2;

BEGIN_VS_SHADER( DecalModulate_dx11,
		 "Help for DecalModulate_dx11" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_FALLBACK
{
	return 0;
}

DECLARE_CONSTANT_BUFFER( VertexLitGeneric )

SHADER_INIT_PARAMS()
{
	SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );

#ifndef _X360
	if ( g_pHardwareConfig->HasFastVertexTextures() )
	{
		// The vertex shader uses the vertex id stream
		SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	}
#endif
}

SHADER_INIT
{
	LoadTexture( BASETEXTURE );
}

SHADER_INIT_GLOBAL
{
	INIT_CONSTANT_BUFFER( VertexLitGeneric );
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		pShaderShadow->EnableAlphaTest( true );
		pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 0.0f );
		pShaderShadow->EnableDepthWrites( false );
		pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL );
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

		SetInternalVertexShaderConstantBuffers();
		SetVertexShaderConstantBuffer( USER_CBUFFER_REG_0, CONSTANT_BUFFER( VertexLitGeneric ) );

		SetPixelShaderConstantBuffer( 0, SHADER_CONSTANTBUFFER_PERSCENE );
		SetPixelShaderConstantBuffer( 1, SHADER_CONSTANTBUFFER_PERFRAME );

		// Be sure not to write to dest alpha
		pShaderShadow->EnableAlphaWrites( false );

		pShaderShadow->EnableBlending( true );
		pShaderShadow->BlendFunc( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );
		pShaderShadow->DisableFogGammaCorrection( true ); //fog should stay exactly middle grey
		FogToGrey();

		DECLARE_STATIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs40 );
		SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR,  false );
		SET_STATIC_VERTEX_SHADER_COMBO( CUBEMAP,  false );
		SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT,  false );
		SET_STATIC_VERTEX_SHADER_COMBO( SEAMLESS_BASE,  false );
		SET_STATIC_VERTEX_SHADER_COMBO( SEAMLESS_DETAIL,  false );
		SET_STATIC_VERTEX_SHADER_COMBO( SEPARATE_DETAIL_UVS, false );
		SET_STATIC_VERTEX_SHADER_COMBO( DECAL, true );
		SET_STATIC_VERTEX_SHADER_COMBO( BUMPMAP, false );
		SET_STATIC_VERTEX_SHADER_COMBO( WRINKLEMAP, false );
		SET_STATIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs40 );

		DECLARE_STATIC_PIXEL_SHADER( decalmodulate_ps40 );
		SET_STATIC_PIXEL_SHADER( decalmodulate_ps40 );

		// Set stream format (note that this shader supports compression)
		unsigned int flags = VERTEX_POSITION | VERTEX_FORMAT_COMPRESSED;
#ifndef _X360
			// The VS30 shader offsets decals along the normal (for morphed geom)
			flags |= g_pHardwareConfig->HasFastVertexTextures() ? VERTEX_NORMAL : 0;
#endif
			int pTexCoordDim[3] = { 2, 0, 3 };
			int nTexCoordCount = 1;
			int userDataSize = 0;

#ifndef _X360
			if ( g_pHardwareConfig->HasFastVertexTextures() )
			{
				nTexCoordCount = 3;
			}
#endif

			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, pTexCoordDim, userDataSize );
		}
		DYNAMIC_STATE
		{
			//if ( pShaderAPI->InFlashlightMode() && ( !IsX360() && ( r_flashlight_version2.GetInt() == 0 ) ) )
			//{
			//	// Don't draw anything for the flashlight pass
			//	Draw( false );
			//	return;
			//}

			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

			VertexLitGeneric_CBuffer_t constants;
			memset( &constants, 0, sizeof( VertexLitGeneric_CBuffer_t ) );
			constants.cBaseTextureTransform[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
			constants.cBaseTextureTransform[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
			SetHWMorphVertexShaderState( constants.cMorphDimensions, constants.cMorphSubrect, SHADER_VERTEXTEXTURE_SAMPLER0 );
			UPDATE_CONSTANT_BUFFER( VertexLitGeneric, constants );

			MaterialFogMode_t fogType = s_pShaderAPI->GetSceneFogMode();
			int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;

			DECLARE_DYNAMIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs40 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DYNAMIC_LIGHT, 0 );	// Use simplest possible vertex lighting, since ps is so simple
			SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT, 0 );		//
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, fogIndex );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( LIGHTING_PREVIEW, 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING, pShaderAPI->IsHWMorphingEnabled() );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( FLASHLIGHT, false );
			SET_DYNAMIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs40 );

			DECLARE_DYNAMIC_PIXEL_SHADER( decalmodulate_ps40 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
			SET_DYNAMIC_PIXEL_SHADER( decalmodulate_ps40 );

			bool bUnusedTexCoords[3] = { false, false, !pShaderAPI->IsHWMorphingEnabled() };
			pShaderAPI->MarkUnusedVertexFields( 0, 3, bUnusedTexCoords );
		}
		Draw();
}
END_SHADER
