#ifndef UNLITTWOTEXTURE_CONSTANTS_FXC_H_
#define UNLITTWOTEXTURE_CONSTANTS_FXC_H_

struct UnlitTwoTexture_t
{
	// vsh
	float4 cBaseTextureTransform[2];
	float4 cBaseTexture2Transform[2];

	// psh
	float4 cModulationColor;
	float4 g_FogParams;
	float4 g_FogColor;
};

#endif // UNLITTWOTEXTURE_CONSTANTS_FXC_H_