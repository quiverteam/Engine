#ifndef SPRITECARD_CONSTANTS_FXC_H_
#define SPRITECARD_CONSTANTS_FXC_H_

struct SpriteCard_t
{
	// Vertex shader
	float4 ScaleParms;
	float4 SizeParms;
	float4 SizeParms2;
	int4 SpriteControls;

	// Pixel shader
	float4 g_Parameters;
	float4 g_DepthFeatheringConstants;
};

#endif // SPRITECARD_CONSTANTS_FXC_H_