//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VERTEXLITGENERIC_DX11_HELPER_H
#define VERTEXLITGENERIC_DX11_HELPER_H

#include <string.h>


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseVSShader;
class IMaterialVar;
class IShaderDynamicAPI;
class IShaderShadow;

//-----------------------------------------------------------------------------
// Structure that defines that constants used by the shader
//-----------------------------------------------------------------------------
ALIGN16 struct VertexLitGeneric_CBuffer_t
{
	// Vertex shader
	Vector4D cBaseTextureTransform[2];
	Vector4D cDetailTextureTransform[2];
	Vector4D cMorphDimensions;
	Vector4D cMorphSubrect;
	Vector4D cSeamlessScale;
	Vector4D cModulationColor;

	// Pixel shader
	Vector4D cDetailTint;
	Vector4D cGlowParams;
	Vector4D cGlowColor;
	Vector4D cMaskRangeParams;
	Vector4D cOutlineColor;
	Vector4D cOutlineParams;
	Vector4D cEnvMapSaturation_SelfIllumMask;
	Vector4D cEnvMapTint;
	Vector4D cConstScaleBiasExp;
	Vector4D cEnvMapContrast;
	Vector4D cSelfIllumTint;
	Vector4D cDepthFeathering;
	Vector4D cShaderControls;

	// Pixel shader (phong)
	Vector4D cFresnelSpecParams;
	Vector4D cSpecularRimParams;
	Vector4D cPhongRimParams;
	Vector4D cSpecExponent;
};

extern ConstantBufferHandle_t g_hVertexLitGeneric_CBuffer;

//-----------------------------------------------------------------------------
// Init params/ init/ draw methods
//-----------------------------------------------------------------------------
struct VertexLitGeneric_DX11_Vars_t
{
	VertexLitGeneric_DX11_Vars_t()
	{
		memset( this, 0xFF, sizeof( *this ) );
	}

	int m_nBaseTexture;
	int m_nWrinkle;
	int m_nStretch;
	int m_nBaseTextureFrame;
	int m_nBaseTextureTransform;
	int m_nAlbedo;
	int m_nDetail;
	int m_nDetailFrame;
	int m_nDetailScale;
	int m_nEnvmap;
	int m_nEnvmapFrame;
	int m_nEnvmapMask;
	int m_nEnvmapMaskFrame;
	int m_nEnvmapMaskTransform;
	int m_nEnvmapTint;
	int m_nBumpmap;
	int m_nNormalWrinkle;
	int m_nNormalStretch;
	int m_nBumpFrame;
	int m_nBumpTransform;
	int m_nEnvmapContrast;
	int m_nEnvmapSaturation;
	int m_nAlphaTestReference;
	int m_nVertexAlphaTest;
	int m_nFlashlightNoLambert;
	int m_nFlashlightTexture;
	int m_nFlashlightTextureFrame;

	int m_nSelfIllumTint;
	int m_nSelfIllumFresnel;
	int m_nSelfIllumFresnelMinMaxExp;

	int m_nPhongExponent;
	int m_nPhongTint;
	int m_nPhongAlbedoTint;
	int m_nPhongExponentTexture;
	int m_nDiffuseWarpTexture;
	int m_nPhongWarpTexture;
	int m_nPhongBoost;
	int m_nPhongExponentFactor;
	int m_nPhongFresnelRanges;
	int m_nSelfIllumEnvMapMask_Alpha;
	int m_nAmbientOnly;
	int m_nHDRColorScale;
	int m_nPhong;
	int m_nBaseMapAlphaPhongMask;
	int m_nEnvmapFresnel;

	int m_nDetailTextureCombineMode;
	int m_nDetailTextureBlendFactor;

	// Rim lighting parameters
	int m_nRimLight;
	int m_nRimLightPower;
	int m_nRimLightBoost;
	int m_nRimMask;

	int m_nSeamlessScale;
	int m_nSeamlessBase;
	int m_nSeamlessDetail;

	// distance coded line art parameters
	int m_nDistanceAlpha;
	int m_nDistanceAlphaFromDetail;

	int m_nSoftEdges;
	int m_nEdgeSoftnessStart;
	int m_nEdgeSoftnessEnd;
	int m_nScaleEdgeSoftnessBasedOnScreenRes;

	int m_nGlow;
	int m_nGlowColor;
	int m_nGlowAlpha;
	int m_nGlowStart;
	int m_nGlowEnd;
	int m_nGlowX;
	int m_nGlowY;
	int m_nOutline;
	int m_nOutlineColor;
	int m_nOutlineAlpha;
	int m_nOutlineStart0;
	int m_nOutlineStart1;
	int m_nOutlineEnd0;
	int m_nOutlineEnd1;
	int m_nScaleOutlineSoftnessBasedOnScreenRes;

	int m_nSeparateDetailUVs;
	int m_nDetailTextureTransform;

	int m_nLinearWrite;
	int m_nGammaColorRead;

	int m_nDetailTint;
	int m_nInvertPhongMask;

	int m_nDepthBlend;
	int m_nDepthBlendScale;

	int m_nSelfIllumMask;
	int m_nReceiveFlashlight;

// new --------------------------------

	int m_nBlendTintByBaseAlpha;

	int m_nTintReplacesBaseColor;

	int m_nTwoSidedLighting;
};

void InitParamsVertexLitGeneric_DX11( CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, bool bVertexLitGeneric, VertexLitGeneric_DX11_Vars_t &info );
void InitVertexLitGeneric_DX11( CBaseVSShader *pShader, IMaterialVar **params, bool bVertexLitGeneric, VertexLitGeneric_DX11_Vars_t &info );
void DrawVertexLitGeneric_DX11( CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow,
			       bool bVertexLitGeneric, VertexLitGeneric_DX11_Vars_t &info, VertexCompressionType_t vertexCompression,
			       CBasePerMaterialContextData **pContextDataPtr, bool bForceFlashlight = false
);


#endif // VERTEXLITGENERIC_DX11_HELPER_H
