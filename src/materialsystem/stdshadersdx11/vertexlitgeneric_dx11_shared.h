#ifndef VERTEXLITGENERIC_DX11_SHARED_H_
#define VERTEXLITGENERIC_DX11_SHARED_H_

struct VertexLitAndUnlitGeneric_t
{
	// Vertex shader
	float4 cBaseTexCoordTransform[2];
	float4 cDetailTexCoordTransform[2];
	float4 cSeamlessScale;
	float4 cMorphSubrect;
	float4 cMorphTargetTextureDim;
	float4 cModulationColor;

	// Pixel shader
	float4 g_DetailTint;
	float4 g_GlowParameters;
	float4 g_GlowColor;
	float4 g_DistanceAlphaParams;
	float4 g_OutlineColor;
	float4 g_OutlineParams;
	float4 g_EnvmapSaturation_SelfIllumMask;
	float4 g_EnvmapTint_TintReplaceFactor;
	float4 g_SelfIllumScaleBiasExpBrightness;
	float4 g_EnvmapContrast_ShadowTweaks;
	float4 g_SelfIllumTint_and_BlendFactor;
	float4 g_DepthFeatheringConstants;
	float4 g_ShaderControls;

	// Pixel shader (phong)
	float4 g_FresnelSpecParams;
	float4 g_SpecularRimParams;
	// RimBoost_RimMaskControl_BaseMapAlphaPhongMask_InvertPhongMask
	float4 g_RimPhongParams;
	float4 g_SpecExponent;
};

#endif //#ifndef VERTEXLITGENERIC_DX11_SHARED_H_
