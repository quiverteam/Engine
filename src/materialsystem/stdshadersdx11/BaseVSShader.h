//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
// This is what all vs/ps (dx8+) shaders inherit from.
//===========================================================================//

#ifndef BASEVSSHADER_H
#define BASEVSSHADER_H

#ifdef _WIN32		   
#pragma once
#endif

#include "shaderlib/cshader.h"
#include "shaderlib/baseshader.h"
#include "shader_register_map.h"
#include "ConVar.h"
#include <renderparm.h>

#define SUPPORT_DX8 0
#define SUPPORT_DX7 0
//-----------------------------------------------------------------------------
// Helper macro for vertex shaders
//-----------------------------------------------------------------------------
#define BEGIN_VS_SHADER_FLAGS(_name, _help, _flags)	__BEGIN_SHADER_INTERNAL( CBaseVSShader, _name, _help, _flags )
#define BEGIN_VS_SHADER(_name,_help)	__BEGIN_SHADER_INTERNAL( CBaseVSShader, _name, _help, 0 )


// useful parameter initialization macro
#define INIT_FLOAT_PARM( parm, value )					\
		if ( !params[(parm)]->IsDefined() )				\
		{												\
			params[(parm)]->SetFloatValue( (value) );	\
		}


//-----------------------------------------------------------------------------
// Base class for shaders, contains helper methods.
//-----------------------------------------------------------------------------
class CBaseVSShader : public CBaseShader
{
public:

	// Sets up ambient light cube...
	void SetAmbientCubeDynamicStateVertexShader( );
	float GetAmbientLightCubeLuminance( );

	// Helpers for dealing with envmaptint
	void SetEnvMapTintPixelShaderDynamicState( int pixelReg, int tintVar, int alphaVar, bool bConvertFromGammaToLinear = false );
	
	// Helper methods for pixel shader overbrighting
	void EnablePixelShaderOverbright( int reg, bool bEnable, bool bDivideByTwo );

	// Helper for dealing with modulation
	void SetModulationDynamicState();
	void SetModulationDynamicState_LinearColorSpace();
	void SetModulationDynamicState_LinearColorSpace_LinearScale( float flScale );

	//
	// Standard shader passes!
	//

	void InitParamsUnlitGeneric_DX8( 
		int baseTextureVar,
		int detailScaleVar,
		int envmapOptionalVar,
		int envmapVar,
		int envmapTintVar, 
		int envmapMaskScaleVar,
		int nDetailBlendMode );

	void InitUnlitGeneric_DX8( 
		int baseTextureVar,
		int detailVar,
		int envmapVar,
		int envmapMaskVar );

	// Dx8 Unlit Generic pass
	void VertexShaderUnlitGenericPass( int baseTextureVar, int frameVar, 
									   int baseTextureTransformVar, 
									   int detailVar, int detailTransform, bool bDetailTransformIsScale, 
									   int envmapVar, int envMapFrameVar, int envmapMaskVar,
									   int envmapMaskFrameVar, int envmapMaskScaleVar, int envmapTintVar,
									   int alphaTestReferenceVar,
									   int nDetailBlendModeVar,
									   int nOutlineVar,
									   int nOutlineColorVar,
									   int nOutlineStartVar,
									   int nOutlineEndVar,
									   int nSeparateDetailUVsVar
									   );

	// Helpers for drawing world bump mapped stuff.
	void DrawModelBumpedSpecularLighting( int bumpMapVar, int bumpMapFrameVar,
											   int envMapVar, int envMapVarFrame,
											   int envMapTintVar, int alphaVar,
											   int envMapContrastVar, int envMapSaturationVar,
											   int bumpTransformVar,
											   bool bBlendSpecular, bool bNoWriteZ = false );
	void DrawWorldBumpedSpecularLighting( int bumpmapVar, int envmapVar,
											   int bumpFrameVar, int envmapFrameVar,
											   int envmapTintVar, int alphaVar,
											   int envmapContrastVar, int envmapSaturationVar,
											   int bumpTransformVar, int fresnelReflectionVar,
											   bool bBlend, bool bNoWriteZ = false );

	const char *UnlitGeneric_ComputeVertexShaderName( bool bMask,
													  bool bEnvmap,
													  bool bBaseTexture,
													  bool bBaseAlphaEnvmapMask,
													  bool bDetail,
													  bool bVertexColor,
													  bool bEnvmapCameraSpace,
													  bool bEnvmapSphere );

	const char *UnlitGeneric_ComputePixelShaderName( bool bMask,
													 bool bEnvmap,
													 bool bBaseTexture,
													 bool bBaseAlphaEnvmapMask,
													 bool bDetail,
													 bool bMultiplyDetail,
													 bool bMaskBaseByDetailAlpha );

	void DrawWorldBaseTexture( int baseTextureVar, int baseTextureTransformVar, int frameVar, int colorVar, int alphaVar );
	void DrawWorldBumpedDiffuseLighting( int bumpmapVar, int bumpFrameVar,
		int bumpTransformVar, bool bMultiply, bool bSSBump  );
	void DrawWorldBumpedSpecularLighting( int envmapMaskVar, int envmapMaskFrame,
		int bumpmapVar, int envmapVar,
		int bumpFrameVar, int envmapFrameVar,
		int envmapTintVar, int alphaVar,
		int envmapContrastVar, int envmapSaturationVar,
		int bumpTransformVar,  int fresnelReflectionVar,
		bool bBlend );
	void DrawBaseTextureBlend( int baseTextureVar, int baseTextureTransformVar, 
		int baseTextureFrameVar,
		int baseTexture2Var, int baseTextureTransform2Var, 
		int baseTextureFrame2Var, int colorVar, int alphaVar );
	void DrawWorldBumpedDiffuseLighting_Base_ps14( int bumpmapVar, int bumpFrameVar,
		int bumpTransformVar, int baseTextureVar, int baseTextureTransformVar, int frameVar );
	void DrawWorldBumpedDiffuseLighting_Blend_ps14( int bumpmapVar, int bumpFrameVar, int bumpTransformVar, 
		int baseTextureVar, int baseTextureTransformVar, int baseTextureFrameVar, 
		int baseTexture2Var, int baseTextureTransform2Var, int baseTextureFrame2Var);
	/*void DrawWorldBumpedUsingVertexShader( int baseTextureVar, int baseTextureTransformVar,
										   int bumpmapVar, int bumpFrameVar, 
										   int bumpTransformVar,
										   int envmapMaskVar, int envmapMaskFrame,
										   int envmapVar, 
										   int envmapFrameVar,
										   int envmapTintVar, int colorVar, int alphaVar,
										   int envmapContrastVar, int envmapSaturationVar, int frameVar, int fresnelReflectionVar,
										   bool doBaseTexture2,
										   int baseTexture2Var,
										   int baseTextureTransform2Var,
										   int baseTextureFrame2Var,
										   bool bSSBump
		);*/
	
	// Sets up hw morphing state for the vertex shader
	void SetHWMorphVertexShaderState( Vector4D &dimensions, Vector4D &subrect, VertexTextureSampler_t morphSampler );

	// Computes the shader index for vertex lit materials
	int ComputeVertexLitShaderIndex( bool bVertexLitGeneric, bool hasBump, bool hasEnvmap, bool hasVertexColor, bool bHasNormal ) const;

	BlendType_t EvaluateBlendRequirements( int textureVar, bool isBaseTexture, int detailTextureVar = -1 );

	struct DrawFlashlight_dx90_Vars_t
	{
		DrawFlashlight_dx90_Vars_t() 
		{ 
			// set all ints to -1
			memset( this, 0xFF, sizeof(DrawFlashlight_dx90_Vars_t) ); 
			// set all bools to a default value.
			m_bBump = false;
			m_bLightmappedGeneric = false;
			m_bWorldVertexTransition = false;
			m_bTeeth = false;
			m_bSSBump = false;
			m_fSeamlessScale = 0.0;
		}
		bool m_bBump;
		bool m_bLightmappedGeneric;
		bool m_bWorldVertexTransition;
		bool m_bTeeth;
		int m_nBumpmapVar;
		int m_nBumpmapFrame;
		int m_nBumpTransform;
		int m_nFlashlightTextureVar;
		int m_nFlashlightTextureFrameVar;
		int m_nBaseTexture2Var;
		int m_nBaseTexture2FrameVar;
		int m_nBumpmap2Var;
		int m_nBumpmap2Frame;
		int m_nBump2Transform;
		int m_nDetailVar;
		int m_nDetailScale;
		int m_nDetailTextureCombineMode;
		int m_nDetailTextureBlendFactor;
		int m_nDetailTint;
		int m_nTeethForwardVar;
		int m_nTeethIllumFactorVar;
		int m_nAlphaTestReference;
		bool m_bSSBump;
		float m_fSeamlessScale;								// 0.0 = not seamless
	};
	void DrawFlashlight_dx90( IMaterialVar** params, 
		IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, DrawFlashlight_dx90_Vars_t &vars );

	void HashShadow2DJitter( const float fJitterSeed, float *fU, float* fV );

	//Alpha tested materials can end up leaving garbage in the dest alpha buffer if they write depth. 
	//This pass fills in the areas that passed the alpha test with depth in dest alpha 
	//by writing only equal depth pixels and only if we should be writing depth to dest alpha
	void DrawEqualDepthToDestAlpha( void );
	
private:
	// Helper methods for VertexLitGenericPass
//	void UnlitGenericShadowState( int baseTextureVar, int detailVar, int envmapVar, int envmapMaskVar, bool doSkin );
	void UnlitGenericDynamicState( int baseTextureVar, int frameVar, int baseTextureTransformVar,
		int detailVar, int detailTransform, bool bDetailTransformIsScale, int envmapVar, 
		int envMapFrameVar, int envmapMaskVar, int envmapMaskFrameVar,
		int envmapMaskScaleVar, int envmapTintVar );

	// Converts a color + alpha into a vector4
	void ColorVarsToVector( int colorVar, int alphaVar, Vector4D &color );

};

FORCEINLINE void SetFlashLightColorFromState( FlashlightState_t const &state, IShaderDynamicAPI *pShaderAPI, int nPSRegister=28, bool bFlashlightNoLambert=false )
{
	// Old code
	//float flToneMapScale = ( pShaderAPI->GetToneMappingScaleLinear() ).x;
	//float flFlashlightScale = 1.0f / flToneMapScale;

	// Fix to old code to keep flashlight from ever getting brighter than 1.0
	//float flToneMapScale = ( pShaderAPI->GetToneMappingScaleLinear() ).x;
	//if ( flToneMapScale < 1.0f )
	//	flToneMapScale = 1.0f;
	//float flFlashlightScale = 1.0f / flToneMapScale;

	// Force flashlight to 25% bright always
	float flFlashlightScale = 0.25f;

	if ( !g_pHardwareConfig->GetHDREnabled() )
	{
		// Non-HDR path requires 2.0 flashlight
		flFlashlightScale = 2.0f;
	}

	// DX10 requires some hackery due to sRGB/blend ordering change from DX9
	if ( g_pHardwareConfig->UsesSRGBCorrectBlending() )
	{
		flFlashlightScale *= 2.5f; // Magic number that works well on the NVIDIA 8800
	}

	// Generate pixel shader constant
	float const *pFlashlightColor = state.m_Color;
	float vPsConst[4] = { flFlashlightScale * pFlashlightColor[0], flFlashlightScale * pFlashlightColor[1], flFlashlightScale * pFlashlightColor[2], pFlashlightColor[3] };
	vPsConst[3] = bFlashlightNoLambert ? 2.0f : 0.0f; // This will be added to N.L before saturate to force a 1.0 N.L term

	// Red flashlight for testing
	//vPsConst[0] = 0.5f; vPsConst[1] = 0.0f; vPsConst[2] = 0.0f;

	pShaderAPI->SetPixelShaderConstant( nPSRegister, ( float * )vPsConst );
}

FORCEINLINE float ShadowAttenFromState( FlashlightState_t const &state )
{
	// DX10 requires some hackery due to sRGB/blend ordering change from DX9, which makes the shadows too light
	if ( g_pHardwareConfig->UsesSRGBCorrectBlending() )
		return state.m_flShadowAtten * 0.1f; // magic number

	return state.m_flShadowAtten;
}

FORCEINLINE float ShadowFilterFromState( FlashlightState_t const &state )
{
	return state.m_flShadowFilterSize / state.m_flShadowMapResolution;
}

// convenient material variable access functions for helpers to use.
FORCEINLINE bool IsTextureSet( int nVar, IMaterialVar **params )
{
	return ( nVar != -1 ) && ( params[nVar]->IsTexture() );
}

FORCEINLINE bool IsBoolSet( int nVar, IMaterialVar **params )
{
	return ( nVar != -1 ) && ( params[nVar]->GetIntValue() );
}

FORCEINLINE int GetIntParam( int nVar, IMaterialVar **params, int nDefaultValue = 0 )
{
	return ( nVar != -1 ) ? ( params[nVar]->GetIntValue() ) : nDefaultValue;
}

FORCEINLINE float GetFloatParam( int nVar, IMaterialVar **params, float flDefaultValue = 0.0 )
{
	return ( nVar != -1 ) ? ( params[nVar]->GetFloatValue() ) : flDefaultValue;
}

FORCEINLINE void InitFloatParam( int nIndex, IMaterialVar **params, float flValue )
{
	if ( (nIndex != -1) && !params[nIndex]->IsDefined() )
	{
		params[nIndex]->SetFloatValue( flValue );
	}
}

FORCEINLINE void InitIntParam( int nIndex, IMaterialVar **params, int nValue )
{
	if ( (nIndex != -1) && !params[nIndex]->IsDefined() )
	{
		params[nIndex]->SetIntValue( nValue );
	}
}


class ConVar;

#ifdef _DEBUG
extern ConVar mat_envmaptintoverride;
extern ConVar mat_envmaptintscale;
#endif


#endif // BASEVSSHADER_H
