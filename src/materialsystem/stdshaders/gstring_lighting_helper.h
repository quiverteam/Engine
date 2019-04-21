#ifndef GSTRING_LIGHTING_HELPER
#define GSTRING_LIGHTING_HELPER

#include "materialsystem/ishaderapi.h"
#include "utlvector.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterial.h"

#include "materialsystem/itexture.h"

#include "materialsystem/imesh.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/materialsystem_config.h"
#include "shaderlib/ShaderDLL.h"

// The light state you can access through the ShaderAPI has a DIFFERENT order than what
// the materialsystem uses internally! Dynamic, fading lights are at the end, but there
// is no way to follow that behavior here!
// So all shaders here have to build and push the lightstate separately in custom code,
// to find the directional sunlight, pretty wasteful.

template < class S >
inline void SetCustomPixelLightingState( CCommandBufferBuilder< S > &DynamicCmdsOut, const LightState_t &lightState, IShaderDynamicAPI *pShaderAPI,
	int iConstantVar )
{
	const int maxLights = min(4, lightState.m_nNumLights);
	static Vector4D s_PixelLightingState[6] =
		{ vec4_origin, vec4_origin, vec4_origin, vec4_origin, vec4_origin, vec4_origin };

	if ( maxLights > 0 )
	{
		const int iNumFirstLights = min(3, maxLights);
		for ( int i = 0; i < iNumFirstLights; i++ )
		{
			const LightDesc_t &lightDesc = pShaderAPI->GetLight( i );
			const int iConstantOffset = 2 * i;
			Vector &vecPosition = s_PixelLightingState[ iConstantOffset + 1 ].AsVector3D();
			s_PixelLightingState[ iConstantOffset ].Init( lightDesc.m_Color.x, lightDesc.m_Color.y, lightDesc.m_Color.z, 0.0f );
			if ( lightDesc.m_Type == MATERIAL_LIGHT_DIRECTIONAL )
			{
				vecPosition = lightDesc.m_Position - lightDesc.m_Direction * 10000.0f;
			}
			else
			{
				vecPosition = lightDesc.m_Position;
			}
		}

		if ( maxLights > 3 )
		{
			const LightDesc_t &lightDesc = pShaderAPI->GetLight( 3 );
			s_PixelLightingState[ 0 ].w = lightDesc.m_Color.x;
			s_PixelLightingState[ 1 ].w = lightDesc.m_Color.y;
			s_PixelLightingState[ 2 ].w = lightDesc.m_Color.z;

			if ( lightDesc.m_Type == MATERIAL_LIGHT_DIRECTIONAL )
			{
				Vector vecPosition = lightDesc.m_Position - lightDesc.m_Direction * 10000.0f;
				s_PixelLightingState[ 3 ].w = vecPosition.x;
				s_PixelLightingState[ 4 ].w = vecPosition.y;
				s_PixelLightingState[ 5 ].w = vecPosition.z;
			}
			else
			{
				const Vector &vecPosition = lightDesc.m_Position;
				s_PixelLightingState[ 3 ].w = vecPosition.x;
				s_PixelLightingState[ 4 ].w = vecPosition.y;
				s_PixelLightingState[ 5 ].w = vecPosition.z;
			}
		}

		DynamicCmdsOut.SetPixelShaderConstant( iConstantVar, s_PixelLightingState[ 0 ].Base(), 6 );
	}
}

template < class S, size_t C >
inline void SetCustomVertexLightingState( CCommandBufferBuilder< S > &DynamicCmdsOut, const LightState_t &lightState, IShaderDynamicAPI *pShaderAPI,
	int iConstantVar, float (&vDirectionalLights)[C] )
{
	const int maxLights = min(min(4, lightState.m_nNumLights), (int)C);
	static Vector4D s_VertexLightingState[20];

	if ( maxLights > 0 )
	{
		for ( int i = 0; i < maxLights; i++ )
		{
			const LightDesc_t &lightDesc = pShaderAPI->GetLight( i );
			Vector4D *pDest = &s_VertexLightingState[ i * 5 ];

			float w = ( lightDesc.m_Type == MATERIAL_LIGHT_DIRECTIONAL ) ? 1.0f : 0.0f;
			pDest[0].Init( lightDesc.m_Color.x, lightDesc.m_Color.y, lightDesc.m_Color.z, w);

			vDirectionalLights[i] = w;

			w = ( lightDesc.m_Type == MATERIAL_LIGHT_SPOT ) ? 1.0f : 0.0f;
			pDest[1].Init( lightDesc.m_Direction.x, lightDesc.m_Direction.y, lightDesc.m_Direction.z, w );
			pDest[2].Init( lightDesc.m_Position.x, lightDesc.m_Position.y, lightDesc.m_Position.z, 1.0f );

			if ( lightDesc.m_Type == MATERIAL_LIGHT_SPOT )
			{
				// No, thanks.
				//const_cast< LightDesc_t* >( &lightDesc )->RecalculateDerivedValues();

				float stopdot = cos( lightDesc.m_Theta * 0.5f );
				float stopdot2 = cos( lightDesc.m_Phi * 0.5f );
				float oodot = (stopdot > stopdot2) ? 1.0f / (stopdot - stopdot2) : 0.0f;
				pDest[3].Init( lightDesc.m_Falloff, stopdot, stopdot2, oodot );
				
				/*float spotParamW;
				const float flThetaPhiDelta = lightDesc.m_ThetaDot - lightDesc.m_PhiDot;
				if ( flThetaPhiDelta > 1.0e-10f )
				{
					spotParamW = 1.0f / flThetaPhiDelta;
				}
				else
				{
					spotParamW = 1.0f;
				}
				pDest[3].Init( lightDesc.m_Falloff, lightDesc.m_ThetaDot, lightDesc.m_PhiDot, spotParamW );*/
			}
			else
			{
				pDest[3].Init( 0, 1, 1, 1 );
			}

			pDest[4].Init( lightDesc.m_Attenuation0, lightDesc.m_Attenuation1, lightDesc.m_Attenuation2, 0.0f );
		}

		DynamicCmdsOut.SetVertexShaderConstant( iConstantVar, s_VertexLightingState[ 0 ].Base(), maxLights * 5 );
	}
}

#endif