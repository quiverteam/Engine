//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: This is where all common code for vertex shaders go.
//
// NOTE: This file expects that you have included common_cbuffers_fxc.h and
// defined the transform, skinning, lighting, and misc buffers before
// including this file!
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef COMMON_VS_FXC_H_
#define COMMON_VS_FXC_H_

#include "common_fxc.h"

#define NUM_MODEL_TRANSFORMS	53

// Put global skip commands here. . make sure and check that the appropriate vars are defined
// so these aren't used on the wrong shaders!
// --------------------------------------------------------------------------------
// Ditch all fastpath attemps if we are doing LIGHTING_PREVIEW.
//	SKIP: defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// --------------------------------------------------------------------------------


#ifndef COMPRESSED_VERTS
// Default to no vertex compression
#define COMPRESSED_VERTS 0
#endif

#if ( !defined( SHADER_MODEL_VS_2_0 ) && !defined( SHADER_MODEL_VS_3_0 ) && !defined(SHADER_MODEL_VS_4_0) )
#if COMPRESSED_VERTS == 1
#error "Vertex compression is only for DX9 and up!"
#endif
#endif

// We're testing 2 normal compression methods
// One compressed normals+tangents into a SHORT2 each (8 bytes total)
// The other compresses them together, into a single UBYTE4 (4 bytes total)
// FIXME: pick one or the other, compare lighting quality in important cases
#define COMPRESSED_NORMALS_SEPARATETANGENTS_SHORT2	0
#define COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4	1
//#define COMPRESSED_NORMALS_TYPE						COMPRESSED_NORMALS_SEPARATETANGENTS_SHORT2
#define COMPRESSED_NORMALS_TYPE					COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4


#define FOGTYPE_RANGE				0
#define FOGTYPE_HEIGHT				1

#define COMPILE_ERROR ( 1/0; )

// ----------------------------------------------------------------------
// CONSTANT BUFFERS
// NOTE: These need to match the structs defined in ShaderDeviceDx11.h!!!
// ----------------------------------------------------------------------

#define cOOGamma			cConstants1.x
#define cOverbright			2.0f
#define cOneThird			cConstants1.z
#define cOOOverbright		( 1.0f / 2.0f )

#define g_nLightCount					cLightCountRegister.x

#define cFogEndOverFogRange cFogParams.x
#define cFogOne cFogParams.y
#define cFogMaxDensity cFogParams.z
#define cOOFogRange cFogParams.w

//=======================================================================================
// Methods to decompress vertex normals
//=======================================================================================

//-----------------------------------------------------------------------------------
// Decompress a normal from two-component compressed format
// We expect this data to come from a signed SHORT2 stream in the range of -32768..32767
//
// -32678 and 0 are invalid encodings
// w contains the sign to use in the cross product when generating a binormal
void _DecompressShort2Tangent( float2 inputTangent, out float4 outputTangent )
{
	float2 ztSigns		= sign( inputTangent );				// sign bits for z and tangent (+1 or -1)
	float2 xyAbs		= abs(  inputTangent );				// 1..32767
	outputTangent.xy	= (xyAbs - 16384.0f) / 16384.0f;	// x and y
	outputTangent.z		= ztSigns.x * sqrt( saturate( 1.0f - dot( outputTangent.xy, outputTangent.xy ) ) );
	outputTangent.w		= ztSigns.y;
}

//-----------------------------------------------------------------------------------
// Same code as _DecompressShort2Tangent, just one returns a float4, one a float3
void _DecompressShort2Normal( float2 inputNormal, out float3 outputNormal )
{
	float4 result;
	_DecompressShort2Tangent( inputNormal, result );
	outputNormal = result.xyz;
}

//-----------------------------------------------------------------------------------
// Decompress normal+tangent together
void _DecompressShort2NormalTangent( float2 inputNormal, float2 inputTangent, out float3 outputNormal, out float4 outputTangent )
{
	// FIXME: if we end up sticking with the SHORT2 format, pack the normal and tangent into a single SHORT4 element
	//        (that would make unpacking normal+tangent here together much cheaper than the sum of their parts)
	_DecompressShort2Normal(  inputNormal,  outputNormal  );
	_DecompressShort2Tangent( inputTangent, outputTangent );
}

//=======================================================================================
// Decompress a normal and tangent from four-component compressed format
// We expect this data to come from an unsigned UBYTE4 stream in the range of 0..255
// The final vTangent.w contains the sign to use in the cross product when generating a binormal
void _DecompressUByte4NormalTangent( float4 inputNormal,
									out float3 outputNormal,   // {nX, nY, nZ}
									out float4 outputTangent )   // {tX, tY, tZ, sign of binormal}
{
	float fOne   = 1.0f;

	float4 ztztSignBits	= ( inputNormal - 128.0f ) < 0;						// sign bits for zs and binormal (1 or 0)  set-less-than (slt) asm instruction
	float4 xyxyAbs		= abs( inputNormal - 128.0f ) - ztztSignBits;		// 0..127
	float4 xyxySignBits	= ( xyxyAbs - 64.0f ) < 0;							// sign bits for xs and ys (1 or 0)
	float4 normTan		= (abs( xyxyAbs - 64.0f ) - xyxySignBits) / 63.0f;	// abs({nX, nY, tX, tY})
	outputNormal.xy		= normTan.xy;										// abs({nX, nY, __, __})
	outputTangent.xy	= normTan.zw;										// abs({tX, tY, __, __})

	float4 xyxySigns	= 1 - 2*xyxySignBits;								// Convert sign bits to signs
	float4 ztztSigns	= 1 - 2*ztztSignBits;								// ( [1,0] -> [-1,+1] )

	outputNormal.z		= 1.0f - outputNormal.x - outputNormal.y;			// Project onto x+y+z=1
	outputNormal.xyz	= normalize( outputNormal.xyz );					// Normalize onto unit sphere
	outputNormal.xy	   *= xyxySigns.xy;										// Restore x and y signs
	outputNormal.z	   *= ztztSigns.x;										// Restore z sign

	outputTangent.z		= 1.0f - outputTangent.x - outputTangent.y;			// Project onto x+y+z=1
	outputTangent.xyz	= normalize( outputTangent.xyz );					// Normalize onto unit sphere
	outputTangent.xy   *= xyxySigns.zw;										// Restore x and y signs
	outputTangent.z	   *= ztztSigns.z;										// Restore z sign
	outputTangent.w		= ztztSigns.w;										// Binormal sign
}


//-----------------------------------------------------------------------------------
// Decompress just a normal from four-component compressed format (same as above)
// We expect this data to come from an unsigned UBYTE4 stream in the range of 0..255
// [ When compiled, this works out to approximately 17 asm instructions ]
void _DecompressUByte4Normal( float4 inputNormal,
							out float3 outputNormal)					// {nX, nY, nZ}
{
	float fOne			= 1.0f;

	float2 ztSigns		= ( inputNormal.xy - 128.0f ) < 0;				// sign bits for zs and binormal (1 or 0)  set-less-than (slt) asm instruction
	float2 xyAbs		= abs( inputNormal.xy - 128.0f ) - ztSigns;		// 0..127
	float2 xySigns		= ( xyAbs -  64.0f ) < 0;						// sign bits for xs and ys (1 or 0)
	outputNormal.xy		= ( abs( xyAbs - 64.0f ) - xySigns ) / 63.0f;	// abs({nX, nY})

	outputNormal.z		= 1.0f - outputNormal.x - outputNormal.y;		// Project onto x+y+z=1
	outputNormal.xyz	= normalize( outputNormal.xyz );				// Normalize onto unit sphere

	outputNormal.xy	   *= lerp( fOne.xx, -fOne.xx, xySigns   );			// Restore x and y signs
	outputNormal.z	   *= lerp( fOne.x,  -fOne.x,  ztSigns.x );			// Restore z sign
}


void DecompressVertex_Normal( float4 inputNormal, out float3 outputNormal )
{
	if ( COMPRESSED_VERTS == 1 )
	{
		if ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_SEPARATETANGENTS_SHORT2 )
		{
			_DecompressShort2Normal( inputNormal.xy, outputNormal );
		}
		else // ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 )
		{
			_DecompressUByte4Normal( inputNormal, outputNormal );
		}
	}
	else
	{
		outputNormal = inputNormal.xyz;
	}
}

void DecompressVertex_NormalTangent( float4 inputNormal,  float4 inputTangent, out float3 outputNormal, out float4 outputTangent )
{
	if ( COMPRESSED_VERTS == 1 )
	{
		if ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_SEPARATETANGENTS_SHORT2 )
		{
			_DecompressShort2NormalTangent( inputNormal.xy, inputTangent.xy, outputNormal, outputTangent );
		}
		else // ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 )
		{
			_DecompressUByte4NormalTangent( inputNormal, outputNormal, outputTangent );
		}
	}
	else
	{
		outputNormal  = inputNormal.xyz;
		outputTangent = inputTangent;
	}
}


#if defined(SHADER_MODEL_VS_3_0) || defined(SHADER_MODEL_VS_4_0)

//-----------------------------------------------------------------------------
// Methods to sample morph data from a vertex texture
// NOTE: vMorphTargetTextureDim.x = width, cVertexTextureDim.y = height, cVertexTextureDim.z = # of float4 fields per vertex
// For position + normal morph for example, there will be 2 fields.
//-----------------------------------------------------------------------------
float4 SampleMorphDelta( Texture2D vtt, SamplerState vt, const float3 vMorphTargetTextureDim, const float4 vMorphSubrect, const float flVertexID, const float flField )
{
	float flColumn = floor( flVertexID / vMorphSubrect.w );

	float4 t;
	t.x = vMorphSubrect.x + vMorphTargetTextureDim.z * flColumn + flField + 0.5f;
	t.y = vMorphSubrect.y + flVertexID - flColumn * vMorphSubrect.w + 0.5f;
	t.xy /= vMorphTargetTextureDim.xy;	
	t.z = t.w = 0.f;

	return vtt.SampleLevel( vt, t.xy, t.z );
}

// Optimized version which reads 2 deltas
void SampleMorphDelta2( Texture2D vtt, SamplerState vt, const float3 vMorphTargetTextureDim, const float4 vMorphSubrect, const float flVertexID, out float4 delta1, out float4 delta2 )
{
	float flColumn = floor( flVertexID / vMorphSubrect.w );

	float4 t;
	t.x = vMorphSubrect.x + vMorphTargetTextureDim.z * flColumn + 0.5f;
	t.y = vMorphSubrect.y + flVertexID - flColumn * vMorphSubrect.w + 0.5f;
	t.xy /= vMorphTargetTextureDim.xy;	
	t.z = t.w = 0.f;

	delta1 = vtt.SampleLevel( vt, t.xy, t.z );
	t.x += 1.0f / vMorphTargetTextureDim.x;
	delta2 = vtt.SampleLevel( vt, t.xy, t.z );
}

#endif // SHADER_MODEL_VS_3_0

//-----------------------------------------------------------------------------
// Method to apply morphs
//-----------------------------------------------------------------------------
bool ApplyMorph( float3 vPosFlex, float4 flexScale, inout float3 vPosition )
{
	// Flexes coming in from a separate stream
	float3 vPosDelta = vPosFlex.xyz * flexScale.x;
	vPosition.xyz += vPosDelta;
	return true;
}

bool ApplyMorph( float3 vPosFlex, float3 vNormalFlex, float4 flexScale, inout float3 vPosition, inout float3 vNormal )
{
	// Flexes coming in from a separate stream
	float3 vPosDelta = vPosFlex.xyz * flexScale.x;
	float3 vNormalDelta = vNormalFlex.xyz * flexScale.x;
	vPosition.xyz += vPosDelta;
	vNormal       += vNormalDelta;
	return true;
}

bool ApplyMorph( float3 vPosFlex, float3 vNormalFlex, float4 flexScale,
	inout float3 vPosition, inout float3 vNormal, inout float3 vTangent )
{
	// Flexes coming in from a separate stream
	float3 vPosDelta = vPosFlex.xyz * flexScale.x;
	float3 vNormalDelta = vNormalFlex.xyz * flexScale.x;
	vPosition.xyz += vPosDelta;
	vNormal       += vNormalDelta;
	vTangent.xyz  += vNormalDelta;
	return true;
}

bool ApplyMorph( float4 vPosFlex, float3 vNormalFlex, float4 flexScale,
	inout float3 vPosition, inout float3 vNormal, inout float3 vTangent, out float flWrinkle )
{
	// Flexes coming in from a separate stream
	float3 vPosDelta = vPosFlex.xyz * flexScale.x;
	float3 vNormalDelta = vNormalFlex.xyz * flexScale.x;
	flWrinkle = vPosFlex.w * flexScale.y;
	vPosition.xyz += vPosDelta;
	vNormal       += vNormalDelta;
	vTangent.xyz  += vNormalDelta;
	return true;
}

#if defined(SHADER_MODEL_VS_3_0) || defined(SHADER_MODEL_VS_4_0)
#pragma message "We have sm 3.0 or 4.0"

bool ApplyMorph( Texture2D morphTexture, SamplerState morphSampler, const float3 vMorphTargetTextureDim, const float4 vMorphSubrect, 
				const float flVertexID, const float3 vMorphTexCoord,
				inout float3 vPosition )
{
#if MORPHING

#if !DECAL
	// Flexes coming in from a separate stream
	float4 vPosDelta = SampleMorphDelta( morphTexture, morphSampler, vMorphTargetTextureDim, vMorphSubrect, flVertexID, 0 );
	vPosition	+= vPosDelta.xyz;
#else
	float4 t = float4( vMorphTexCoord.x, vMorphTexCoord.y, 0.0f, 0.0f );
	float3 vPosDelta = morphTexture.SampleLevel( morphSampler, t.xy, t.z );
	vPosition	+= vPosDelta.xyz * vMorphTexCoord.z;
#endif // DECAL

	return true;

#else // !MORPHING
	return false;
#endif
}
 
bool ApplyMorph( Texture2D morphTexture, SamplerState morphSampler, const float3 vMorphTargetTextureDim, const float4 vMorphSubrect, 
				const float flVertexID, const float3 vMorphTexCoord, 
				inout float3 vPosition, inout float3 vNormal )
{
#if MORPHING

#if !DECAL
	float4 vPosDelta, vNormalDelta;
	SampleMorphDelta2( morphTexture, morphSampler, vMorphTargetTextureDim, vMorphSubrect, flVertexID, vPosDelta, vNormalDelta );
	vPosition	+= vPosDelta.xyz;
	vNormal		+= vNormalDelta.xyz;
#else
	float4 t = float4( vMorphTexCoord.x, vMorphTexCoord.y, 0.0f, 0.0f );
	float3 vPosDelta = morphTexture.SampleLevel( morphSampler, t.xy, t.z );
	t.x += 1.0f / vMorphTargetTextureDim.x;
	float3 vNormalDelta = morphTexture.SampleLevel( morphSampler, t.xy, t.z );
	vPosition	+= vPosDelta.xyz * vMorphTexCoord.z;
	vNormal		+= vNormalDelta.xyz * vMorphTexCoord.z;
#endif // DECAL

	return true;

#else // !MORPHING
	return false;
#endif
}

bool ApplyMorph( Texture2D morphTexture, SamplerState morphSampler, const float3 vMorphTargetTextureDim, const float4 vMorphSubrect, 
				const float flVertexID, const float3 vMorphTexCoord, 
				inout float3 vPosition, inout float3 vNormal, inout float3 vTangent )
{
#if MORPHING

#if !DECAL
	float4 vPosDelta, vNormalDelta;
	SampleMorphDelta2( morphTexture, morphSampler, vMorphTargetTextureDim, vMorphSubrect, flVertexID, vPosDelta, vNormalDelta );
	vPosition	+= vPosDelta.xyz;
	vNormal		+= vNormalDelta.xyz;
	vTangent	+= vNormalDelta.xyz;
#else
	float4 t = float4( vMorphTexCoord.x, vMorphTexCoord.y, 0.0f, 0.0f );
	float3 vPosDelta = morphTexture.SampleLevel( morphSampler, t.xy, t.z );
	t.x += 1.0f / vMorphTargetTextureDim.x;
	float3 vNormalDelta = morphTexture.SampleLevel( morphSampler, t.xy, t.z );
	vPosition	+= vPosDelta.xyz * vMorphTexCoord.z;
	vNormal		+= vNormalDelta.xyz * vMorphTexCoord.z;
	vTangent	+= vNormalDelta.xyz * vMorphTexCoord.z;
#endif // DECAL

	return true;

#else // MORPHING

	return false;
#endif
}

bool ApplyMorph( Texture2D morphTexture, SamplerState morphSampler, const float3 vMorphTargetTextureDim, const float4 vMorphSubrect,
	const float flVertexID, const float3 vMorphTexCoord,
	inout float3 vPosition, inout float3 vNormal, inout float3 vTangent, out float flWrinkle )
{
#if MORPHING

#if !DECAL
	float4 vPosDelta, vNormalDelta;
	SampleMorphDelta2( morphTexture, morphSampler, vMorphTargetTextureDim, vMorphSubrect, flVertexID, vPosDelta, vNormalDelta );
	vPosition	+= vPosDelta.xyz;
	vNormal		+= vNormalDelta.xyz;
	vTangent	+= vNormalDelta.xyz;
	flWrinkle = vPosDelta.w;
#else
	float4 t = float4( vMorphTexCoord.x, vMorphTexCoord.y, 0.0f, 0.0f );
	float4 vPosDelta = morphTexture.SampleLevel( morphSampler, t.xy, t.z );
	t.x += 1.0f / vMorphTargetTextureDim.x;
	float3 vNormalDelta = morphTexture.SampleLevel( morphSampler, t.xy, t.z );

	vPosition	+= vPosDelta.xyz * vMorphTexCoord.z;
	vNormal		+= vNormalDelta.xyz * vMorphTexCoord.z;
	vTangent	+= vNormalDelta.xyz * vMorphTexCoord.z;
	flWrinkle	= vPosDelta.w * vMorphTexCoord.z;
#endif // DECAL

	return true;

#else // MORPHING

	flWrinkle = 0.0f;
	return false;

#endif
}

#endif   // SHADER_MODEL_VS_3_0


float RangeFog( const float3 projPos, const float4 fogParams )
{
	//        cFogMaxDensity,             cOOFogRange, cFogEndOverFogRange
	return max( fogParams.z, ( -projPos.z * fogParams.w + fogParams.x ) );
}

float WaterFog( const float3 worldPos, const float3 projPos, const float3 eyePos, const float fogZ, const float4 fogParams )
{
	float4 tmp;
	
	tmp.xy = float2( fogZ, eyePos.z) - worldPos.z;

	// tmp.x is the distance from the water surface to the vert
	// tmp.y is the distance from the eye position to the vert

	// if $tmp.x < 0, then set it to 0
	// This is the equivalent of moving the vert to the water surface if it's above the water surface
	
	tmp.x = max( 0.0f, tmp.x );

	// $tmp.w = $tmp.x / $tmp.y
	tmp.w = tmp.x / tmp.y;

	tmp.w *= projPos.z;

	// $tmp.w is now the distance that we see through water.

	return max( fogParams.z, ( -tmp.w * fogParams.w + fogParams.y ) );
}

float CalcFog( const float3 worldPos, const float3 projPos, const float3 eyePos, const int fogType, const float fogZ, const float4 fogParams )
{
#if defined( _X360 )
	// 360 only does pixel fog
	return 1.0f;
#endif

	if( fogType == FOGTYPE_RANGE )
	{
		return RangeFog( projPos, fogParams );
	}
	else
	{
#if SHADERMODEL_VS_2_0 == 1
		// We do this work in the pixel shader in dx9, so don't do any fog here.
		return 1.0f;
#else
		return WaterFog( worldPos, projPos, eyePos, fogZ, fogParams  );
#endif
	}
}

float CalcFog( const float3 worldPos, const float3 projPos, const float3 eyePos, const bool bWaterFog, const float fogZ, const float4 fogParams )
{
#if defined( _X360 )
	// 360 only does pixel fog
	return 1.0f;
#endif

	float flFog;
	if( !bWaterFog )
	{
		flFog = RangeFog( projPos, fogParams );
	}
	else
	{
#if SHADERMODEL_VS_2_0 == 1
		// We do this work in the pixel shader in dx9, so don't do any fog here.
		flFog = 1.0f;
#else
		flFog = WaterFog( worldPos, projPos, eyePos, fogZ, fogParams );
#endif
	}

	return flFog;
}

float4 DecompressBoneWeights( const float4 weights )
{
	float4 result = weights;

	if ( COMPRESSED_VERTS )
	{
		// Decompress from SHORT2 to float. In our case, [-1, +32767] -> [0, +1]
		// NOTE: we add 1 here so we can divide by 32768 - which is exact (divide by 32767 is not).
		//       This avoids cracking between meshes with different numbers of bone weights.
		//       We use SHORT2 instead of SHORT2N for a similar reason - the GPU's conversion
		//       from [-32768,+32767] to [-1,+1] is imprecise in the same way.
		result += 1;
		result /= 32768;
	}

	return result;
}


void SkinPosition( bool bSkinning, const float4 modelPos, 
                   const float4 boneWeights, uint4 boneIndices,
		   float4x3 model[NUM_MODEL_TRANSFORMS],
				   out float3 worldPos )
{

	// Needed for invariance issues caused by multipass rendering
#if defined( _X360 )
	[isolate] 
#endif
	{ 
		if ( !bSkinning )
		{
			worldPos = mul4x3( modelPos, (float4x3)model[0] );
		}
		else // skinning - always three bones
		{
			float4x3 mat1 = (float4x3)model[boneIndices[0]];
			float4x3 mat2 = (float4x3)model[boneIndices[1]];
			float4x3 mat3 = (float4x3)model[boneIndices[2]];

			float3 weights = DecompressBoneWeights( boneWeights ).xyz;
			weights[2] = 1 - (weights[0] + weights[1]);

			float4x3 blendMatrix = mat1 * weights[0] + mat2 * weights[1] + mat3 * weights[2];
			worldPos = mul4x3( modelPos, blendMatrix );
		}
	}
}

void SkinPositionAndNormal( bool bSkinning, const float4 modelPos, const float3 modelNormal,
                            const float4 boneWeights, uint4 boneIndices,
			    float4x3 model[NUM_MODEL_TRANSFORMS],
						    out float3 worldPos, out float3 worldNormal )
{
	// Needed for invariance issues caused by multipass rendering
#if defined( _X360 )
	[isolate] 
#endif
	{ 

		if ( !bSkinning )
		{
			worldPos = mul4x3( modelPos, (float4x3)model[0] );
			worldNormal = mul3x3( modelNormal, ( const float3x3 )model[0] );
		}
		else // skinning - always three bones
		{
			float4x3 mat1 = (float4x3)model[boneIndices[0]];
			float4x3 mat2 = (float4x3)model[boneIndices[1]];
			float4x3 mat3 = (float4x3)model[boneIndices[2]];

			float3 weights = DecompressBoneWeights( boneWeights ).xyz;
			weights[2] = 1 - (weights[0] + weights[1]);

			float4x3 blendMatrix = mat1 * weights[0] + mat2 * weights[1] + mat3 * weights[2];
			worldPos = mul4x3( modelPos, blendMatrix );
			worldNormal = mul3x3( modelNormal, ( float3x3 )blendMatrix );
		}

	} // end [isolate]
}

// Is it worth keeping SkinPosition and SkinPositionAndNormal around since the optimizer
// gets rid of anything that isn't used?
void SkinPositionNormalAndTangentSpace( 
							bool bSkinning,
						    const float4 modelPos, const float3 modelNormal, 
							const float4 modelTangentS,
                            const float4 boneWeights, uint4 boneIndices,
				float4x3 model[NUM_MODEL_TRANSFORMS],
						    out float3 worldPos, out float3 worldNormal, 
							out float3 worldTangentS, out float3 worldTangentT )
{

	// Needed for invariance issues caused by multipass rendering
#if defined( _X360 )
	[isolate] 
#endif
	{ 
		if ( !bSkinning )
		{
			worldPos = mul4x3( modelPos, (float4x3)model[0] );
			worldNormal = mul3x3( modelNormal, ( const float3x3 )model[0] );
			worldTangentS = mul3x3( ( float3 )modelTangentS, ( const float3x3 )model[0] );
		}
		else // skinning - always three bones
		{
			float4x3 mat1 = (float4x3)model[boneIndices[0]];
			float4x3 mat2 = (float4x3)model[boneIndices[1]];
			float4x3 mat3 = (float4x3)model[boneIndices[2]];

			float3 weights = DecompressBoneWeights( boneWeights ).xyz;
			weights[2] = 1 - (weights[0] + weights[1]);

			float4x3 blendMatrix = mat1 * weights[0] + mat2 * weights[1] + mat3 * weights[2];
			worldPos = mul4x3( modelPos, blendMatrix );
			worldNormal = mul3x3( modelNormal, ( const float3x3 )blendMatrix );
			worldTangentS = mul3x3( ( float3 )modelTangentS, ( const float3x3 )blendMatrix );
		}
		worldTangentT = cross( worldNormal, worldTangentS ) * modelTangentS.w;
	}
}


//-----------------------------------------------------------------------------
// Lighting helper functions
//-----------------------------------------------------------------------------

float3 AmbientLight( const float3 worldNormal, const float3 ambientCube[6] )
{
	float3 nSquared = worldNormal * worldNormal;
	int3 isNegative = ( worldNormal < 0.0 );
	int3 isPositive = 1 - isNegative;
	isNegative *= nSquared;
	isPositive *= nSquared;
	float3 linearColor;
	linearColor = isPositive.x * ambientCube[0] + isNegative.x * ambientCube[1] +
		isPositive.y * ambientCube[2] + isNegative.y * ambientCube[3] +
		isPositive.z * ambientCube[4] + isNegative.z * ambientCube[5];
	return linearColor;
}

float CosineTermInternal( const float3 worldPos, const float3 worldNormal, int lightNum, bool bHalfLambert, LightInfo lightInfo[MAX_NUM_LIGHTS] )
{
	// Calculate light direction assuming this is a point or spot
	float3 lightDir = normalize( lightInfo[lightNum].pos - worldPos );

	// Select the above direction or the one in the structure, based upon light type
	lightDir = lerp( lightDir, -lightInfo[lightNum].dir, lightInfo[lightNum].color.w );

	// compute N dot L
	float NDotL = dot( worldNormal, lightDir );

	if ( !bHalfLambert )
	{
		NDotL = max( 0.0f, NDotL );
	}
	else	// Half-Lambert
	{
		NDotL = NDotL * 0.5 + 0.5;
		NDotL = NDotL * NDotL;
	}
	return NDotL;
}

// This routine uses booleans to do early-outs and is meant to be called by routines OUTSIDE of this file
float CosineTerm( const float3 worldPos, const float3 worldNormal, int lightNum, bool bHalfLambert,
		  int4 lightEnabled[MAX_NUM_LIGHTS], LightInfo lightInfo[MAX_NUM_LIGHTS] )
{
	float flResult = 0.0f;
	//if ( lightEnabled[lightNum] != 0 )
	{
		flResult = CosineTermInternal( worldPos, worldNormal, lightNum, bHalfLambert, lightInfo );
	}

	return flResult;
}


float3 DoLightInternal( const float3 worldPos, const float3 worldNormal, int lightNum, bool bHalfLambert,
			LightInfo lightInfo[MAX_NUM_LIGHTS] )
{
	return lightInfo[lightNum].color *
		CosineTermInternal( worldPos, worldNormal, lightNum, bHalfLambert, lightInfo ) *
		LightAttenInternal( worldPos, lightNum, lightInfo );
}


#if 0
// This routine
float3 DoLighting( const float3 worldPos, const float3 worldNormal,
				   const float3 staticLightingColor, const bool bStaticLight,
				   const bool bDynamicLight, bool bHalfLambert, LightInfo lightInfo[MAX_NUM_LIGHTS],
		   float3 ambientCube[6])
{
	float3 linearColor = float3( 0.0f, 0.0f, 0.0f );

	if( bStaticLight )			// Static light
	{
		float3 col = staticLightingColor * cOverbright;
#if defined ( _X360 )
		linearColor += col * col;
#else
		linearColor += GammaToLinear( col );
#endif
	}

	if( bDynamicLight )			// Dynamic light
	{
		for (int i = 0; i < g_nLightCount; i++)
		{
			linearColor += DoLightInternal( worldPos, worldNormal, i, bHalfLambert, lightInfo );
		}		
	}

	if( bDynamicLight )
	{
		linearColor += AmbientLight( worldNormal, ambientCube ); //ambient light is already remapped
	}

	return linearColor;
}

#endif


float3 DoLightingUnrolled( const float3 worldPos, const float3 worldNormal,
				  const float3 staticLightingColor, const bool bStaticLight,
				  const bool bDynamicLight, bool bHalfLambert, const int nNumLights,
			   LightInfo lightInfo[MAX_NUM_LIGHTS], float3 ambientCube[6])
{
	float3 linearColor = float3( 0.0f, 0.0f, 0.0f );

	if( bStaticLight )			// Static light
	{
		linearColor += GammaToLinear( staticLightingColor * cOverbright );
	}

	if( bDynamicLight )			// Ambient light
	{
		if ( nNumLights >= 1 )
			linearColor += DoLightInternal( worldPos, worldNormal, 0, bHalfLambert, lightInfo );
		if ( nNumLights >= 2 )
			linearColor += DoLightInternal( worldPos, worldNormal, 1, bHalfLambert, lightInfo );
		if ( nNumLights >= 3 )
			linearColor += DoLightInternal( worldPos, worldNormal, 2, bHalfLambert, lightInfo );
		if ( nNumLights >= 4 )
			linearColor += DoLightInternal( worldPos, worldNormal, 3, bHalfLambert, lightInfo );
	}

	if( bDynamicLight )
	{
		linearColor += AmbientLight( worldNormal, ambientCube ); //ambient light is already remapped
	}

	return linearColor;
}

int4 FloatToInt( in float4 floats )
{
	return D3DCOLORtoUBYTE4( floats.zyxw / 255.001953125 );
}


float2 ComputeSphereMapTexCoords( in float3 reflectionVector, float4x4 viewModel )
{
	// transform reflection vector into view space
	reflectionVector = mul( reflectionVector, ( float3x3 )viewModel );

	// generate <rx ry rz+1>
	float3 tmp = float3( reflectionVector.x, reflectionVector.y, reflectionVector.z + 1.0f );

	// find 1 / len
	float ooLen = dot( tmp, tmp );
	ooLen = 1.0f / sqrt( ooLen );

	// tmp = tmp/|tmp| + 1
	tmp.xy = ooLen * tmp.xy + 1.0f;

	return tmp.xy * 0.5f;
}


#define DEFORMATION_CLAMP_TO_BOX_IN_WORLDSPACE 1
							// minxyz.minsoftness / maxxyz.maxsoftness
float3 ApplyDeformation( float3 worldpos, int deftype, float4 defparms0, float4 defparms1,
						 float4 defparms2, float4 defparms3 )
{
	float3 ret = worldpos;
	if ( deftype == DEFORMATION_CLAMP_TO_BOX_IN_WORLDSPACE )
	{
		ret=max( ret, defparms2.xyz );
		ret=min( ret, defparms3.xyz );
	}

	return ret;
}


#endif //#ifndef COMMON_VS_FXC_H_
