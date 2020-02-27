//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "shaderlib/cshader.h"

#include "wireframe_vs40.inc"
#include "wireframe_ps40.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef _WIN32
BEGIN_SHADER( Wireframe_DX11,
	      "Help for Wireframe_DX11" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->PolyMode( SHADER_POLYMODEFACE_FRONT_AND_BACK, SHADER_POLYMODE_LINE );

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
