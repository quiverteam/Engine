//========= Copyright Valve Corporation, All rights reserved. ============//
//  SKIP: $BUMPMAP2 && $WARPLIGHTING
//  SKIP: $WARPLIGHTING && $DETAILTEXTURE
//	SKIP: $ENVMAPMASK && $BUMPMAP
//	SKIP: $NORMALMAPALPHAENVMAPMASK && $BASEALPHAENVMAPMASK
//	SKIP: $NORMALMAPALPHAENVMAPMASK && $ENVMAPMASK
//	SKIP: $BASEALPHAENVMAPMASK && $ENVMAPMASK
//	SKIP: $BASEALPHAENVMAPMASK && $SELFILLUM
//  SKIP: !$FASTPATH && $FASTPATHENVMAPCONTRAST
//  SKIP: !$FASTPATH && $FASTPATHENVMAPTINT
//  SKIP: !$BUMPMAP && $DIFFUSEBUMPMAP
//	SKIP: !$BUMPMAP && $BUMPMAP2
//	SKIP: $ENVMAPMASK && $BUMPMAP2
//	SKIP: $BASETEXTURENOENVMAP && ( !$BASETEXTURE2 || !$CUBEMAP )
//	SKIP: $BASETEXTURE2NOENVMAP && ( !$BASETEXTURE2 || !$CUBEMAP )
//	SKIP: $BASEALPHAENVMAPMASK && $BUMPMAP
//  SKIP: $PARALLAXMAP && $DETAILTEXTURE
//  SKIP: $SEAMLESS && $RELIEF_MAPPING
//  SKIP: $SEAMLESS && $DETAILTEXTURE
//  SKIP: $SEAMLESS && $MASKEDBLENDING
//  SKIP: $BUMPMASK && ( $SEAMLESS || $DETAILTEXTURE || $SELFILLUM || $BASETEXTURENOENVMAP || $BASETEXTURE2 )
//	SKIP: !$BUMPMAP && ($NORMAL_DECODE_MODE == 1)
//	SKIP: !$BUMPMAP && ($NORMAL_DECODE_MODE == 2)
//	SKIP: !$BUMPMAP && ($NORMALMASK_DECODE_MODE == 1)
//	SKIP: !$BUMPMAP && ($NORMALMASK_DECODE_MODE == 2)
//  NOSKIP: $FANCY_BLENDING && (!$FASTPATH)

// debug crap:
// NOSKIP: $DETAILTEXTURE
// NOSKIP: $CUBEMAP
// NOSKIP: $ENVMAPMASK
// NOSKIP: $BASEALPHAENVMAPMASK
// NOSKIP: $SELFILLUM

#define USE_32BIT_LIGHTMAPS_ON_360 //uncomment to use 32bit lightmaps, be sure to keep this in sync with the same #define in materialsystem/cmatlightmaps.cpp

#include "common_ps_fxc.h"
#include "common_flashlight_fxc.h"
#include "common_lightmappedgeneric_fxc.h"

#if SEAMLESS
#define USE_FAST_PATH 1
#else
#define USE_FAST_PATH FASTPATH
#endif

#if USE_FAST_PATH == 1

cbuffer LightmappedGeneric_PS40_FastPath : register( b1 )
{
	float4 g_EnvmapTint;
	float4 g_OutlineParams;
	float4 g_OutlineColor;
	float4 g_EdgeSoftnessParms;
};

#if FASTPATHENVMAPCONTRAST == 0
static const float3 g_EnvmapContrast = { 0.0f, 0.0f, 0.0f };
#else
static const float3 g_EnvmapContrast = { 1.0f, 1.0f, 1.0f };
#endif
static const float3 g_EnvmapSaturation = { 1.0f, 1.0f, 1.0f };
static const float g_FresnelReflection = 1.0f;
static const float g_OneMinusFresnelReflection = 0.0f;
static const float4 g_SelfIllumTint = { 1.0f, 1.0f, 1.0f, 1.0f };
#if OUTLINE
#define OUTLINE_MIN_VALUE0 g_OutlineParams.x
#define OUTLINE_MIN_VALUE1 g_OutlineParams.y
#define OUTLINE_MAX_VALUE0 g_OutlineParams.z
#define OUTLINE_MAX_VALUE1 g_OutlineParams.w

#define OUTLINE_COLOR g_OutlineColor

#endif
#if SOFTEDGES
#define SOFT_MASK_MIN g_EdgeSoftnessParms.x
#define SOFT_MASK_MAX g_EdgeSoftnessParms.y
#endif
#else

cbuffer LightmappedGeneric_PS40 : register ( b1 )
{
	float4 g_EnvmapTint;
	float3 g_EnvmapContrast;
	float3 g_EnvmapSaturation;
	float4 g_FresnelReflectionReg;
	float4 g_SelfIllumTint;
};

#define g_FresnelReflection g_FresnelReflectionReg.a
#define g_OneMinusFresnelReflection g_FresnelReflectionReg.b

#endif

cbuffer LightmappedGeneric_PS40_2 : register( b2 )
{
	float4 g_DetailTint_and_BlendFactor;
	float3 g_EyePos;
	float4 g_FogParams;
	float4 g_TintValuesAndLightmapScale;
	float4 g_FlashlightAttenuationFactors;
	float3 g_FlashlightPos;
	matrix g_FlashlightWorldToTexture;
	float4 g_ShadowTweaks;
};

#define g_DetailTint (g_DetailTint_and_BlendFactor.rgb)
#define g_DetailBlendFactor (g_DetailTint_and_BlendFactor.w)

#define g_flAlpha2 g_TintValuesAndLightmapScale.w



Texture2D BaseTexture			: register( t0 )
SamplerState BaseTextureSampler		: register( s0 );

Texture2D LightmapTexture		: register( t1 )
SamplerState LightmapSampler		: register( s1 );

TextureCube  EnvmapTexture		: register( t2 );
SamplerState EnvmapSampler		: register( s2 );
#if FANCY_BLENDING
Texture2D BlendModulationTexture	: register( t3 );
SamplerState BlendModulationSampler	: register( s3 );
#endif

#if DETAILTEXTURE
Texture2D	DetailTexture : register ( t12 );
SamplerState DetailSampler			: register( s12 );
#endif

Texture2D BumpmapTexture		: register( t4 );
SamplerState BumpmapSampler			: register( s4 );
#if NORMAL_DECODE_MODE == NORM_DECODE_ATI2N_ALPHA
Texture2D AlphaMapTexture : register( t9 );
SamplerState AlphaMapSampler		: register( s9 );	// alpha
#else
#define AlphaMapTexture		BumpmapTexture
#define AlphaMapSampler		BumpmapSampler
#endif

#if BUMPMAP2 == 1
Texture2D BumpmapTexture2 : register( t5 );
SamplerState BumpmapSampler2			: register( s5 );
#if NORMAL_DECODE_MODE == NORM_DECODE_ATI2N_ALPHA
Texture2D AlphaMapTexture2 : register( t10 );
SamplerState AlphaMapSampler2		: register( s10 );	// alpha
#else
#define AlphaMapSampler2		BumpmapSampler2
#endif
#else
Texture2D   EnvmapMaskTexture : register( t5 );
SamplerState EnvmapMaskSampler		: register( s5 );
#endif


#if WARPLIGHTING
Texture2D WarpLightingTexture : register( t6 );
SamplerState WarpLightingSampler		: register( s6 );
#endif
Texture2D BaseTexture2 : register( t7 );
SamplerState BaseTextureSampler2		: register( s7 );

#if BUMPMASK == 1
Texture2D BumpMaskTexture : register( t8 );
SamplerState BumpMaskSampler			: register( s8 );
#if NORMALMASK_DECODE_MODE == NORM_DECODE_ATI2N_ALPHA
Texture2D AlphaMaskTexture : register( t11 );
SamplerState AlphaMaskSampler		: register( s11 );	// alpha
#else
#define AlphaMaskTexture		BumpMaskTexture
#define AlphaMaskSampler		BumpMaskSampler
#endif
#endif

struct PS_INPUT
{
#if SEAMLESS
	float4 SeamlessTexCoord_VertexBlend		: TEXCOORD0;            // zy xz
#else
	float3 baseTexCoord_VertexBlend			: TEXCOORD0;
#endif
	float4 detailOrBumpAndEnvmapMaskTexCoord: TEXCOORD1;
// CENTROID: TEXCOORD2
	float4 lightmapTexCoord1And2			: TEXCOORD2;
// CENTROID: TEXCOORD3
	float4 lightmapTexCoord3				: TEXCOORD3;
	float4 worldPos_projPosZ				: TEXCOORD4;
	float3x3 tangentSpaceTranspose			: TEXCOORD5;
	// tangentSpaceTranspose				: TEXCOORD6
	// tangentSpaceTranspose				: TEXCOORD7

	float4 vertexColor						: COLOR;
};

#if LIGHTING_PREVIEW == 2
LPREVIEW_PS_OUT main( PS_INPUT i ) : SV_TARGET
#else
float4 main( PS_INPUT i ) : SV_TARGET
#endif
{
	bool bBaseTexture2 = BASETEXTURE2 ? true : false;
	bool bDetailTexture = DETAILTEXTURE ? true : false;
	bool bBumpmap = BUMPMAP ? true : false;
	bool bDiffuseBumpmap = DIFFUSEBUMPMAP ? true : false;
	bool bCubemap = CUBEMAP ? true : false;
	bool bEnvmapMask = ENVMAPMASK ? true : false;
	bool bBaseAlphaEnvmapMask = BASEALPHAENVMAPMASK ? true : false;
	bool bSelfIllum = SELFILLUM ? true : false;
	bool bNormalMapAlphaEnvmapMask = NORMALMAPALPHAENVMAPMASK ? true : false;
	bool bBaseTextureNoEnvmap = BASETEXTURENOENVMAP ? true : false;
	bool bBaseTexture2NoEnvmap = BASETEXTURE2NOENVMAP ? true : false;

	float4 baseColor = 0.0f;
	float4 baseColor2 = 0.0f;
	float4 vNormal = float4(0, 0, 1, 1);
	float3 baseTexCoords = float3(0,0,0);

#if SEAMLESS
	baseTexCoords = i.SeamlessTexCoord_VertexBlend.xyz;
#else
	baseTexCoords.xy = i.baseTexCoord_VertexBlend.xy;
#endif

	GetBaseTextureAndNormal( BaseTexture, BaseTextureSampler, BaseTexture2, BaseTextureSampler2, BumpmapTexture, BumpmapSampler, bBaseTexture2, bBumpmap || bNormalMapAlphaEnvmapMask, 
		baseTexCoords, i.vertexColor.rgb, baseColor, baseColor2, vNormal );

#if BUMPMAP == 1	// not ssbump
	vNormal.xyz = vNormal.xyz * 2.0f - 1.0f;					// make signed if we're not ssbump
#endif

	float3 lightmapColor1 = float3( 1.0f, 1.0f, 1.0f );
	float3 lightmapColor2 = float3( 1.0f, 1.0f, 1.0f );
	float3 lightmapColor3 = float3( 1.0f, 1.0f, 1.0f );
#if LIGHTING_PREVIEW == 0
	if( bBumpmap && bDiffuseBumpmap )
	{
		float2 bumpCoord1;
		float2 bumpCoord2;
		float2 bumpCoord3;
		ComputeBumpedLightmapCoordinates( i.lightmapTexCoord1And2, i.lightmapTexCoord3.xy,
			bumpCoord1, bumpCoord2, bumpCoord3 );
		
		lightmapColor1 = LightMapSample( LightmapTexture, LightmapSampler, bumpCoord1 );
		lightmapColor2 = LightMapSample( LightmapTexture, LightmapSampler, bumpCoord2 );
		lightmapColor3 = LightMapSample( LightmapTexture, LightmapSampler, bumpCoord3 );
	}
	else
	{
		float2 bumpCoord1 = ComputeLightmapCoordinates( i.lightmapTexCoord1And2, i.lightmapTexCoord3.xy );
		lightmapColor1 = LightMapSample( LightmapTexture, LightmapSampler, bumpCoord1 );
	}
#endif

#if RELIEF_MAPPING
	// in the parallax case, all texcoords must be the same in order to free
    // up an iterator for the tangent space view vector
	float2 detailTexCoord = i.baseTexCoord_VertexBlend.xy;
	float2 bumpmapTexCoord = i.baseTexCoord_VertexBlend.xy;
	float2 envmapMaskTexCoord = i.baseTexCoord_VertexBlend.xy;
#else

	#if ( DETAILTEXTURE == 1 )
		float2 detailTexCoord = i.detailOrBumpAndEnvmapMaskTexCoord.xy;
		float2 bumpmapTexCoord = i.baseTexCoord_VertexBlend.xy;
	#elif ( BUMPMASK == 1 )
		float2 detailTexCoord = 0.0f;
		float2 bumpmapTexCoord = i.detailOrBumpAndEnvmapMaskTexCoord.xy;
		float2 bumpmap2TexCoord = i.detailOrBumpAndEnvmapMaskTexCoord.wz;
	#else
		float2 detailTexCoord = 0.0f;
		float2 bumpmapTexCoord = i.detailOrBumpAndEnvmapMaskTexCoord.xy;
	#endif

	float2 envmapMaskTexCoord = i.detailOrBumpAndEnvmapMaskTexCoord.wz;
#endif // !RELIEF_MAPPING

	float4 detailColor = float4( 1.0f, 1.0f, 1.0f, 1.0f );
#if DETAILTEXTURE

#if SHADER_MODEL_PS_2_0
	detailColor = DetailTexture.Sample( DetailSampler, detailTexCoord );
#else
	detailColor = float4( g_DetailTint, 1.0f ) * DetailTexture.Sample( DetailSampler, detailTexCoord );
#endif

#endif

#if ( OUTLINE || SOFTEDGES )
	float distAlphaMask = baseColor.a;

#   if OUTLINE
	if ( ( distAlphaMask >= OUTLINE_MIN_VALUE0 ) &&
		 ( distAlphaMask <= OUTLINE_MAX_VALUE1 ) )
	{
		float oFactor=1.0;
		if ( distAlphaMask <= OUTLINE_MIN_VALUE1 )
		{
			oFactor=smoothstep( OUTLINE_MIN_VALUE0, OUTLINE_MIN_VALUE1, distAlphaMask );
		}
		else
		{
			oFactor=smoothstep( OUTLINE_MAX_VALUE1, OUTLINE_MAX_VALUE0, distAlphaMask );
		}
		baseColor = lerp( baseColor, OUTLINE_COLOR, oFactor );
	}
#   endif
#   if SOFTEDGES
	baseColor.a *= smoothstep( SOFT_MASK_MAX, SOFT_MASK_MIN, distAlphaMask );
#   else
	baseColor.a *= distAlphaMask >= 0.5;
#   endif
#endif


#if LIGHTING_PREVIEW == 2
	baseColor.xyz=GammaToLinear(baseColor.xyz);
#endif

	float blendedAlpha = baseColor.a;

#if MASKEDBLENDING
	float blendfactor=0.5;
#elif SEAMLESS
	float blendfactor=i.SeamlessTexCoord_VertexBlend.w;
#else
	float blendfactor=i.baseTexCoord_VertexBlend.z;
#endif

	if( bBaseTexture2 )
	{
#if (SELFILLUM == 0) && (PIXELFOGTYPE != PIXEL_FOG_TYPE_HEIGHT) && (FANCY_BLENDING)
		float4 modt=BlendModulationTexture.Sample(BlendModulationSampler,i.lightmapTexCoord3.zw);
#if MASKEDBLENDING
		// FXC is unable to optimize this, despite blendfactor=0.5 above
		//float minb=modt.g-modt.r;
		//float maxb=modt.g+modt.r;
		//blendfactor=smoothstep(minb,maxb,blendfactor);
		//blendfactor=modt.g;
		float minb=modt.g-modt.r;
		float maxb=modt.g+modt.r;
#else
		float minb=max(0,modt.g-modt.r);
		float maxb=min(1,modt.g+modt.r);
		//float minb=saturate(modt.g-modt.r);
		//float maxb=saturate(modt.g+modt.r);
#endif
		blendfactor=smoothstep(minb,maxb,blendfactor);
//#endif
#endif
		//baseColor.rgb = lerp( baseColor.rgb, baseColor2.rgb, blendfactor );
		baseColor.rgb = lerp( baseColor, baseColor2.rgb, blendfactor );
		blendedAlpha = lerp( baseColor.a, baseColor2.a, blendfactor );
	}

	float3 specularFactor = 1.0f;
	float4 vNormalMask = float4(0, 0, 1, 1);
	if( bBumpmap )
	{
		if( bBaseTextureNoEnvmap )
		{
			vNormal.a = 0.0f;
		}

#if ( BUMPMAP2 == 1 )
		{
	#if ( BUMPMASK == 1 )
			float2 b2TexCoord = bumpmap2TexCoord;
	#else
			float2 b2TexCoord = bumpmapTexCoord;
	#endif

			float4 vNormal2;
			if ( BUMPMAP == 2 )
				vNormal2 = BumpmapTexture2.Sample( BumpmapSampler2, b2TexCoord );
			else
				vNormal2 = DecompressNormal( BumpmapTexture2, BumpmapSampler2, b2TexCoord, NORMAL_DECODE_MODE, AlphaMapTexture2, AlphaMapSampler2 );		// Bump 2 coords

			if( bBaseTexture2NoEnvmap )
			{
				vNormal2.a = 0.0f;
			}

	#if ( BUMPMASK == 1 )
			float3 vNormal1 = DecompressNormal( BumpmapTexture, BumpmapSampler, i.detailOrBumpAndEnvmapMaskTexCoord.xy, NORMALMASK_DECODE_MODE, AlphaMaskTexture, AlphaMapSampler );

			vNormal.xyz = normalize( vNormal1.xyz + vNormal2.xyz );

			// Third normal map...same coords as base
			vNormalMask = DecompressNormal( BumpMaskTexture, BumpMaskSampler, i.baseTexCoord_VertexBlend.xy, NORMALMASK_DECODE_MODE, AlphaMaskTexture, AlphaMaskSampler );

			vNormal.xyz = lerp( vNormalMask.xyz, vNormal.xyz, vNormalMask.a );		// Mask out normals from vNormal
			specularFactor = vNormalMask.a;
	#else // BUMPMASK == 0
			if ( FANCY_BLENDING && bNormalMapAlphaEnvmapMask )
			{
				vNormal = lerp( vNormal, vNormal2, blendfactor);
			}
			else
			{
				vNormal.xyz = lerp( vNormal.xyz, vNormal2.xyz, blendfactor);
			}

	#endif

		}

#endif // BUMPMAP2 == 1

		if( bNormalMapAlphaEnvmapMask )
		{
			specularFactor *= vNormal.a;
		}
	}
	else if ( bNormalMapAlphaEnvmapMask )
	{
		specularFactor *= vNormal.a;
	}

#if ( BUMPMAP2 == 0 )
	if( bEnvmapMask )
	{
		specularFactor *= EnvmapMaskTexture.Sample( EnvmapMaskSampler, envmapMaskTexCoord ).xyz;	
	}
#endif

	if( bBaseAlphaEnvmapMask )
	{
		specularFactor *= 1.0 - blendedAlpha; // Reversing alpha blows!
	}
	float4 albedo = float4( 1.0f, 1.0f, 1.0f, 1.0f );
	float alpha = 1.0f;
	albedo *= baseColor;
	if( !bBaseAlphaEnvmapMask && !bSelfIllum )
	{
		alpha *= baseColor.a;
	}

	if( bDetailTexture )
	{
		albedo = TextureCombine( albedo, detailColor, DETAIL_BLEND_MODE, g_DetailBlendFactor );
	}

	// The vertex color contains the modulation color + vertex color combined
#if ( SEAMLESS == 0 )
	albedo.xyz *= i.vertexColor.xyz;
#endif
	alpha *= i.vertexColor.a * g_flAlpha2; // not sure about this one

	// Save this off for single-pass flashlight, since we'll still need the SSBump vector, not a real normal
	float3 vSSBumpVector = vNormal.xyz;

	float3 diffuseLighting;
	if( bBumpmap && bDiffuseBumpmap )
	{
// ssbump
#if ( BUMPMAP == 2 )
		diffuseLighting = vNormal.x * lightmapColor1 +
						  vNormal.y * lightmapColor2 +
						  vNormal.z * lightmapColor3;
		diffuseLighting *= g_TintValuesAndLightmapScale.rgb;

		// now, calculate vNormal for reflection purposes. if vNormal isn't needed, hopefully
		// the compiler will eliminate these calculations
		vNormal.xyz = normalize( bumpBasis[0]*vNormal.x + bumpBasis[1]*vNormal.y + bumpBasis[2]*vNormal.z);
#else
		float3 dp;
		dp.x = saturate( dot( vNormal.xyz, bumpBasis[0] ) );
		dp.y = saturate( dot( vNormal.xyz, bumpBasis[1] ) );
		dp.z = saturate( dot( vNormal.xyz, bumpBasis[2] ) );
		dp *= dp;
		
#if ( DETAIL_BLEND_MODE == TCOMBINE_SSBUMP_BUMP )
		dp *= 2*detailColor;
#endif
		diffuseLighting = dp.x * lightmapColor1 +
						  dp.y * lightmapColor2 +
						  dp.z * lightmapColor3;
		float sum = dot( dp, float3( 1.0f, 1.0f, 1.0f ) );
		diffuseLighting *= g_TintValuesAndLightmapScale.rgb / sum;
#endif
	}
	else
	{
		diffuseLighting = lightmapColor1 * g_TintValuesAndLightmapScale.rgb;
	}

#if WARPLIGHTING && ( SEAMLESS == 0 )
	float len=0.5*length(diffuseLighting);
	// FIXME: 8-bit lookup textures like this need a "nice filtering" VTF option, which converts
	//        them to 16-bit on load or does filtering in the shader (since most hardware - 360
	//        included - interpolates 8-bit textures at 8-bit precision, which causes banding)
	diffuseLighting *= 2.0*WarpLightingTexture.Sample(WarpLightingSampler,float2(len,0));
#endif

#if CUBEMAP || LIGHTING_PREVIEW || ( defined( _X360 ) && FLASHLIGHT )
	float3 worldSpaceNormal = mul( vNormal.xyz, i.tangentSpaceTranspose );
#endif

	float3 diffuseComponent = albedo.xyz * diffuseLighting;

#if defined( _X360 ) && FLASHLIGHT

	// ssbump doesn't pass a normal to the flashlight...it computes shadowing a different way
#if ( BUMPMAP == 2 )
	bool bHasNormal = false;

	float3 worldPosToLightVector = g_FlashlightPos - i.worldPos_projPosZ.xyz;

	float3 tangentPosToLightVector;
	tangentPosToLightVector.x = dot( worldPosToLightVector, i.tangentSpaceTranspose[0] );
	tangentPosToLightVector.y = dot( worldPosToLightVector, i.tangentSpaceTranspose[1] );
	tangentPosToLightVector.z = dot( worldPosToLightVector, i.tangentSpaceTranspose[2] );

	tangentPosToLightVector = normalize( tangentPosToLightVector );

	float nDotL = saturate( vSSBumpVector.x*dot( tangentPosToLightVector, bumpBasis[0]) +
							vSSBumpVector.y*dot( tangentPosToLightVector, bumpBasis[1]) +
							vSSBumpVector.z*dot( tangentPosToLightVector, bumpBasis[2]) );
#else
	bool bHasNormal = true;
	float nDotL = 1.0f;
#endif

	float fFlashlight = DoFlashlight( g_FlashlightPos, i.worldPos_projPosZ.xyz, i.flashlightSpacePos,
		worldSpaceNormal, g_FlashlightAttenuationFactors.xyz, 
		g_FlashlightAttenuationFactors.w, FlashlightSampler, ShadowDepthSampler,
		RandRotSampler, 0, true, false, i.vProjPos.xy / i.vProjPos.w, false, g_ShadowTweaks, bHasNormal );

	diffuseComponent = albedo.xyz * ( diffuseLighting + ( fFlashlight * nDotL ) );
#endif

	if( bSelfIllum )
	{
		float3 selfIllumComponent = g_SelfIllumTint.xyz * albedo.xyz;
		diffuseComponent = lerp( diffuseComponent, selfIllumComponent, baseColor.a );
	}

	float3 specularLighting = float3( 0.0f, 0.0f, 0.0f );
#if CUBEMAP
	if( bCubemap )
	{
		float3 worldVertToEyeVector = g_EyePos - i.worldPos_projPosZ.xyz;
		float3 reflectVect = CalcReflectionVectorUnnormalized( worldSpaceNormal, worldVertToEyeVector );

		// Calc Fresnel factor
		half3 eyeVect = normalize(worldVertToEyeVector);
		float fresnel = 1.0 - dot( worldSpaceNormal, eyeVect );
		fresnel = pow( fresnel, 5.0 );
		fresnel = fresnel * g_OneMinusFresnelReflection + g_FresnelReflection;

		specularLighting = ENV_MAP_SCALE * EnvmapTexture.Sample( EnvmapSampler, reflectVect ).rgb;
		specularLighting *= specularFactor;
								   
		specularLighting *= g_EnvmapTint.rgb;
#if FANCY_BLENDING == 0
		float3 specularLightingSquared = specularLighting * specularLighting;
		specularLighting = lerp( specularLighting, specularLightingSquared, g_EnvmapContrast );
		float3 greyScale = dot( specularLighting, float3( 0.299f, 0.587f, 0.114f ) );
		specularLighting = lerp( greyScale, specularLighting, g_EnvmapSaturation );
#endif
		specularLighting *= fresnel;
	}
#endif

	float3 result = diffuseComponent + specularLighting;
	
#if LIGHTING_PREVIEW
#	if LIGHTING_PREVIEW == 1
	float dotprod = 0.7+0.25 * dot( worldSpaceNormal, normalize( float3( 1, 2, -.5 ) ) );
	return FinalOutput( float4( dotprod*albedo.xyz, alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
#	else
	LPREVIEW_PS_OUT ret;
	ret.color = float4( albedo.xyz,alpha );
	ret.normal = float4( worldSpaceNormal,alpha );
	ret.position = float4( i.worldPos_projPosZ.xyz, alpha );
	ret.flags = float4( 1, 1, 1, alpha );

	return FinalOutput( ret, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );	
#	endif
#else // == end LIGHTING_PREVIEW ==

	bool bWriteDepthToAlpha = false;

	// ps_2_b and beyond
#if !(defined(SHADER_MODEL_PS_1_1) || defined(SHADER_MODEL_PS_1_4) || defined(SHADER_MODEL_PS_2_0))
	bWriteDepthToAlpha = ( WRITE_DEPTH_TO_DESTALPHA != 0 ) && ( WRITEWATERFOGTODESTALPHA == 0 );
#endif

	//float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos.xyz, i.worldPos_projPosZ.xyz, i.worldPos_projPosZ.w );
	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );

#if WRITEWATERFOGTODESTALPHA && (PIXELFOGTYPE == PIXEL_FOG_TYPE_HEIGHT)
	alpha = fogFactor;
#endif

	return FinalOutput( float4( result.rgb, alpha ), fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR, bWriteDepthToAlpha, i.worldPos_projPosZ.w );

#endif
}
 
