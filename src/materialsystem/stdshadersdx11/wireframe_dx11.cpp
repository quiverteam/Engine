//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#if 1
#include "shaderlib/cshader.h"

#include "wireframe_vs40.inc"
#include "wireframe_ps40.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef _WIN32
DEFINE_FALLBACK_SHADER(Wireframe, Wireframe_DX11)
BEGIN_SHADER( Wireframe_DX11,
	      "Help for Wireframe_DX11" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_INIT
	{
		if ( params[BASETEXTURE]->IsDefined() )
			LoadTexture( BASETEXTURE );
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->PolyMode( SHADER_POLYMODEFACE_FRONT_AND_BACK, SHADER_POLYMODE_LINE );
			pShaderShadow->EnableCulling( false );

			VertexFormat_t format = VERTEX_POSITION | VERTEX_COLOR;
			int dimensions[] = { 2 };
			pShaderShadow->VertexShaderVertexFormat( format, 1, dimensions, 0 );

			SetInternalVertexShaderConstantBuffersNoSkinning();

			DECLARE_STATIC_VERTEX_SHADER( wireframe_vs40 );
			SET_STATIC_VERTEX_SHADER( wireframe_vs40 );

			DECLARE_STATIC_PIXEL_SHADER( wireframe_ps40 );
			SET_STATIC_PIXEL_SHADER( wireframe_ps40 );
		}
		DYNAMIC_STATE
		{
			
			DECLARE_DYNAMIC_VERTEX_SHADER( wireframe_vs40 );
			SET_DYNAMIC_VERTEX_SHADER( wireframe_vs40 );

			DECLARE_DYNAMIC_PIXEL_SHADER( wireframe_ps40 );
			SET_DYNAMIC_PIXEL_SHADER( wireframe_ps40 );
		}
		Draw();
	}
END_SHADER
#endif

#endif