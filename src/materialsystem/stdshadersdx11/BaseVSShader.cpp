//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
// This is what all vs/ps (dx8+) shaders inherit from.
//===========================================================================//
#if !defined(_STATIC_LINKED) || defined(STDSHADER_DX9_DLL_EXPORT)

#include "basevsshader.h"
#include "mathlib/vmatrix.h"
#include "mathlib/bumpvects.h"
#include "ConVar.h"

// what is this
#ifdef HDR
#include "vertexlit_and_unlit_generic_hdr_ps20.inc"
#include "vertexlit_and_unlit_generic_hdr_ps20b.inc"
#endif

//#include "lightmappedgeneric_flashlight_vs30.inc"
//#include "vertexlitgeneric_flashlight_vs30.inc"
//#include "flashlight_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar mat_fullbright( "mat_fullbright","0", FCVAR_CHEAT );

// These functions are to be called from the shaders.

//-----------------------------------------------------------------------------
// Helper methods for pixel shader overbrighting
//-----------------------------------------------------------------------------
void CBaseVSShader::EnablePixelShaderOverbright( int reg, bool bEnable, bool bDivideByTwo )
{
	// can't have other overbright values with pixel shaders as it stands.
	float v[4];
	if( bEnable )
	{
		v[0] = v[1] = v[2] = v[3] = bDivideByTwo ? OVERBRIGHT / 2.0f : OVERBRIGHT;
	}
	else
	{
		v[0] = v[1] = v[2] = v[3] = bDivideByTwo ? 1.0f / 2.0f : 1.0f;
	}
	s_pShaderAPI->SetPixelShaderConstant( reg, v, 1 );
}


//-----------------------------------------------------------------------------
// Helper for dealing with modulation
//-----------------------------------------------------------------------------
void CBaseVSShader::SetModulationDynamicState( Vector4D &output )
{
	ComputeModulationColor( output.Base() );
}

void CBaseVSShader::SetModulationDynamicState_LinearColorSpace( Vector4D &output )
{
	float color[4] = { 1.0, 1.0, 1.0, 1.0 };
	ComputeModulationColor( color );
	color[0] = color[0] > 1.0f ? color[0] : GammaToLinear( color[0] );
	color[1] = color[1] > 1.0f ? color[1] : GammaToLinear( color[1] );
	color[2] = color[2] > 1.0f ? color[2] : GammaToLinear( color[2] );

	output.Init( color[0], color[1], color[2], color[3] );
}

void CBaseVSShader::SetModulationDynamicState_LinearColorSpace_LinearScale( Vector4D &output, float flScale )
{
	float color[4] = { 1.0, 1.0, 1.0, 1.0 };
	ComputeModulationColor( color );
	color[0] = ( color[0] > 1.0f ? color[0] : GammaToLinear( color[0] ) ) * flScale;
	color[1] = ( color[1] > 1.0f ? color[1] : GammaToLinear( color[1] ) ) * flScale;
	color[2] = ( color[2] > 1.0f ? color[2] : GammaToLinear( color[2] ) ) * flScale;

	output.Init( color[0], color[1], color[2], color[3] );
}


//-----------------------------------------------------------------------------
// Converts a color + alpha into a vector4
//-----------------------------------------------------------------------------
void CBaseVSShader::ColorVarsToVector( int colorVar, int alphaVar, Vector4D &color )
{
	color.Init( 1.0, 1.0, 1.0, 1.0 ); 
	if ( colorVar != -1 )
	{
		IMaterialVar* pColorVar = s_ppParams[colorVar];
		if ( pColorVar->GetType() == MATERIAL_VAR_TYPE_VECTOR )
		{
			pColorVar->GetVecValue( color.Base(), 3 );
		}
		else
		{
			color[0] = color[1] = color[2] = pColorVar->GetFloatValue();
		}
	}
	if ( alphaVar != -1 )
	{
		float flAlpha = s_ppParams[alphaVar]->GetFloatValue();
		color[3] = clamp( flAlpha, 0.0f, 1.0f );
	}
}

#ifdef _DEBUG
ConVar mat_envmaptintoverride( "mat_envmaptintoverride", "-1" );
ConVar mat_envmaptintscale( "mat_envmaptintscale", "-1" );
#endif

//-----------------------------------------------------------------------------
// Helpers for dealing with envmap tint
//-----------------------------------------------------------------------------
// set alphaVar to -1 to ignore it.
void CBaseVSShader::SetEnvMapTintPixelShaderDynamicState( int pixelReg, int tintVar, int alphaVar, bool bConvertFromGammaToLinear )
{
	float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	if( g_pConfig->bShowSpecular && mat_fullbright.GetInt() != 2 )
	{
		IMaterialVar* pAlphaVar = NULL;
		if( alphaVar >= 0 )
		{
			pAlphaVar = s_ppParams[alphaVar];
		}
		if( pAlphaVar )
		{
			color[3] = pAlphaVar->GetFloatValue();
		}

		IMaterialVar* pTintVar = s_ppParams[tintVar];
#ifdef _DEBUG
		pTintVar->GetVecValue( color, 3 );

		float envmapTintOverride = mat_envmaptintoverride.GetFloat();
		float envmapTintScaleOverride = mat_envmaptintscale.GetFloat();

		if( envmapTintOverride != -1.0f )
		{
			color[0] = color[1] = color[2] = envmapTintOverride;
		}
		if( envmapTintScaleOverride != -1.0f )
		{
			color[0] *= envmapTintScaleOverride;
			color[1] *= envmapTintScaleOverride;
			color[2] *= envmapTintScaleOverride;
		}

		if( bConvertFromGammaToLinear )
		{
			color[0] = color[0] > 1.0f ? color[0] : GammaToLinear( color[0] );
			color[1] = color[1] > 1.0f ? color[1] : GammaToLinear( color[1] );
			color[2] = color[2] > 1.0f ? color[2] : GammaToLinear( color[2] );
		}
#else
		if( bConvertFromGammaToLinear )
		{
			pTintVar->GetLinearVecValue( color, 3 );
		}
		else
		{
			pTintVar->GetVecValue( color, 3 );
		}
#endif
	}
	else
	{
		color[0] = color[1] = color[2] = color[3] = 0.0f;
	}
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, color, 1 );
}

void CBaseVSShader::SetAmbientCubeDynamicStateVertexShader( )
{
	s_pShaderAPI->SetVertexShaderStateAmbientLightCube();
}

float CBaseVSShader::GetAmbientLightCubeLuminance( )
{
	return s_pShaderAPI->GetAmbientLightCubeLuminance();
}

//-----------------------------------------------------------------------------
// Sets up hw morphing state for the vertex shader
//-----------------------------------------------------------------------------
void CBaseVSShader::SetHWMorphVertexShaderState( Vector4D &dimensions, Vector4D &subrect, VertexTextureSampler_t morphSampler )
{
	if ( !s_pShaderAPI->IsHWMorphingEnabled() )
		return;

	int nMorphWidth, nMorphHeight;
	s_pShaderAPI->GetStandardTextureDimensions( &nMorphWidth, &nMorphHeight, TEXTURE_MORPH_ACCUMULATOR );

	int nDim = s_pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_MORPH_ACCUMULATOR_4TUPLE_COUNT );
	float pMorphAccumSize[4] = { nMorphWidth, nMorphHeight, nDim, 0.0f };
	dimensions.Init( pMorphAccumSize[0], pMorphAccumSize[1], pMorphAccumSize[2], pMorphAccumSize[3] );

	int nXOffset = s_pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_MORPH_ACCUMULATOR_X_OFFSET );
	int nYOffset = s_pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_MORPH_ACCUMULATOR_Y_OFFSET );
	int nWidth = s_pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_MORPH_ACCUMULATOR_SUBRECT_WIDTH );
	int nHeight = s_pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_MORPH_ACCUMULATOR_SUBRECT_HEIGHT );
	float pMorphAccumSubrect[4] = { nXOffset, nYOffset, nWidth, nHeight };
	subrect.Init( pMorphAccumSubrect[0], pMorphAccumSubrect[1], pMorphAccumSubrect[2], pMorphAccumSubrect[3] );

	s_pShaderAPI->BindStandardVertexTexture( morphSampler, TEXTURE_MORPH_ACCUMULATOR );
}

//-----------------------------------------------------------------------------
// GR - translucency query
//-----------------------------------------------------------------------------
BlendType_t CBaseVSShader::EvaluateBlendRequirements( int textureVar, bool isBaseTexture,
													  int detailTextureVar )
{
	// Either we've got a constant modulation
	bool isTranslucent = IsAlphaModulating();

	// Or we've got a vertex alpha
	isTranslucent = isTranslucent || (CurrentMaterialVarFlags() & MATERIAL_VAR_VERTEXALPHA);

	// Or we've got a texture alpha (for blending or alpha test)
	isTranslucent = isTranslucent || ( TextureIsTranslucent( textureVar, isBaseTexture ) &&
		                               !(CurrentMaterialVarFlags() & MATERIAL_VAR_ALPHATEST ) );

	if ( ( detailTextureVar != -1 ) && ( ! isTranslucent ) )
	{
		isTranslucent = TextureIsTranslucent( detailTextureVar, isBaseTexture );
	}

	if ( CurrentMaterialVarFlags() & MATERIAL_VAR_ADDITIVE )
	{	
		return isTranslucent ? BT_BLENDADD : BT_ADD;	// Additive
	}
	else
	{
		return isTranslucent ? BT_BLEND : BT_NONE;		// Normal blending
	}
}

#if 0//#ifdef STDSHADER_DX11_DLL_EXPORT
void CBaseVSShader::DrawFlashlight_dx90( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, 
										IShaderShadow* pShaderShadow, DrawFlashlight_dx90_Vars_t &vars )
{
	// FLASHLIGHTFIXME: hack . . need to fix the vertex shader so that it can deal with and without bumps for vertexlitgeneric
	if( !vars.m_bLightmappedGeneric )
	{
		vars.m_bBump = false;
	}
	bool bBump2 = vars.m_bWorldVertexTransition && vars.m_bBump && vars.m_nBumpmap2Var != -1 && params[vars.m_nBumpmap2Var]->IsTexture();
	bool bSeamless = vars.m_fSeamlessScale != 0.0;
	bool bDetail = vars.m_bLightmappedGeneric && (vars.m_nDetailVar != -1) && params[vars.m_nDetailVar]->IsDefined() && (vars.m_nDetailScale != -1);

	int nDetailBlendMode = 0;
	if ( bDetail )
	{
		nDetailBlendMode = GetIntParam( vars.m_nDetailTextureCombineMode, params );
		nDetailBlendMode = nDetailBlendMode > 1 ? 1 : nDetailBlendMode;
	}

	if( pShaderShadow )
	{
		SetInitialShadowState();
		pShaderShadow->EnableDepthWrites( false );
		pShaderShadow->EnableAlphaWrites( false );

		// Alpha blend
		SetAdditiveBlendingShadowState( BASETEXTURE, true );

		// Alpha test
		pShaderShadow->EnableAlphaTest( IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) );
		if ( vars.m_nAlphaTestReference != -1 && params[vars.m_nAlphaTestReference]->GetFloatValue() > 0.0f )
		{
			pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GEQUAL, params[vars.m_nAlphaTestReference]->GetFloatValue() );
		}

		// Spot sampler
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );

		// Base sampler
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );

		// Normalizing cubemap sampler
		pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );

		// Normalizing cubemap sampler2 or normal map sampler
		pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );

		// RandomRotation sampler
		pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );

		// Flashlight depth sampler
		pShaderShadow->EnableTexture( SHADER_SAMPLER7, true );
		pShaderShadow->SetShadowDepthFiltering( SHADER_SAMPLER7 );

		if( vars.m_bWorldVertexTransition )
		{
			// $basetexture2
			pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER4, true );
		}
		if( bBump2 )
		{
			// Normalmap2 sampler
			pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );
		}
		if( bDetail )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER8, true );				// detail sampler
			if ( nDetailBlendMode != 0 ) //Not Mod2X
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER8, true );
		}
		
		pShaderShadow->EnableSRGBWrite( true );

		if( vars.m_bLightmappedGeneric )
		{
			lightmappedgeneric_flashlight_vs30_Static_Index	vshIndex;
			vshIndex.SetWORLDVERTEXTRANSITION( vars.m_bWorldVertexTransition );
			vshIndex.SetNORMALMAP( vars.m_bBump );
			vshIndex.SetSEAMLESS( bSeamless );
			vshIndex.SetDETAIL( bDetail );
			pShaderShadow->SetVertexShader( "lightmappedgeneric_flashlight_vs30", vshIndex.GetIndex() );

			unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;
			if( vars.m_bBump )
			{
				flags |= VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			}
			int numTexCoords = 1;
			if( vars.m_bWorldVertexTransition )
			{
				flags |= VERTEX_COLOR;
				numTexCoords = 2; // need lightmap texcoords to get alpha.
			}
			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0 );
		}
		else
		{
			/*vertexlitgeneric_flashlight_vs30_Static_Index vshIndex;
			vshIndex.SetTEETH( vars.m_bTeeth );
			pShaderShadow->SetVertexShader( "vertexlitgeneric_flashlight_vs30", vshIndex.GetIndex() );*/

			DECLARE_DYNAMIC_VERTEX_SHADER( vertexlitgeneric_flashlight_vs30 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			SET_DYNAMIC_VERTEX_SHADER( vertexlitgeneric_flashlight_vs30 );

			unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;
			int numTexCoords = 1;
			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, vars.m_bBump ? 4 : 0 );
		}

		int nBumpMapVariant = 0;
		if ( vars.m_bBump )
		{
			nBumpMapVariant = ( vars.m_bSSBump ) ? 2 : 1;
		}
		
		// disabled at the moment, need to make this a convar anyway
		int nShadowFilterMode = g_pHardwareConfig->GetShadowFilterMode();

		flashlight_ps30_Static_Index	pshIndex;
		pshIndex.SetNORMALMAP( nBumpMapVariant );
		pshIndex.SetNORMALMAP2( bBump2 );
		pshIndex.SetWORLDVERTEXTRANSITION( vars.m_bWorldVertexTransition );
		pshIndex.SetSEAMLESS( bSeamless );
		pshIndex.SetDETAILTEXTURE( bDetail );
		pshIndex.SetDETAIL_BLEND_MODE( nDetailBlendMode );
		pshIndex.SetFLASHLIGHTDEPTHFILTERMODE( nShadowFilterMode );
		pShaderShadow->SetPixelShader( "flashlight_ps30", pshIndex.GetIndex() );
		
		FogToBlack();
	}
	else
	{
		VMatrix worldToTexture;
		ITexture *pFlashlightDepthTexture;
		FlashlightState_t flashlightState = pShaderAPI->GetFlashlightStateEx( worldToTexture, &pFlashlightDepthTexture );

		if ( pFlashlightDepthTexture == NULL )
		{
			const int iFlashlightShadowIndex = ( flashlightState.m_nShadowQuality >> 16 ) - 1;

			if ( iFlashlightShadowIndex >= 0
				&& iFlashlightShadowIndex <= ( INT_FLASHLIGHT_DEPTHTEXTURE_FALLBACK_LAST - INT_FLASHLIGHT_DEPTHTEXTURE_FALLBACK_FIRST ) )
			{
				pFlashlightDepthTexture = (ITexture*)pShaderAPI->GetIntRenderingParameter( INT_FLASHLIGHT_DEPTHTEXTURE_FALLBACK_FIRST + iFlashlightShadowIndex );
			}
		}
		
		float flFlashlightPos[4] = { XYZ( flashlightState.m_vecLightOrigin ) };
		pShaderAPI->SetPixelShaderConstant( PSREG_FRESNEL_SPEC_PARAMS, flFlashlightPos );

		SetFlashLightColorFromState( flashlightState, pShaderAPI );

		BindTexture( SHADER_SAMPLER0, flashlightState.m_pSpotlightTexture, flashlightState.m_nSpotlightTextureFrame );

		if( pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() && flashlightState.m_bEnableShadows )
		{
			BindTexture( SHADER_SAMPLER7, pFlashlightDepthTexture, 0 );
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER5, TEXTURE_SHADOW_NOISE_2D );

			// Tweaks associated with a given flashlight
			float tweaks[4];
			tweaks[0] = ShadowFilterFromState( flashlightState );
			tweaks[1] = ShadowAttenFromState( flashlightState );
			HashShadow2DJitter( flashlightState.m_flShadowJitterSeed, &tweaks[2], &tweaks[3] );
			pShaderAPI->SetPixelShaderConstant( PSREG_ENVMAP_TINT__SHADOW_TWEAKS, tweaks, 1 );

			// Dimensions of screen, used for screen-space noise map sampling
			float vScreenScale[4] = {1920.0f / 32.0f, 1080.0f / 32.0f, 0, 0};
			int nWidth, nHeight;
			pShaderAPI->GetBackBufferDimensions( nWidth, nHeight );
			vScreenScale[0] = (float) nWidth  / 32.0f;
			vScreenScale[1] = (float) nHeight / 32.0f;
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_SCREEN_SCALE, vScreenScale, 1 );
		}

		if( params[BASETEXTURE]->IsTexture() && mat_fullbright.GetInt() != 2 )
		{
			BindTexture( SHADER_SAMPLER1, BASETEXTURE, FRAME );
		}
		else
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_GREY );
		}
		if( vars.m_bWorldVertexTransition )
		{
			Assert( vars.m_nBaseTexture2Var >= 0 && vars.m_nBaseTexture2FrameVar >= 0 );
			BindTexture( SHADER_SAMPLER4, vars.m_nBaseTexture2Var, vars.m_nBaseTexture2FrameVar );
		}
		pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_NORMALIZATION_CUBEMAP );
		if( vars.m_bBump )
		{
			BindTexture( SHADER_SAMPLER3, vars.m_nBumpmapVar, vars.m_nBumpmapFrame );
		}
		else
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER3, TEXTURE_NORMALIZATION_CUBEMAP );
		}

		if( bDetail )
		{
			BindTexture( SHADER_SAMPLER8, vars.m_nDetailVar );
		}

		if( vars.m_bWorldVertexTransition )
		{
			if( bBump2 )
			{
				BindTexture( SHADER_SAMPLER6, vars.m_nBumpmap2Var, vars.m_nBumpmap2Frame );
			}
		}

		if( vars.m_bLightmappedGeneric )
		{
			DECLARE_DYNAMIC_VERTEX_SHADER( lightmappedgeneric_flashlight_vs30 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			SET_DYNAMIC_VERTEX_SHADER( lightmappedgeneric_flashlight_vs30 );
			if ( bSeamless )
			{
				float const0[4]={ vars.m_fSeamlessScale,0,0,0};
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, const0 );
			}

			if ( bDetail )
			{
				float vDetailConstants[4] = {1,1,1,1};

				if ( vars.m_nDetailTint != -1 )
				{
					params[vars.m_nDetailTint]->GetVecValue( vDetailConstants, 3 );
				}

				if ( vars.m_nDetailTextureBlendFactor != -1 )
				{
					vDetailConstants[3] = params[vars.m_nDetailTextureBlendFactor]->GetFloatValue();
				}

				pShaderAPI->SetPixelShaderConstant( 0, vDetailConstants, 1 );
			}
		}
		else
		{
			//vertexlitgeneric_flashlight_vs11_Dynamic_Index vshIndex;
			vertexlitgeneric_flashlight_vs30_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

			if( vars.m_bTeeth )
			{
				Assert( vars.m_nTeethForwardVar >= 0 );
				Assert( vars.m_nTeethIllumFactorVar >= 0 );
				Vector4D lighting;
				params[vars.m_nTeethForwardVar]->GetVecValue( lighting.Base(), 3 );
				lighting[3] = params[vars.m_nTeethIllumFactorVar]->GetFloatValue();
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, lighting.Base() );
			}
		}

		pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

		float vEyePos_SpecExponent[4];
		//pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
		//i don't remember what this is from
		vEyePos_SpecExponent[0] = flashlightState.m_vecLightOrigin[0];
		vEyePos_SpecExponent[1] = flashlightState.m_vecLightOrigin[1];
		vEyePos_SpecExponent[2] = flashlightState.m_vecLightOrigin[2];
		vEyePos_SpecExponent[3] = 0.0f;
		pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

		DECLARE_DYNAMIC_PIXEL_SHADER( flashlight_ps30 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE,  pShaderAPI->GetPixelFogCombo() );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, flashlightState.m_bEnableShadows );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( UBERLIGHT, flashlightState.m_bUberlight );
		SET_DYNAMIC_PIXEL_SHADER( flashlight_ps30 );

	//	SetupUberlightFromState( pShaderAPI, flashlightState );

		float atten[4];										// Set the flashlight attenuation factors
		atten[0] = flashlightState.m_fConstantAtten;
		atten[1] = flashlightState.m_fLinearAtten;
		atten[2] = flashlightState.m_fQuadraticAtten;
		atten[3] = flashlightState.m_FarZ;
		s_pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_ATTENUATION, atten, 1 );

		SetFlashlightVertexShaderConstants( vars.m_bBump, vars.m_nBumpTransform, bDetail, vars.m_nDetailScale,  bSeamless ? false : true );
	}
	Draw();
}

// Take 0..1 seed and map to (u, v) coordinate to be used in shadow filter jittering...
void CBaseVSShader::HashShadow2DJitter( const float fJitterSeed, float *fU, float* fV )
{
	const int nTexRes = 32;
	int nSeed = fmod (fJitterSeed, 1.0f) * nTexRes * nTexRes;

	int nRow = nSeed / nTexRes;
	int nCol = nSeed % nTexRes;

	// Div and mod to get an individual texel in the fTexRes x fTexRes grid
	*fU = nRow / (float) nTexRes;	// Row
	*fV = nCol / (float) nTexRes;	// Column
}
#endif

#endif // !_STATIC_LINKED || STDSHADER_DX8_DLL_EXPORT

void CBaseVSShader::DrawEqualDepthToDestAlpha( void )
{
#ifdef STDSHADER_DX11_DLL_EXPORT
//	if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
	{
		bool bMakeActualDrawCall = false;
		if( s_pShaderShadow )
		{
			s_pShaderShadow->EnableColorWrites( false );
			s_pShaderShadow->EnableAlphaWrites( true );
			s_pShaderShadow->EnableDepthWrites( false );
			s_pShaderShadow->EnableAlphaTest( false );
			s_pShaderShadow->EnableBlending( false );

			s_pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_EQUAL );

			s_pShaderShadow->SetVertexShader( "depthtodestalpha_vs40", 0 );
			s_pShaderShadow->SetPixelShader( "depthtodestalpha_ps40", 0 );
		}
		if( s_pShaderAPI )
		{
			BindVertexShaderConstantBuffer( 0, SHADER_CONSTANTBUFFER_PERMODEL );
			BindVertexShaderConstantBuffer( 1, SHADER_CONSTANTBUFFER_PERFRAME );
			BindVertexShaderConstantBuffer( 2, SHADER_CONSTANTBUFFER_PERSCENE );
			s_pShaderAPI->SetVertexShaderIndex( 0 );
			s_pShaderAPI->SetPixelShaderIndex( 0 );

			bMakeActualDrawCall = s_pShaderAPI->ShouldWriteDepthToDestAlpha();
		}
		Draw( bMakeActualDrawCall );
	}
#else
	Assert( 0 ); //probably just needs a shader update to the latest
#endif
}