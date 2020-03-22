#ifndef LIGHTMAPPEDGENERIC_DX11_SHARED_FXC_H_
#define LIGHTMAPPEDGENERIC_DX11_SHARED_FXC_H_

struct LightmappedGeneric_t
{
	// Vertex shader
	float4 cBaseTexCoordTransform[2];
	float4 cDetailOrBumpTexCoordTransform[2];
	float4 cEnvmapMaskTexCoordTransform[2];
	float4 cBlendMaskTexCoordTransform[2];
	float4 cSeamlessScale;
	float4 cModulationColor;

	// Pixel shader
	float4 g_OutlineParamsAndColor[2];
	float4 g_EnvmapTint;
	float4 cFresnelReflection;
	float4 cSelfIllumTint;
	float4 g_DetailTint_and_BlendFactor;
	float4 g_TintValuesAndLightmapScale;
	float4 g_EdgeSoftnessParms;
	float4 cEnvmapContrast;
	float4 cEnvmapSaturation;

	int4 g_DetailBlendMode;

	float4 g_FogParams;
	float4 g_FogColor;

	float g_AlphaTestRef;
};

#endif // LIGHTMAPPEDGENERIC_DX11_SHARED_FXC_H_