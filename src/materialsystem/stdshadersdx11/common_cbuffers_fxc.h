#ifndef COMMON_CBUFFERS_H_
#define COMMON_CBUFFERS_H_

#include "shader_register_map.h"
#include "lightinfo_fxc.h"

#define CBUFFER_PERMODEL( reg ) \
cbuffer PerModel_t : reg \
{ \
	matrix cModelMatrix; \
	float4 cFlexScale; \
	int4 cLightEnabled; \
	int4 cLightCountRegister; \
	LightInfo cLightInfo[4]; \
	float3 cAmbientCube[6]; \
};

#define CBUFFER_SKINNING( reg )\
cbuffer Skinning_t : reg \
{ \
	float4x4 cModel[53]; \
};

#define CBUFFER_PERFRAME( reg ) \
cbuffer PerFrame_t : reg \
{ \
	matrix cViewMatrix; \
	float4 cEyePos; \
	float4 cFlashlightPos; \
};

#define CBUFFER_PERSCENE( reg ) \
cbuffer PerScene_t : reg \
{ \
	matrix cProjMatrix; \
	matrix cFlashlightWorldToTexture; \
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