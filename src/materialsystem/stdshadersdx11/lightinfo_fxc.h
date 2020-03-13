#ifndef LIGHTINFO_FXC_H
#define LIGHTINFO_FXC_H

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

#endif // LIGHTINFO_FXC_H