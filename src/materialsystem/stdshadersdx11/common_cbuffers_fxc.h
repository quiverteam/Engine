#ifndef COMMON_CBUFFERS_H_
#define COMMON_CBUFFERS_H_

#include "shader_register_map.h"

#define CBUFFER_PERMATERIAL( reg ) \
cbuffer PerMaterial_t : reg \
{ \
	float4 cShadowTweaks; \
	float4 cLightScale; \
	float4 cConstants1; \
	float4 cModulationColor; \
	float4 cAlphaTestRef; \
};

// w components of color and dir indicate light type:
// 1x - directional
// 01 - spot
// 00 - point
struct LightInfo
{
	float4 color;						// {xyz} is color	w is light type code (see comment below)
	float4 dir;							// {xyz} is dir		w is light type code
	float4 pos;
	float4 spotParams;
	float4 atten;
};

#define CBUFFER_PERMODEL( reg ) \
cbuffer PerModel_t : reg \
{ \
	matrix cModelViewProj; \
	matrix cViewModel; \
	float4x4 cModel[53]; \
	float4 cFlexWeights[512]; \
	float4 cFlexScale; \
	float4 cLightEnabled; \
	float4 cLightCountRegister; \
	float4 cAmbientCube[6]; \
	LightInfo cLightInfo[4]; \
};

#define CBUFFER_PERFRAME( reg ) \
cbuffer PerFrame_t : reg \
{ \
	matrix cViewProj; \
	float4 cEyePos; \
	float4 cFlashlightPos; \
};

#define CBUFFER_PERSCENE( reg ) \
cbuffer PerScene_t : reg \
{ \
	matrix cFlashlightWorldToTexture; \
	float4 cFlashlightScreenScale; \
	float4 cFlashlightColor; \
	float4 cFlashlightAttenuationFactors; \
	float4 cLinearFogColor; \
	float4 cFogParams; \
	float4 cFogColor; \
	float cFogZ; \
};

#endif // COMMON_CBUFFERS_H_