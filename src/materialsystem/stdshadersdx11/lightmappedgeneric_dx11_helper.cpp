//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Lightmap only shader
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "lightmappedgeneric_dx11_helper.h"
#include "BaseVSShader.h"
#include "../stdshaders/commandbuilder.h"
#include "convar.h"
#include "tier0/vprof.h"

#include <type_traits>

#include "lightmappedgeneric_ps40.inc"
#include "lightmappedgeneric_vs40.inc"

//#include "lightmappedadv_flashlight_ps30.inc"
//#include "lightmappedadv_flashlight_vs30.inc"

#include "tier0/memdbgon.h"

ConVar mat_disable_lightwarp( "mat_disable_lightwarp", "0" );
ConVar mat_disable_fancy_blending( "mat_disable_fancy_blending", "0" );
ConVar mat_fullbright( "mat_fullbright", "0", FCVAR_CHEAT );
static ConVar r_csm_blend_tweak( "r_csm_blend_tweak", "16" );

ConVar mat_enable_lightmapped_phong( "mat_enable_lightmapped_phong", "1", FCVAR_ARCHIVE, "If 1, allow phong on world brushes. If 0, disallow. mat_force_lightmapped_phong does not work if this value is 0." );
ConVar mat_force_lightmapped_phong( "mat_force_lightmapped_phong", "1", FCVAR_CHEAT, "Forces the use of phong on all LightmappedAdv textures, regardless of setting in VMT." );
ConVar mat_force_lightmapped_phong_boost( "mat_force_lightmapped_phong_boost", "5.0", FCVAR_CHEAT );
ConVar mat_force_lightmapped_phong_exp( "mat_force_lightmapped_phong_exp", "50.0", FCVAR_CHEAT );

static ALIGN16 LightmappedGeneric_CBuffer_t s_LightmappedGeneric_CBuffer;
ConstantBufferHandle_t g_hLightmappedGeneric_CBuffer;

// The idea behind the context is to only query the material parameters and calculate stuff
// on shadow state or when the material's parameters have changed. This way, in dynamic state,
// we can update constants and bind stuff as quickly as possible.
class CLightmappedGeneric_DX11_Context : public CBasePerMaterialContextData
{
public:
	uint8 *m_pStaticCmds;
	CCommandBufferBuilder< CFixedCommandStorageBuffer< 1000 > > m_SemiStaticCmdsOut;

	bool m_bVertexShaderFastPath;
	bool m_bPixelShaderFastPath;
	bool m_bPixelShaderForceFastPathBecauseOutline;
	bool m_bFullyOpaque;
	bool m_bFullyOpaqueWithoutAlphaTest;
	bool m_bSeamlessMapping;
	bool m_bHasBlendMaskTransform;
	bool m_bHasTextureTransform;
	bool m_bHasEnvmapMask;
	bool m_bHasBump;
	bool m_bHasBump2;
	bool m_bHasDetailTexture;
	bool m_bHasEnvmap;
	bool m_bHasOutline;
	bool m_bHasSoftEdges;

	LightmappedGeneric_CBuffer_t m_Constants;

	void ResetStaticCmds( void )
	{
		if ( m_pStaticCmds )
		{
			delete[] m_pStaticCmds;
			m_pStaticCmds = NULL;
		}
	}

	CLightmappedGeneric_DX11_Context( void )
	{
		m_pStaticCmds = NULL;
		m_bSeamlessMapping = false;
		m_bHasBlendMaskTransform = false;
		m_bHasTextureTransform = false;
		m_bHasEnvmapMask = false;
		m_bHasBump = false;
		m_bHasBump2 = false;
		m_bHasDetailTexture = false;
		m_bHasOutline = false;
		m_bHasSoftEdges = false;
	}

	~CLightmappedGeneric_DX11_Context( void )
	{
		ResetStaticCmds();
	}

};

void DrawLightmappedAdvFlashlight_DX11_Internal( CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI,
						IShaderShadow *pShaderShadow, const LightmappedAdvFlashlight_DX11_Vars_t &vars )
{
#if 0
	Assert( vars.m_bLightmappedGeneric );

	bool bBump2 = vars.m_bWorldVertexTransition && vars.m_bBump && vars.m_nBumpmap2Var != -1 && params[vars.m_nBumpmap2Var]->IsTexture();
	bool bSeamless = vars.m_fSeamlessScale != 0.0;
	bool bDetail = ( vars.m_nDetailVar != -1 ) && params[vars.m_nDetailVar]->IsDefined() && ( vars.m_nDetailScale != -1 );
	bool bPhong = params[vars.m_nPhong]->GetIntValue() != 0;

	int nDetailBlendMode = 0;
	if ( bDetail )
	{
//		nDetailBlendMode = GetIntParam( vars.m_nDetailTextureCombineMode, params );
//		nDetailBlendMode = nDetailBlendMode > 1 ? 1 : nDetailBlendMode;
	}

	PhongMaskVariant_t nPhongMaskVariant = PHONGMASK_NONE;
	if ( bPhong )
	{
		if ( IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK ) )
		{
			nPhongMaskVariant = PHONGMASK_BASEALPHA;
		}
		else if ( IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK ) )
		{
			nPhongMaskVariant = PHONGMASK_NORMALALPHA;
		}
		else if ( params[vars.m_nPhongMask]->IsDefined() )
		{
			nPhongMaskVariant = PHONGMASK_STANDALONE;
		}
	}

	if ( pShaderShadow )
	{
		pShader->SetInitialShadowState();
		pShaderShadow->EnableDepthWrites( false );
		pShaderShadow->EnableAlphaWrites( false );

		// Alpha blend
		pShader->SetAdditiveBlendingShadowState( BASETEXTURE, true );

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

		if ( vars.m_bWorldVertexTransition )
		{
			// $basetexture2
			pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER4, true );
		}
		if ( bBump2 )
		{
			// Normalmap2 sampler
			pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );
		}
		if ( bDetail )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER8, true );				// detail sampler
			if ( nDetailBlendMode != 0 ) //Not Mod2X
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER8, true );
		}
		if ( nPhongMaskVariant == PHONGMASK_STANDALONE )
		{
			// phong mask sampler
			pShaderShadow->EnableTexture( SHADER_SAMPLER9, true );
		}

		pShaderShadow->EnableSRGBWrite( true );

		DECLARE_STATIC_VERTEX_SHADER( lightmappedadv_flashlight_vs40 );
		SET_STATIC_VERTEX_SHADER_COMBO( WORLDVERTEXTRANSITION, vars.m_bWorldVertexTransition );
		SET_STATIC_VERTEX_SHADER_COMBO( NORMALMAP, vars.m_bBump );
		SET_STATIC_VERTEX_SHADER_COMBO( SEAMLESS, bSeamless );
		SET_STATIC_VERTEX_SHADER_COMBO( DETAIL, bDetail );
		SET_STATIC_VERTEX_SHADER_COMBO( PHONG, bPhong );
		SET_STATIC_VERTEX_SHADER( lightmappedadv_flashlight_vs40 );

		unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;
		if ( vars.m_bBump )
		{
			flags |= VERTEX_TANGENT_S | VERTEX_TANGENT_T;
		}
		int numTexCoords = 1;
		if ( vars.m_bWorldVertexTransition )
		{
			flags |= VERTEX_COLOR;
			numTexCoords = 2; // need lightmap texcoords to get alpha.
		}
		pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0 );

		int nBumpMapVariant = 0;
		if ( vars.m_bBump )
		{
			nBumpMapVariant = ( vars.m_bSSBump ) ? 2 : 1;
		}

		DECLARE_STATIC_PIXEL_SHADER( lightmappedadv_flashlight_ps40 );
		SET_STATIC_PIXEL_SHADER_COMBO( NORMALMAP, nBumpMapVariant );
		SET_STATIC_PIXEL_SHADER_COMBO( NORMALMAP2, bBump2 );
		SET_STATIC_PIXEL_SHADER_COMBO( WORLDVERTEXTRANSITION, vars.m_bWorldVertexTransition );
		SET_STATIC_PIXEL_SHADER_COMBO( SEAMLESS, bSeamless );
		SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE, bDetail );
		SET_STATIC_PIXEL_SHADER_COMBO( DETAIL_BLEND_MODE, nDetailBlendMode );
		SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, 0 );
		SET_STATIC_PIXEL_SHADER_COMBO( PHONG, bPhong );
		SET_STATIC_PIXEL_SHADER_COMBO( PHONGMASK, nPhongMaskVariant );
		SET_STATIC_PIXEL_SHADER( lightmappedadv_flashlight_ps40 );

		pShader->FogToBlack();
	}
	else
	{
		VMatrix worldToTexture;
		ITexture *pFlashlightDepthTexture;
		FlashlightState_t flashlightState = pShaderAPI->GetFlashlightStateEx( worldToTexture, &pFlashlightDepthTexture );

		if ( pFlashlightDepthTexture == NULL )
		{
			const int iFlashlightShadowIndex = ( flashlightState.m_nShadowQuality >> 16 ) & 0xFF;

			if ( iFlashlightShadowIndex >= 0
			     && iFlashlightShadowIndex <= ( INT_FLASHLIGHT_DEPTHTEXTURE_FALLBACK_LAST - INT_FLASHLIGHT_DEPTHTEXTURE_FALLBACK_FIRST ) )
			{
				pFlashlightDepthTexture = (ITexture *)pShaderAPI->GetIntRenderingParameter( INT_FLASHLIGHT_DEPTHTEXTURE_FALLBACK_FIRST + iFlashlightShadowIndex );
			}
		}

		SetFlashLightColorFromState( flashlightState, pShaderAPI );

		pShader->BindTexture( SHADER_SAMPLER0, flashlightState.m_pSpotlightTexture, flashlightState.m_nSpotlightTextureFrame );

		if ( pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() && flashlightState.m_bEnableShadows )
		{
			pShader->BindTexture( SHADER_SAMPLER7, pFlashlightDepthTexture, 0 );
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER5, TEXTURE_SHADOW_NOISE_2D );

			// Tweaks associated with a given flashlight
			float tweaks[4];
			tweaks[0] = flashlightState.m_flShadowFilterSize / flashlightState.m_flShadowMapResolution;
			tweaks[1] = ShadowAttenFromState( flashlightState );
			pShader->HashShadow2DJitter( flashlightState.m_flShadowJitterSeed, &tweaks[2], &tweaks[3] );
			pShaderAPI->SetPixelShaderConstant( PSREG_ENVMAP_TINT__SHADOW_TWEAKS, tweaks, 1 );

			// Dimensions of screen, used for screen-space noise map sampling
			float vScreenScale[4] = { 1280.0f / 32.0f, 720.0f / 32.0f, 0, 0 };
			int nWidth, nHeight;
			pShaderAPI->GetBackBufferDimensions( nWidth, nHeight );
			vScreenScale[0] = static_cast<float>( nWidth ) / 32.0f;
			vScreenScale[1] = static_cast<float>( nHeight ) / 32.0f;
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_SCREEN_SCALE, vScreenScale, 1 );
		}

		if ( params[BASETEXTURE]->IsTexture() && mat_fullbright.GetInt() != 2 )
		{
			pShader->BindTexture( SHADER_SAMPLER1, BASETEXTURE, FRAME );
		}
		else
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_GREY );
		}
		if ( vars.m_bWorldVertexTransition )
		{
			Assert( vars.m_nBaseTexture2Var >= 0 && vars.m_nBaseTexture2FrameVar >= 0 );
			pShader->BindTexture( SHADER_SAMPLER4, vars.m_nBaseTexture2Var, vars.m_nBaseTexture2FrameVar );
		}
		pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_NORMALIZATION_CUBEMAP );
		if ( vars.m_bBump )
		{
			pShader->BindTexture( SHADER_SAMPLER3, vars.m_nBumpmapVar, vars.m_nBumpmapFrame );
		}
		else
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER3, TEXTURE_NORMALIZATION_CUBEMAP );
		}

		if ( bDetail )
		{
			pShader->BindTexture( SHADER_SAMPLER8, vars.m_nDetailVar );
		}

		if ( bBump2 )
		{
			pShader->BindTexture( SHADER_SAMPLER6, vars.m_nBumpmap2Var, vars.m_nBumpmap2Frame );
		}

		if ( nPhongMaskVariant == PHONGMASK_STANDALONE )
		{
			pShader->BindTexture( SHADER_SAMPLER9, vars.m_nPhongMask, vars.m_nPhongMaskFrame );
		}

		DECLARE_DYNAMIC_VERTEX_SHADER( lightmappedadv_flashlight_vs40 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
		SET_DYNAMIC_VERTEX_SHADER( lightmappedadv_flashlight_vs40 );

		if ( bSeamless )
		{
			float const0[4] = { vars.m_fSeamlessScale,0,0,0 };
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, const0 );
		}

		if ( bDetail )
		{
			float vDetailConstants[4] = { 1,1,1,1 };

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

		if ( bPhong )
		{
			float vEyePos[4];
			pShaderAPI->GetWorldSpaceCameraPosition( vEyePos );
			vEyePos[3] = 0.0f;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_10, vEyePos );
		}

		pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

		float vEyePos_SpecExponent[4];
		pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
		vEyePos_SpecExponent[3] = params[vars.m_nPhongExponent]->GetFloatValue();
		pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

		static IShaderDynamicAPI *shaderAPI;
		shaderAPI = pShaderAPI;

		auto func = []( int var, const float *pVec, int nConsts )
		{
			shaderAPI->SetPixelShaderConstant( var, pVec, nConsts );
		};

		DECLARE_DYNAMIC_PIXEL_SHADER( lightmappedadv_flashlight_ps40 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, flashlightState.m_bEnableShadows );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( UBERLIGHT, flashlightState.m_bUberlight );
		SET_DYNAMIC_PIXEL_SHADER( lightmappedadv_flashlight_ps40 );

		float atten[4];										// Set the flashlight attenuation factors
		atten[0] = flashlightState.m_fConstantAtten;
		atten[1] = flashlightState.m_fLinearAtten;
		atten[2] = flashlightState.m_fQuadraticAtten;
		atten[3] = flashlightState.m_FarZ;
		pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_ATTENUATION, atten, 1 );

		float lightPos[4];
		lightPos[0] = flashlightState.m_vecLightOrigin[0];
		lightPos[1] = flashlightState.m_vecLightOrigin[1];
		lightPos[2] = flashlightState.m_vecLightOrigin[2];
		lightPos[3] = 1.0f;
		pShaderAPI->SetPixelShaderConstant( 1, lightPos, 1 );

		float specParams[4];
		params[vars.m_nPhongFresnelRanges]->GetVecValue( specParams, 3 );
		specParams[3] = params[vars.m_nPhongBoost]->GetFloatValue();
		pShaderAPI->SetPixelShaderConstant( PSREG_FRESNEL_SPEC_PARAMS, specParams, 1 );

		pShader->SetFlashlightVertexShaderConstants( vars.m_bBump, vars.m_nBumpTransform, bDetail, vars.m_nDetailScale, bSeamless ? false : true );
	}
	pShader->Draw();
#endif
}

void InitParamsLightmappedGeneric_DX11( CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, LightmappedGeneric_DX11_Vars_t &info )
{
	if ( g_pHardwareConfig->SupportsBorderColor() )
	{
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight_border" );
	}
	else
	{
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );
	}

	// Write over $basetexture with $albedo if we are going to be using diffuse normal mapping.
	if ( g_pConfig->UseBumpmapping() && params[info.m_nBumpmap]->IsDefined() && params[info.m_nAlbedo]->IsDefined() &&
	     params[info.m_nBaseTexture]->IsDefined() &&
	     !( params[info.m_nNoDiffuseBumpLighting]->IsDefined() && params[info.m_nNoDiffuseBumpLighting]->GetIntValue() ) )
	{
		params[info.m_nBaseTexture]->SetStringValue( params[info.m_nAlbedo]->GetStringValue() );
	}

	if ( pShader->IsUsingGraphics() && params[info.m_nEnvmap]->IsDefined() && !pShader->CanUseEditorMaterials() )
	{
		if ( stricmp( params[info.m_nEnvmap]->GetStringValue(), "env_cubemap" ) == 0 )
		{
			Warning( "env_cubemap used on world geometry without rebuilding map. . ignoring: %s\n", pMaterialName );
			params[info.m_nEnvmap]->SetUndefined();
		}
	}

	if ( ( mat_disable_lightwarp.GetBool() ) &&
		( info.m_nLightWarpTexture != -1 ) )
	{
		params[info.m_nLightWarpTexture]->SetUndefined();
	}
	if ( ( mat_disable_fancy_blending.GetBool() ) &&
		( info.m_nBlendModulateTexture != -1 ) )
	{
		params[info.m_nBlendModulateTexture]->SetUndefined();
	}

	if ( !params[info.m_nEnvmapTint]->IsDefined() )
		params[info.m_nEnvmapTint]->SetVecValue( 1.0f, 1.0f, 1.0f );

	if ( !params[info.m_nNoDiffuseBumpLighting]->IsDefined() )
		params[info.m_nNoDiffuseBumpLighting]->SetIntValue( 0 );

	if ( !params[info.m_nSelfIllumTint]->IsDefined() )
		params[info.m_nSelfIllumTint]->SetVecValue( 1.0f, 1.0f, 1.0f );

	if ( !params[info.m_nDetailScale]->IsDefined() )
		params[info.m_nDetailScale]->SetFloatValue( 4.0f );

	if ( !params[info.m_nDetailTint]->IsDefined() )
		params[info.m_nDetailTint]->SetVecValue( 1.0f, 1.0f, 1.0f, 1.0f );

	InitFloatParam( info.m_nDetailTextureBlendFactor, params, 1.0 );
	InitIntParam( info.m_nDetailTextureCombineMode, params, 0 );

	if ( !params[info.m_nFresnelReflection]->IsDefined() )
		params[info.m_nFresnelReflection]->SetFloatValue( 1.0f );

	if ( !params[info.m_nEnvmapMaskFrame]->IsDefined() )
		params[info.m_nEnvmapMaskFrame]->SetIntValue( 0 );

	if ( !params[info.m_nEnvmapFrame]->IsDefined() )
		params[info.m_nEnvmapFrame]->SetIntValue( 0 );

	if ( !params[info.m_nBumpFrame]->IsDefined() )
		params[info.m_nBumpFrame]->SetIntValue( 0 );

	if ( !params[info.m_nDetailFrame]->IsDefined() )
		params[info.m_nDetailFrame]->SetIntValue( 0 );

	if ( !params[info.m_nEnvmapContrast]->IsDefined() )
		params[info.m_nEnvmapContrast]->SetFloatValue( 0.0f );

	if ( !params[info.m_nEnvmapSaturation]->IsDefined() )
		params[info.m_nEnvmapSaturation]->SetFloatValue( 1.0f );

	InitFloatParam( info.m_nAlphaTestReference, params, 0.0f );

	// No texture means no self-illum or env mask in base alpha
	if ( !params[info.m_nBaseTexture]->IsDefined() )
	{
		CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	}

	if ( params[info.m_nBumpmap]->IsDefined() )
	{
		params[info.m_nEnvmapMask]->SetUndefined();
	}

	// If in decal mode, no debug override...
	if ( IS_FLAG_SET( MATERIAL_VAR_DECAL ) )
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
	}

	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	if ( g_pConfig->UseBumpmapping() && params[info.m_nBumpmap]->IsDefined() && ( params[info.m_nNoDiffuseBumpLighting]->GetIntValue() == 0 ) )
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP );
	}

	// If mat_specular 0, then get rid of envmap
	if ( !g_pConfig->UseSpecular() && params[info.m_nEnvmap]->IsDefined() && params[info.m_nBaseTexture]->IsDefined() )
	{
		params[info.m_nEnvmap]->SetUndefined();
	}

	if ( !params[info.m_nBaseTextureNoEnvmap]->IsDefined() )
	{
		params[info.m_nBaseTextureNoEnvmap]->SetIntValue( 0 );
	}
	if ( !params[info.m_nBaseTexture2NoEnvmap]->IsDefined() )
	{
		params[info.m_nBaseTexture2NoEnvmap]->SetIntValue( 0 );
	}

	if ( ( info.m_nSelfShadowedBumpFlag != -1 ) &&
		( !params[info.m_nSelfShadowedBumpFlag]->IsDefined() )
	     )
	{
		params[info.m_nSelfShadowedBumpFlag]->SetIntValue( 0 );
	}
	// handle line art parms
	InitFloatParam( info.m_nEdgeSoftnessStart, params, 0.5 );
	InitFloatParam( info.m_nEdgeSoftnessEnd, params, 0.5 );
	InitFloatParam( info.m_nOutlineAlpha, params, 1.0 );

	if ( !params[info.m_nPhong]->IsDefined() || !mat_enable_lightmapped_phong.GetBool() )
	{
		params[info.m_nPhong]->SetIntValue( 0 );
	}
	if ( !params[info.m_nPhongBoost]->IsDefined() )
	{
		params[info.m_nPhongBoost]->SetFloatValue( 1.0 );
	}
	if ( !params[info.m_nPhongFresnelRanges]->IsDefined() )
	{
		params[info.m_nPhongFresnelRanges]->SetVecValue( 0.0, 0.5, 1.0 );
	}
	if ( !params[info.m_nPhongExponent]->IsDefined() )
	{
		params[info.m_nPhongExponent]->SetFloatValue( 5.0 );
	}

	if ( params[info.m_nPhong]->GetIntValue() && mat_enable_lightmapped_phong.GetBool() )
	{
		if ( pShader->CanUseEditorMaterials() )
		{
			params[info.m_nPhong]->SetIntValue( 0 );
		}
		else if ( !params[info.m_nEnvmapMaskTransform]->MatrixIsIdentity() )
		{
			Warning( "Warning! material %s: $envmapmasktransform and $phong are mutial exclusive. Disabling phong..\n", pMaterialName );
			params[info.m_nPhong]->SetIntValue( 0 );
		}
	}
	else if ( mat_force_lightmapped_phong.GetBool() && mat_enable_lightmapped_phong.GetBool() &&
		  params[info.m_nEnvmapMaskTransform]->MatrixIsIdentity() )
	{
		params[info.m_nPhong]->SetIntValue( 1 );
		params[info.m_nPhongBoost]->SetFloatValue( mat_force_lightmapped_phong_boost.GetFloat() );
		params[info.m_nPhongFresnelRanges]->SetVecValue( 0.0, 0.5, 1.0 );
		params[info.m_nPhongExponent]->SetFloatValue( mat_force_lightmapped_phong_exp.GetFloat() );
	}

	SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
}

void InitLightmappedGeneric_DX11( CBaseVSShader *pShader, IMaterialVar **params, LightmappedGeneric_DX11_Vars_t &info )
{
	if ( g_pConfig->UseBumpmapping() && params[info.m_nBumpmap]->IsDefined() )
	{
		pShader->LoadBumpMap( info.m_nBumpmap );
	}

	if ( g_pConfig->UseBumpmapping() && params[info.m_nBumpmap2]->IsDefined() )
	{
		pShader->LoadBumpMap( info.m_nBumpmap2 );
	}

	if ( g_pConfig->UseBumpmapping() && params[info.m_nBumpMask]->IsDefined() )
	{
		pShader->LoadBumpMap( info.m_nBumpMask );
	}

	if ( params[info.m_nBaseTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTexture );

		if ( !params[info.m_nBaseTexture]->GetTextureValue()->IsTranslucent() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}
	}

	if ( params[info.m_nBaseTexture2]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTexture2 );
	}

	if ( params[info.m_nLightWarpTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nLightWarpTexture );
	}

	if ( ( info.m_nBlendModulateTexture != -1 ) &&
		( params[info.m_nBlendModulateTexture]->IsDefined() ) )
	{
		pShader->LoadTexture( info.m_nBlendModulateTexture );
	}

	if ( params[info.m_nDetail]->IsDefined() )
	{
		int nDetailBlendMode = ( info.m_nDetailTextureCombineMode == -1 ) ? 0 : params[info.m_nDetailTextureCombineMode]->GetIntValue();
		nDetailBlendMode = nDetailBlendMode > 1 ? 1 : nDetailBlendMode;

		pShader->LoadTexture( info.m_nDetail/*, nDetailBlendMode != 0 ? TEXTUREFLAGS_SRGB : 0*/ );
	}

	pShader->LoadTexture( info.m_nFlashlightTexture );

	// Don't alpha test if the alpha channel is used for other purposes
	if ( IS_FLAG_SET( MATERIAL_VAR_SELFILLUM ) || IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK ) )
	{
		CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
	}

	if ( params[info.m_nEnvmap]->IsDefined() )
	{
		if ( !IS_FLAG_SET( MATERIAL_VAR_ENVMAPSPHERE ) )
		{
			pShader->LoadCubeMap( info.m_nEnvmap/*, g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE ? TEXTUREFLAGS_SRGB : 0*/ );
		}
		else
		{
			pShader->LoadTexture( info.m_nEnvmap );
		}

		if ( !g_pHardwareConfig->SupportsCubeMaps() )
		{
			SET_FLAGS( MATERIAL_VAR_ENVMAPSPHERE );
		}

		if ( params[info.m_nEnvmapMask]->IsDefined() )
		{
			pShader->LoadTexture( info.m_nEnvmapMask );
		}
	}
	else
	{
		params[info.m_nEnvmapMask]->SetUndefined();
	}

	// We always need this because of the flashlight.
	SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
}

void DrawLightmappedGeneric_DX11_Internal( CBaseVSShader *pShader, IMaterialVar **params, bool hasFlashlight,
					  IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow,
					  LightmappedGeneric_DX11_Vars_t &info,
					  CBasePerMaterialContextData **pContextDataPtr
)
{
	VPROF_BUDGET( "DrawLightmappedGeneric_DX11", VPROF_BUDGETGROUP_OTHER_UNACCOUNTED );

	CLightmappedGeneric_DX11_Context *pContextData = reinterpret_cast<CLightmappedGeneric_DX11_Context *> ( *pContextDataPtr );
	if ( pShaderShadow || ( !pContextData ) || pContextData->m_bMaterialVarsChanged || hasFlashlight )
	{
		bool hasBaseTexture = params[info.m_nBaseTexture]->IsTexture();
		int nAlphaChannelTextureVar = hasBaseTexture ? (int)info.m_nBaseTexture : (int)info.m_nEnvmapMask;
		BlendType_t nBlendType = pShader->EvaluateBlendRequirements( nAlphaChannelTextureVar, hasBaseTexture );
		bool bIsAlphaTested = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0;
		bool bFullyOpaqueWithoutAlphaTest = ( nBlendType != BT_BLENDADD ) && ( nBlendType != BT_BLEND ) && ( !hasFlashlight || IsX360() ); //dest alpha is free for special use
		bool bFullyOpaque = bFullyOpaqueWithoutAlphaTest && !bIsAlphaTested;
		bool bNeedRegenStaticCmds = ( !pContextData ) || pShaderShadow;

		if ( !pContextData )								// make sure allocated
		{
			pContextData = new CLightmappedGeneric_DX11_Context;
			*pContextDataPtr = pContextData;
		}

		pContextData->m_bHasBump = ( params[info.m_nBumpmap]->IsTexture() ) && ( !g_pHardwareConfig->PreferReducedFillrate() );
		bool hasSSBump = pContextData->m_bHasBump && ( info.m_nSelfShadowedBumpFlag != -1 ) && ( params[info.m_nSelfShadowedBumpFlag]->GetIntValue() );
		bool hasBaseTexture2 = hasBaseTexture && params[info.m_nBaseTexture2]->IsTexture();
		bool hasLightWarpTexture = params[info.m_nLightWarpTexture]->IsTexture();
		pContextData->m_bHasBump2 = pContextData->m_bHasBump && params[info.m_nBumpmap2]->IsTexture();
		bool hasDetailTexture = params[info.m_nDetail]->IsTexture();
		pContextData->m_bHasDetailTexture = hasDetailTexture;
		bool hasSelfIllum = IS_FLAG_SET( MATERIAL_VAR_SELFILLUM );
		bool hasBumpMask = pContextData->m_bHasBump && pContextData->m_bHasBump2 && params[info.m_nBumpMask]->IsTexture() && !hasSelfIllum &&
			!hasDetailTexture && !hasBaseTexture2 && ( params[info.m_nBaseTextureNoEnvmap]->GetIntValue() == 0 );
		bool bHasBlendModulateTexture =
			( info.m_nBlendModulateTexture != -1 ) &&
			( params[info.m_nBlendModulateTexture]->IsTexture() );
		bool hasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );

		if ( hasFlashlight && !IsX360() )
		{
			// !!speed!! do this in the caller so we don't build struct every time
			LightmappedAdvFlashlight_DX11_Vars_t vars;
			vars.m_bBump = pContextData->m_bHasBump;
			vars.m_nBumpmapVar = info.m_nBumpmap;
			vars.m_nBumpmapFrame = info.m_nBumpFrame;
			vars.m_nBumpTransform = info.m_nBumpTransform;
			vars.m_nFlashlightTextureVar = info.m_nFlashlightTexture;
			vars.m_nFlashlightTextureFrameVar = info.m_nFlashlightTextureFrame;
			vars.m_bLightmappedGeneric = true;
			vars.m_bWorldVertexTransition = hasBaseTexture2;
			vars.m_nBaseTexture2Var = info.m_nBaseTexture2;
			vars.m_nBaseTexture2FrameVar = info.m_nBaseTexture2Frame;
			vars.m_nBumpmap2Var = info.m_nBumpmap2;
			vars.m_nBumpmap2Frame = info.m_nBumpFrame2;
			vars.m_nBump2Transform = info.m_nBumpTransform2;
			vars.m_nAlphaTestReference = info.m_nAlphaTestReference;
			vars.m_bSSBump = hasSSBump;
			vars.m_nDetailVar = info.m_nDetail;
			vars.m_nDetailScale = info.m_nDetailScale;
			vars.m_nDetailTextureCombineMode = info.m_nDetailTextureCombineMode;
			vars.m_nDetailTextureBlendFactor = info.m_nDetailTextureBlendFactor;
			vars.m_nDetailTint = info.m_nDetailTint;

			if ( ( info.m_nSeamlessMappingScale != -1 ) )
				vars.m_fSeamlessScale = params[info.m_nSeamlessMappingScale]->GetFloatValue();
			else
				vars.m_fSeamlessScale = 0.0;

			vars.m_nPhong = info.m_nPhong;
			vars.m_nPhongBoost = info.m_nPhongBoost;
			vars.m_nPhongFresnelRanges = info.m_nPhongFresnelRanges;
			vars.m_nPhongExponent = info.m_nPhongExponent;
			vars.m_nPhongMask = info.m_nEnvmapMask;
			vars.m_nPhongMaskFrame = info.m_nEnvmapMaskFrame;

			DrawLightmappedAdvFlashlight_DX11_Internal( pShader, params, pShaderAPI, pShaderShadow, vars );
			return;
		}

		pContextData->m_bFullyOpaque = bFullyOpaque;
		pContextData->m_bFullyOpaqueWithoutAlphaTest = bFullyOpaqueWithoutAlphaTest;

		bool bHasOutline = IsBoolSet( info.m_nOutline, params );
		pContextData->m_bHasOutline = bHasOutline;
		pContextData->m_bPixelShaderForceFastPathBecauseOutline = bHasOutline;
		bool bHasSoftEdges = IsBoolSet( info.m_nSoftEdges, params );
		pContextData->m_bHasSoftEdges = bHasSoftEdges;
		pContextData->m_bHasEnvmapMask = params[info.m_nEnvmapMask]->IsTexture();

		float fDetailBlendFactor = GetFloatParam( info.m_nDetailTextureBlendFactor, params, 1.0 );

		if ( pShaderShadow || bNeedRegenStaticCmds )
		{
			bool hasVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );
			bool hasDiffuseBumpmap = pContextData->m_bHasBump && ( params[info.m_nNoDiffuseBumpLighting]->GetIntValue() == 0 );

			bool hasEnvmap = params[info.m_nEnvmap]->IsTexture();
			pContextData->m_bHasEnvmap = hasEnvmap;

			if ( bNeedRegenStaticCmds )
			{
				pContextData->ResetStaticCmds();
				CCommandBufferBuilder< CFixedCommandStorageBuffer< 5000 > > staticCmdsBuf;


				if ( !hasBaseTexture )
				{
					if ( hasEnvmap )
					{
						// if we only have an envmap (no basetexture), then we want the albedo to be black.
						staticCmdsBuf.BindStandardTexture( SHADER_SAMPLER0, TEXTURE_BLACK );
					}
					else
					{
						staticCmdsBuf.BindStandardTexture( SHADER_SAMPLER0, TEXTURE_WHITE );
					}
				}
				staticCmdsBuf.BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );

				pContextData->m_bSeamlessMapping = ( ( info.m_nSeamlessMappingScale != -1 ) &&
					( params[info.m_nSeamlessMappingScale]->GetFloatValue() != 0.0 ) );

				if ( pContextData->m_bSeamlessMapping )
				{
					pContextData->m_Constants.cSeamlessMappingScale.x = params[info.m_nSeamlessMappingScale]->GetFloatValue();
				}

				staticCmdsBuf.End();
				// now, copy buf
				pContextData->m_pStaticCmds = new uint8[staticCmdsBuf.Size()];
				memcpy( pContextData->m_pStaticCmds, staticCmdsBuf.Base(), staticCmdsBuf.Size() );
			}
			if ( pShaderShadow )
			{
				pShader->SetDefaultBlendingShadowState( nAlphaChannelTextureVar, hasBaseTexture );

				// Bind em

				pShader->SetInternalVertexShaderConstantBuffersNoSkinning();
				pShader->SetVertexShaderConstantBuffer( 3, g_hLightmappedGeneric_CBuffer );

				pShader->SetInternalPixelShaderConstantBuffers();
				pShader->SetPixelShaderConstantBuffer( 3, g_hLightmappedGeneric_CBuffer );

				unsigned int flags = VERTEX_POSITION;

				if ( hasEnvmap )
				{
					flags |= VERTEX_TANGENT_S | VERTEX_TANGENT_T | VERTEX_NORMAL;
				}

				if ( hasVertexColor || hasBaseTexture2 || pContextData->m_bHasBump2 )
				{
					flags |= VERTEX_COLOR;
				}

				// texcoord0 : base texcoord
				// texcoord1 : lightmap texcoord
				// texcoord2 : lightmap texcoord offset
				int numTexCoords = 2;
				if ( pContextData->m_bHasBump )
				{
					numTexCoords = 3;
				}

				pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0 );

				// Pre-cache pixel shaders
				bool hasBaseAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK );

				int bumpmap_variant = ( hasSSBump ) ? 2 : pContextData->m_bHasBump;
				bool bMaskedBlending = ( ( info.m_nMaskedBlending != -1 ) &&
					( params[info.m_nMaskedBlending]->GetIntValue() != 0 ) );

				DECLARE_STATIC_VERTEX_SHADER( lightmappedgeneric_vs40 );
				SET_STATIC_VERTEX_SHADER_COMBO( ENVMAP_MASK, pContextData->m_bHasEnvmapMask );
				SET_STATIC_VERTEX_SHADER_COMBO( TANGENTSPACE, pContextData->m_bHasEnvmap );
				SET_STATIC_VERTEX_SHADER_COMBO( BUMPMAP, pContextData->m_bHasBump );
				SET_STATIC_VERTEX_SHADER_COMBO( DIFFUSEBUMPMAP, hasDiffuseBumpmap );
				SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) );
				SET_STATIC_VERTEX_SHADER_COMBO( VERTEXALPHATEXBLENDFACTOR, hasBaseTexture2 || pContextData->m_bHasBump2 );
				SET_STATIC_VERTEX_SHADER_COMBO( BUMPMASK, hasBumpMask );

				SET_STATIC_VERTEX_SHADER_COMBO( RELIEF_MAPPING, false );
				SET_STATIC_VERTEX_SHADER_COMBO( SEAMLESS, pContextData->m_bSeamlessMapping );
				SET_STATIC_VERTEX_SHADER( lightmappedgeneric_vs40 );


				DECLARE_STATIC_PIXEL_SHADER( lightmappedgeneric_ps40 );
				SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE2, hasBaseTexture2 );
				SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE, hasDetailTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP, bumpmap_variant );
				SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP2, pContextData->m_bHasBump2 );
				SET_STATIC_PIXEL_SHADER_COMBO( BUMPMASK, hasBumpMask );
				SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSEBUMPMAP, hasDiffuseBumpmap );
				SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP, pContextData->m_bHasEnvmap );
				SET_STATIC_PIXEL_SHADER_COMBO( ENVMAPMASK, pContextData->m_bHasEnvmapMask );
				SET_STATIC_PIXEL_SHADER_COMBO( BASEALPHAENVMAPMASK, hasBaseAlphaEnvmapMask );
				SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM, hasSelfIllum );
				SET_STATIC_PIXEL_SHADER_COMBO( NORMALMAPALPHAENVMAPMASK, hasNormalMapAlphaEnvmapMask );
				SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURENOENVMAP, params[info.m_nBaseTextureNoEnvmap]->GetIntValue() );
				SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE2NOENVMAP, params[info.m_nBaseTexture2NoEnvmap]->GetIntValue() );
				SET_STATIC_PIXEL_SHADER_COMBO( WARPLIGHTING, hasLightWarpTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( FANCY_BLENDING, bHasBlendModulateTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( MASKEDBLENDING, bMaskedBlending );
				SET_STATIC_PIXEL_SHADER_COMBO( SEAMLESS, pContextData->m_bSeamlessMapping );
				SET_STATIC_PIXEL_SHADER_COMBO( ALPHATEST, bIsAlphaTested );
				SET_STATIC_PIXEL_SHADER( lightmappedgeneric_ps40 );

				// HACK HACK HACK - enable alpha writes all the time so that we have them for
				// underwater stuff and writing depth to dest alpha
				// But only do it if we're not using the alpha already for translucency
				pShaderShadow->EnableAlphaWrites( bFullyOpaque );

				//pShaderShadow->EnableSRGBWrite( true );

				pShader->DefaultFog();

			} // end shadow state
		} // end shadow || regen display list
		if ( pShaderAPI && pContextData->m_bMaterialVarsChanged )
		{
			// need to regenerate the semistatic cmds
			pContextData->m_SemiStaticCmdsOut.Reset();
			pContextData->m_bMaterialVarsChanged = false;

			int nDetailBlendMode = 0;
			if ( hasDetailTexture )
			{
				nDetailBlendMode = GetIntParam( info.m_nDetailTextureCombineMode, params );
				ITexture *pDetailTexture = params[info.m_nDetail]->GetTextureValue();
				if ( pDetailTexture->GetFlags() & TEXTUREFLAGS_SSBUMP )
				{
					if ( pContextData->m_bHasBump )
						nDetailBlendMode = 10;					// ssbump
					else
						nDetailBlendMode = 11;					// ssbump_nobump
				}
			}
			pContextData->m_Constants.g_DetailBlendMode.x = nDetailBlendMode;

			pContextData->m_bHasBlendMaskTransform = (
				( info.m_nBlendMaskTransform != -1 ) &&
				( info.m_nMaskedBlending != -1 ) &&
				( params[info.m_nMaskedBlending]->GetIntValue() ) &&
				( !( params[info.m_nBumpTransform]->MatrixIsIdentity() ) ) );

			// If we don't have a texture transform, we don't have
			// to set vertex shader constants or run vertex shader instructions
			// for the texture transform.
			pContextData->m_bHasTextureTransform =
				!( params[info.m_nBaseTextureTransform]->MatrixIsIdentity() &&
				   params[info.m_nBumpTransform]->MatrixIsIdentity() &&
				   params[info.m_nBumpTransform2]->MatrixIsIdentity() &&
				   params[info.m_nEnvmapMaskTransform]->MatrixIsIdentity() );

			pContextData->m_bHasTextureTransform |= pContextData->m_bHasBlendMaskTransform;

			pContextData->m_bVertexShaderFastPath = !pContextData->m_bHasTextureTransform;

			if ( params[info.m_nDetail]->IsTexture() )
			{
				pContextData->m_bVertexShaderFastPath = false;
			}
			if ( pContextData->m_bHasBlendMaskTransform )
			{
				pShader->StoreVertexShaderTextureTransform( pContextData->m_Constants.cBlendMaskTexCoordTransform,
									    info.m_nBlendMaskTransform );
			}

			if ( !pContextData->m_bVertexShaderFastPath )
			{
				pContextData->m_bHasEnvmapMask = params[info.m_nEnvmapMask]->IsTexture();
				if ( !pContextData->m_bSeamlessMapping )
					pShader->StoreVertexShaderTextureTransform( pContextData->m_Constants.cBaseTexCoordTransform, info.m_nBaseTextureTransform );

				// If we have a detail texture, then the bump texcoords are the same as the base texcoords.
				if ( pContextData->m_bHasBump && !pContextData->m_bHasDetailTexture )
				{
					pShader->StoreVertexShaderTextureTransform( pContextData->m_Constants.cDetailOrBumpTexCoordTransform, info.m_nBumpTransform );
				}
				if ( pContextData->m_bHasEnvmapMask )
				{
					pShader->StoreVertexShaderTextureTransform( pContextData->m_Constants.cEnvmapMaskOrBump2TexCoordTransform, info.m_nEnvmapMaskTransform );
				}
				else if ( pContextData->m_bHasBump2 )
				{
					pShader->StoreVertexShaderTextureTransform( pContextData->m_Constants.cEnvmapMaskOrBump2TexCoordTransform, info.m_nBumpTransform2 );

				}
			}

			pShader->StoreEnvmapTint( pContextData->m_Constants.g_EnvmapTint, info.m_nEnvmapTint );

			// set up shader modulation color
			pContextData->m_Constants.cModulationColor.Init( 1, 1, 1, 1 );
			pShader->ComputeModulationColor( pContextData->m_Constants.cModulationColor.Base() );
			float flLScale = pShaderAPI->GetLightMapScaleFactor();
			pContextData->m_Constants.cModulationColor[0] *= flLScale;
			pContextData->m_Constants.cModulationColor[1] *= flLScale;
			pContextData->m_Constants.cModulationColor[2] *= flLScale;

			pContextData->m_Constants.g_TintValuesAndLightmapScale = pContextData->m_Constants.cModulationColor;
			pContextData->m_Constants.g_TintValuesAndLightmapScale[3] *=
				( IS_PARAM_DEFINED( info.m_nAlpha2 ) && params[info.m_nAlpha2]->GetFloatValue() > 0.0f ) ? params[info.m_nAlpha2]->GetFloatValue() : 1.0f;
			//pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 12, color );
			//pContextData->m_TintValuesAndLightmapScale.Init( 0, 0, 1, 1 );

			if ( pContextData->m_bHasDetailTexture )
			{
				pContextData->m_Constants.g_DetailTint_and_BlendFactor.Init( 1, 1, 1, 1 );

				if ( info.m_nDetailTint != -1 )
				{
					params[info.m_nDetailTint]->GetVecValue( pContextData->m_Constants.g_DetailTint_and_BlendFactor.Base(), 3 );
				}

				pContextData->m_Constants.g_DetailTint_and_BlendFactor[3] = fDetailBlendFactor;

			}

			float selfIllumTintVal[4];
			params[info.m_nSelfIllumTint]->GetVecValue( selfIllumTintVal, 3 );
			float envmapContrast = params[info.m_nEnvmapContrast]->GetFloatValue();
			float envmapSaturation = params[info.m_nEnvmapSaturation]->GetFloatValue();
			float fresnelReflection = params[info.m_nFresnelReflection]->GetFloatValue();
			bool hasEnvmap = params[info.m_nEnvmap]->IsTexture();

			pContextData->m_bPixelShaderFastPath = true;
			bool bUsingContrast = hasEnvmap && ( ( envmapContrast != 0.0f ) && ( envmapContrast != 1.0f ) ) && ( envmapSaturation != 1.0f );
			bool bUsingFresnel = hasEnvmap && ( fresnelReflection != 1.0f );
			bool bUsingSelfIllumTint = IS_FLAG_SET( MATERIAL_VAR_SELFILLUM ) && ( selfIllumTintVal[0] != 1.0f || selfIllumTintVal[1] != 1.0f || selfIllumTintVal[2] != 1.0f );
			if ( bUsingContrast || bUsingFresnel || bUsingSelfIllumTint || !g_pConfig->bShowSpecular )
			{
				pContextData->m_bPixelShaderFastPath = false;
			}
			if ( !pContextData->m_bPixelShaderFastPath )
			{
				params[info.m_nEnvmapContrast]->GetVecValue( pContextData->m_Constants.g_EnvmapContrast.Base(), 3 );
				params[info.m_nEnvmapSaturation]->GetVecValue( pContextData->m_Constants.g_EnvmapSaturation.Base(), 3 );

				float flFresnel = params[info.m_nFresnelReflection]->GetFloatValue();
				// [ 0, 0, 1-R(0), R(0) ]
				pContextData->m_Constants.g_FresnelReflectionReg.Init( 0, 0, 1.0 - flFresnel, flFresnel );

				pContextData->m_Constants.g_SelfIllumTint = params[info.m_nSelfIllumTint]->GetVecValue();
			}
			else
			{
				if ( bHasOutline )
				{
					float flOutlineParms[8] = { GetFloatParam( info.m_nOutlineStart0, params ),
												GetFloatParam( info.m_nOutlineStart1, params ),
												GetFloatParam( info.m_nOutlineEnd0, params ),
												GetFloatParam( info.m_nOutlineEnd1, params ),
												0,0,0,
												GetFloatParam( info.m_nOutlineAlpha, params ) };
					if ( info.m_nOutlineColor != -1 )
					{
						params[info.m_nOutlineColor]->GetVecValue( flOutlineParms + 4, 3 );
					}

					pContextData->m_Constants.g_OutlineParams[0] = &flOutlineParms[0];
					pContextData->m_Constants.g_OutlineParams[1] = &flOutlineParms[4];
				}

				if ( bHasSoftEdges )
				{
					pContextData->m_Constants.g_SoftEdgeParams.Init(
						GetFloatParam( info.m_nEdgeSoftnessStart, params ),
						GetFloatParam( info.m_nEdgeSoftnessEnd, params ),
						0, 0 );
				}
			}

			if ( bIsAlphaTested )
			{
				if ( info.m_nAlphaTestReference != -1 && params[info.m_nAlphaTestReference]->GetFloatValue() > 0.0f )
				{
					pContextData->m_Constants.g_AlphaTestRef = params[info.m_nAlphaTestReference]->GetFloatValue();
				}
			}

			// texture binds
			if ( hasBaseTexture )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER0, info.m_nBaseTexture, info.m_nBaseTextureFrame );
			}
			// handle mat_fullbright 2
			bool bLightingOnly = mat_fullbright.GetInt() == 2 && !IS_FLAG_SET( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
			if ( bLightingOnly )
			{
				// BASE TEXTURE
				if ( hasSelfIllum )
				{
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER0, TEXTURE_GREY_ALPHA_ZERO );
				}
				else
				{
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER0, TEXTURE_GREY );
				}

				// BASE TEXTURE 2	
				if ( hasBaseTexture2 )
				{
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER7, TEXTURE_GREY );
				}

				// DETAIL TEXTURE
				if ( hasDetailTexture )
				{
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER12, TEXTURE_GREY );
				}

				// disable color modulation
				pContextData->m_Constants.cModulationColor.Init();

				// turn off environment mapping
				pContextData->m_Constants.g_EnvmapTint.Init();
			}

			// always set the transform for detail textures since I'm assuming that you'll
			// always have a detailscale.
			if ( hasDetailTexture )
			{
				pShader->StoreVertexShaderTextureScaledTransform( pContextData->m_Constants.cDetailOrBumpTexCoordTransform, info.m_nBaseTextureTransform, info.m_nDetailScale );
			}

			if ( hasBaseTexture2 )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER6, info.m_nBaseTexture2, info.m_nBaseTexture2Frame );
			}

			if ( hasDetailTexture )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER2, info.m_nDetail, info.m_nDetailFrame );
			}

			if ( pContextData->m_bHasBump || hasNormalMapAlphaEnvmapMask )
			{
				if ( !g_pConfig->m_bFastNoBump )
				{
					pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER3, info.m_nBumpmap, info.m_nBumpFrame );
				}
				else
				{
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER3, TEXTURE_NORMALMAP_FLAT );
				}
			}
			if ( pContextData->m_bHasBump2 )
			{
				if ( !g_pConfig->m_bFastNoBump )
				{
					pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER5, info.m_nBumpmap2, info.m_nBumpFrame2 );
				}
				else
				{
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER5, TEXTURE_NORMALMAP_FLAT );
				}
			}
			if ( hasBumpMask )
			{
				if ( !g_pConfig->m_bFastNoBump )
				{
					pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER9, info.m_nBumpMask, -1 );
				}
				else
				{
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER9, TEXTURE_NORMALMAP_FLAT );
				}
			}

			if ( pContextData->m_bHasEnvmapMask )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER5, info.m_nEnvmapMask, info.m_nEnvmapMaskFrame );
			}

			if ( hasLightWarpTexture )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER7, info.m_nLightWarpTexture, -1 );
			}

			if ( bHasBlendModulateTexture )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER8, info.m_nBlendModulateTexture, -1 );
			}

			pContextData->m_SemiStaticCmdsOut.End();
		}
	}
	DYNAMIC_STATE
	{
		static CCommandBufferBuilder< CFixedCommandStorageBuffer< 1000 > > DynamicCmdsOut;
		DynamicCmdsOut.Reset();
		DynamicCmdsOut.Call( pContextData->m_pStaticCmds );
		DynamicCmdsOut.Call( pContextData->m_SemiStaticCmdsOut.Base() );

		// Upload the constants to the gpu
		pShaderAPI->UpdateConstantBuffer( g_hLightmappedGeneric_CBuffer, &pContextData->m_Constants );

		if ( pContextData->m_bHasEnvmap )
		{
			DynamicCmdsOut.BindTexture( pShader, SHADER_SAMPLER4, info.m_nEnvmap, info.m_nEnvmapFrame );
		}
		int nFixedLightingMode = pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_ENABLE_FIXED_LIGHTING );

		bool bVertexShaderFastPath = pContextData->m_bVertexShaderFastPath;

		if ( nFixedLightingMode != 0 )
		{
			if ( pContextData->m_bPixelShaderForceFastPathBecauseOutline )
				nFixedLightingMode = 0;
			else
				bVertexShaderFastPath = false;
		}

		MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();

		DECLARE_DYNAMIC_VERTEX_SHADER( lightmappedgeneric_vs40 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( FASTPATH, bVertexShaderFastPath );
		SET_DYNAMIC_VERTEX_SHADER_COMBO(
			LIGHTING_PREVIEW,
			( nFixedLightingMode ) ? 1 : 0
		);
		SET_DYNAMIC_VERTEX_SHADER_CMD( DynamicCmdsOut, lightmappedgeneric_vs40 );

		bool bPixelShaderFastPath = pContextData->m_bPixelShaderFastPath;
		if ( nFixedLightingMode != 0 )
		{
			bPixelShaderFastPath = false;
		}
		bool bWriteDepthToAlpha;
		bool bWriteWaterFogToAlpha;
		if ( pContextData->m_bFullyOpaque )
		{
			bWriteDepthToAlpha = pShaderAPI->ShouldWriteDepthToDestAlpha();
			bWriteWaterFogToAlpha = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			AssertMsg( !( bWriteDepthToAlpha && bWriteWaterFogToAlpha ), "Can't write two values to alpha at the same time." );
		}
		else
		{
			//can't write a special value to dest alpha if we're actually using as-intended alpha
			bWriteDepthToAlpha = false;
			bWriteWaterFogToAlpha = false;
		}

		DECLARE_DYNAMIC_PIXEL_SHADER( lightmappedgeneric_ps40 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( FASTPATH, bPixelShaderFastPath || pContextData->m_bPixelShaderForceFastPathBecauseOutline );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( FASTPATHENVMAPCONTRAST, bPixelShaderFastPath && pContextData->m_Constants.g_EnvmapContrast[0] == 1.0f );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );

		// Don't write fog to alpha if we're using translucency
		SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, bWriteDepthToAlpha );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( LIGHTING_PREVIEW, nFixedLightingMode );
		SET_DYNAMIC_PIXEL_SHADER_CMD( DynamicCmdsOut, lightmappedgeneric_ps40 );

		DynamicCmdsOut.End();
		pShaderAPI->ExecuteCommandBuffer( DynamicCmdsOut.Base() );
	}

	pShader->Draw();

	if ( IsPC() && ( IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0 ) && pContextData->m_bFullyOpaqueWithoutAlphaTest )
	{
		//Alpha testing makes it so we can't write to dest alpha
		//Writing to depth makes it so later polygons can't write to dest alpha either
		//This leads to situations with garbage in dest alpha.

		//Fix it now by converting depth to dest alpha for any pixels that just wrote.
		pShader->DrawEqualDepthToDestAlpha();
	}
}

void DrawLightmappedGeneric_DX11( CBaseVSShader *pShader, IMaterialVar **params,
				 IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow,
				 LightmappedGeneric_DX11_Vars_t &info,
				 CBasePerMaterialContextData **pContextDataPtr )
{
	bool hasFlashlight = pShader->UsingFlashlight( params );
	DrawLightmappedGeneric_DX11_Internal( pShader, params, hasFlashlight, pShaderAPI, pShaderShadow, info, pContextDataPtr );
}
