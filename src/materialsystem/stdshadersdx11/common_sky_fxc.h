//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef COMMON_SKY_FXC_H_
#define COMMON_SKY_FXC_H_

struct Sky_t
{
	// Vertex shader
	float4 vTextureSizeInfo;
	float4 mBaseTexCoordTransform[2];

	// Pixel shader
	float4 InputScale;
};

#endif //#ifndef COMMON_SKY_FXC_H_
