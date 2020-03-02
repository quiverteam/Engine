//========= Copyright Valve Corporation, All rights reserved. ============//

void GetBaseTextureAndNormal( Texture2D basetex, SamplerState base, Texture2D basetex2, SamplerState base2, Texture2D bumptex, SamplerState bump,
			      bool bBase2, bool bBump, float3 coords, float3 vWeights,
			      out float4 vResultBase, out float4 vResultBase2, out float4 vResultBump )
{
	vResultBase = 0;
	vResultBase2 = 0;
	vResultBump = 0;

	if ( !bBump )
	{
		vResultBump = float4(0, 0, 1, 1);
	}

#if SEAMLESS

	vResultBase  += vWeights.x * basetex.Sample( base, coords.zy );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.x * basetex2.Sample( base2, coords.zy );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.x * bumptex.Sample( bump, coords.zy );
	}

	vResultBase  += vWeights.y * basetex.Sample( base, coords.xz );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.y * basetex2.Sample( base2, coords.xz );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.y * bumptex.Sample( bump, coords.xz );
	}

	vResultBase  += vWeights.z * basetex.Sample( base, coords.xy );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.z * basetex2.Sample( base2, coords.xy );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.z * bumptex.Sample( bump, coords.xy );
	}

#else  // not seamless

	vResultBase  = basetex.Sample( base, coords.xy );
	if ( bBase2 )
	{
		vResultBase2 = basetex2.Sample( base2, coords.xy );
	}
	if ( bBump )
	{
		vResultBump  = bumptex.Sample( bump, coords.xy );
	}
#endif

}




float3 LightMapSample( Texture2D LightmapTex, SamplerState LightmapSampler, float2 vTexCoord )
{
#	if ( !defined( _X360 ) || !defined( USE_32BIT_LIGHTMAPS_ON_360 ) )
	{
		float3 sample = LightmapTex.Sample( LightmapSampler, vTexCoord ).xyz;

		return sample;
	}
#	else
	{
#		if 0 //1 for cheap sampling, 0 for accurate scaling from the individual samples
		{
			float4 sample = LightmapTex.Sample( LightmapSampler, vTexCoord );

			return sample.rgb * sample.a;
		}
#		else
		{
			float4 Weights;
			float4 samples_0; //no arrays allowed in inline assembly
			float4 samples_1;
			float4 samples_2;
			float4 samples_3;
			
			asm {
				tfetch2D samples_0, vTexCoord.xy, LightmapSampler, OffsetX = -0.5, OffsetY = -0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
				tfetch2D samples_1, vTexCoord.xy, LightmapSampler, OffsetX =  0.5, OffsetY = -0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
				tfetch2D samples_2, vTexCoord.xy, LightmapSampler, OffsetX = -0.5, OffsetY =  0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
				tfetch2D samples_3, vTexCoord.xy, LightmapSampler, OffsetX =  0.5, OffsetY =  0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false

				getWeights2D Weights, vTexCoord.xy, LightmapSampler
			};

			Weights = float4( (1-Weights.x)*(1-Weights.y), Weights.x*(1-Weights.y), (1-Weights.x)*Weights.y, Weights.x*Weights.y );

			float3 result;
			result.rgb  = samples_0.rgb * (samples_0.a * Weights.x);
			result.rgb += samples_1.rgb * (samples_1.a * Weights.y);
			result.rgb += samples_2.rgb * (samples_2.a * Weights.z);
			result.rgb += samples_3.rgb * (samples_3.a * Weights.w);
		
			return result;
		}
#		endif
	}
#	endif
}