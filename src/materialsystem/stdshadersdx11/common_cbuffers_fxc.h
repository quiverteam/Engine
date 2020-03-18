#ifndef COMMON_CBUFFERS_H_
#define COMMON_CBUFFERS_H_

#include "shader_register_map.h"
#include "lightinfo_fxc.h"

#define CBUFFER_PERMODEL( reg ) \
cbuffer PerModel_t : reg \
{ \
	float4x4 cModelMatrix; \
	float4 cFlexScale; \
	LightInfo cLightInfo[MAX_NUM_LIGHTS]; \
	float3 cAmbientCube[6]; \
};

#define CBUFFER_SKINNING( reg )\
cbuffer Skinning_t : reg \
{ \
	float4x3 cModel[53]; \
};

#define CBUFFER_PERFRAME( reg ) \
cbuffer PerFrame_t : reg \
{ \
	float4x4 cViewMatrix; \
	float4 cEyePos; \
	float4 cFlashlightPos; \
};

#define CBUFFER_PERSCENE( reg ) \
cbuffer PerScene_t : reg \
{ \
	float4x4 cProjMatrix; \
	float4x4 cFlashlightWorldToTexture; \
	float4 cFlashlightScreenScale; \
	float4 cFlashlightColor; \
	float4 cFlashlightAttenuationFactors; \
	float4 cShadowTweaks; \
	float4 cLightScale; \
	float4 cConstants; \
	float4 cLinearFogColor; \
	float4 cFogParams; \
	float4 cFogColor; \
	float4 cFogZ; \
};

#endif // COMMON_CBUFFERS_H_