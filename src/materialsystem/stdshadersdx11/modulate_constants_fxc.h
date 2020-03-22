#ifndef MODULATE_CONSTANTS_FXC_H_
#define MODULATE_CONSTANTS_FXC_H_

struct Modulate_t
{
	// vsh
	float4 cBaseTextureTransform[2];
	float4 cModulationColor;

	// psh
	float4 cWhiteGrayMix;
	float4 g_FogParams;
	float4 g_FogColor;
};

#endif // MODULATE_CONSTANTS_FXC_H_