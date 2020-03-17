//====== Copyright � 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_env_projectedtexture.h"
#include "shareddefs.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "texture_group_names.h"
#include "tier0/icommandline.h"

// shoud really remove and put in cbase.h, like in ASW
#include "vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

float C_EnvProjectedTexture::m_flVisibleBBoxMinHeight = -FLT_MAX;

extern ConVar r_shadowmapresolution;

static ConVar r_projtex_filtersize( "r_projtex_filtersize", "1.0", 0 );

static ConVar r_projtex_uberlight_enable( "r_projtex_uberlight_enable", "0", 0 );

/*static ConVar r_projtex_r( "r_projtex_r", "10", 0 );
static ConVar r_projtex_g( "r_projtex_g", "10", 0 );
static ConVar r_projtex_b( "r_projtex_b", "2", 0 );*/

static ConVar r_projtex_quadratic( "r_projtex_quadratic", "0", 0 );
static ConVar r_projtex_linear( "r_projtex_linear", "100", 0 );
static ConVar r_projtex_constant( "r_projtex_constant", "0", 0 );

static ConVar r_projtex_shadowatten( "r_projtex_shadowatten", "0.0", 0 );

/*static ConVar r_projtex_slopescale( "r_projtex_slopescale", "3.0", 0 );
static ConVar r_projtex_depthbias( "r_projtex_depthbias", "0.00001", 0 );*/

// maybe rename to r_shadowmap_quality or r_shadowmap_sharpness?
static ConVar r_projtex_quality( "r_projtex_quality", "-1", FCVAR_ARCHIVE,
	"\n-1 to use custom settings \n0 - 512 with filter size of 2.0 \n1 - 1024 with filter size of 1.0 \n2 - 2048 with filter size of 0.5 \n3 - 4096 with filter size of 0.25 (overkill) \n4 - 8192 with filter size of 0.125 (RIP PC) \nMap needs to be reloaded to completely take effect atm." );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_EnvProjectedTexture, DT_EnvProjectedTexture, CEnvProjectedTexture )
	RecvPropEHandle( RECVINFO( m_hTargetEntity )	),
	RecvPropBool(	 RECVINFO( m_bState )			),
	RecvPropBool(	 RECVINFO( m_bAlwaysUpdate )	),
	RecvPropFloat(	 RECVINFO( m_flLightFOV )		),
	RecvPropBool(	 RECVINFO( m_bEnableShadows )	),
	RecvPropBool(	 RECVINFO( m_bLightOnlyTarget ) ),
	RecvPropBool(	 RECVINFO( m_bLightWorld )		),
	RecvPropBool(	 RECVINFO( m_bCameraSpace )		),

	RecvPropVector(	 RECVINFO( m_LinearFloatLightColor )		),
	RecvPropInt(	 RECVINFO( m_nLinear )	),
	RecvPropInt(	 RECVINFO( m_nQuadratic )	),
	RecvPropInt(	 RECVINFO( m_nConstant )	),

	RecvPropFloat(	 RECVINFO( m_flAmbient )		),
	RecvPropString(  RECVINFO( m_SpotlightTextureName ) ),
	RecvPropInt(	 RECVINFO( m_nSpotlightTextureFrame ) ),
	RecvPropFloat(	 RECVINFO( m_flNearZ )	),
	RecvPropFloat(	 RECVINFO( m_flFarZ )	),
	RecvPropInt(	 RECVINFO( m_nShadowQuality )	),
END_RECV_TABLE()

C_EnvProjectedTexture *C_EnvProjectedTexture::Create( )
{
	C_EnvProjectedTexture *pEnt = new C_EnvProjectedTexture();

	pEnt->m_flNearZ = 4.0f;
	pEnt->m_flFarZ = 2000.0f;
//	strcpy( pEnt->m_SpotlightTextureName, "particle/rj" );
	pEnt->m_bLightWorld = true;
	pEnt->m_bLightOnlyTarget = false;
//	pEnt->m_bSimpleProjection = false;
	pEnt->m_nShadowQuality = 1;
	pEnt->m_flLightFOV = 45.0f;

	pEnt->m_LinearFloatLightColor.x = 255;
	pEnt->m_LinearFloatLightColor.y = 255;
	pEnt->m_LinearFloatLightColor.z = 255;
//	pEnt->m_LightColor.r = 255;
//	pEnt->m_LightColor.g = 255;
//	pEnt->m_LightColor.b = 255;
//	pEnt->m_LightColor.a = 255;

	pEnt->m_bEnableShadows = false;
//	pEnt->m_flColorTransitionTime = 1.0f;
	pEnt->m_bCameraSpace = false;
	pEnt->SetAbsAngles( QAngle( 90, 0, 0 ) );
	pEnt->m_bAlwaysUpdate = true;
	pEnt->m_bState = true;
//	pEnt->m_flProjectionSize = 500.0f;
//	pEnt->m_flRotation = 0.0f;

	return pEnt;
}

C_EnvProjectedTexture::C_EnvProjectedTexture( void )
{
	m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
}

C_EnvProjectedTexture::~C_EnvProjectedTexture( void )
{
	ShutDownLightHandle();
}

void C_EnvProjectedTexture::ShutDownLightHandle( void )
{
	// Clear out the light
	if( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_LightHandle );
		m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_EnvProjectedTexture::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		// still need to get uberlight working to test this
		m_SpotlightTexture.Init( /*m_FlashlightState.m_bUberlight ? "white" : */m_SpotlightTextureName, TEXTURE_GROUP_OTHER, true );
	}

	m_bForceUpdate = true;
	UpdateLight();
	BaseClass::OnDataChanged( updateType );
}

void C_EnvProjectedTexture::UpdateLight( void )
{
	VPROF("C_EnvProjectedTexture::UpdateLight");
	bool bVisible = true;

	if ( m_bAlwaysUpdate )
	{
		m_bForceUpdate = true;
	}

	if ( !m_bForceUpdate )
	{
		bVisible = IsBBoxVisible();		
	}

	if ( m_bState == false || !bVisible )
	{
		ShutDownLightHandle();
		return;
	}
	
	if ( m_LightHandle == CLIENTSHADOW_INVALID_HANDLE || m_hTargetEntity != NULL || m_bForceUpdate )
	{
		Vector vForward, vRight, vUp, vPos = GetAbsOrigin();
		FlashlightState_t state;

		if ( m_hTargetEntity != NULL )
		{
			if ( m_bCameraSpace )
			{
				const QAngle& angles = GetLocalAngles();

				C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
				if ( pPlayer )
				{
					const QAngle playerAngles = pPlayer->GetAbsAngles();

					Vector vPlayerForward, vPlayerRight, vPlayerUp;
					AngleVectors( playerAngles, &vPlayerForward, &vPlayerRight, &vPlayerUp );

					matrix3x4_t	mRotMatrix;
					AngleMatrix( angles, mRotMatrix );

					VectorITransform( vPlayerForward, mRotMatrix, vForward );
					VectorITransform( vPlayerRight, mRotMatrix, vRight );
					VectorITransform( vPlayerUp, mRotMatrix, vUp );

					float dist = ( m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin() ).Length();
					vPos = m_hTargetEntity->GetAbsOrigin() - vForward * dist;

					VectorNormalize( vForward );
					VectorNormalize( vRight );
					VectorNormalize( vUp );
				}
			}
			else
			{
				// VXP: Fixing targeting
				Vector vecToTarget;
				QAngle vecAngles;
				if ( m_hTargetEntity == NULL )
				{
					vecAngles = GetAbsAngles();
				}
				else
				{
					vecToTarget = m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin();
					VectorAngles( vecToTarget, vecAngles );
				}
				AngleVectors( vecAngles, &vForward, &vRight, &vUp );
			}
		}
		else
		{
			AngleVectors( GetAbsAngles(), &vForward, &vRight, &vUp );
		}

		state.m_fHorizontalFOVDegrees = m_flLightFOV;
		state.m_fVerticalFOVDegrees = m_flLightFOV;

		state.m_vecLightOrigin = vPos;
		BasisToQuaternion( vForward, vRight, vUp, state.m_quatOrientation );

		// quickly check the proposed light's bbox against the view frustum to determine whether we
		// should bother to create it, if it doesn't exist, or cull it, if it does.
#pragma message("OPTIMIZATION: this should be made SIMD")
		// get the half-widths of the near and far planes, 
		// based on the FOV which is in degrees. Remember that
		// on planet Valve, x is forward, y left, and z up. 
		const float tanHalfAngle = tan( m_flLightFOV * ( M_PI / 180.0f ) * 0.5f );
		const float halfWidthNear = tanHalfAngle * m_flNearZ;
		const float halfWidthFar = tanHalfAngle * m_flFarZ;
		// now we can build coordinates in local space: the near rectangle is eg 
		// (0, -halfWidthNear, -halfWidthNear), (0,  halfWidthNear, -halfWidthNear), 
		// (0,  halfWidthNear,  halfWidthNear), (0, -halfWidthNear,  halfWidthNear)

		VectorAligned vNearRect[ 4 ] = {
			VectorAligned( m_flNearZ, -halfWidthNear, -halfWidthNear ), VectorAligned( m_flNearZ,  halfWidthNear, -halfWidthNear ),
			VectorAligned( m_flNearZ,  halfWidthNear,  halfWidthNear ), VectorAligned( m_flNearZ, -halfWidthNear,  halfWidthNear )
		};

		VectorAligned vFarRect[ 4 ] = {
			VectorAligned( m_flFarZ, -halfWidthFar, -halfWidthFar ), VectorAligned( m_flFarZ,  halfWidthFar, -halfWidthFar ),
			VectorAligned( m_flFarZ,  halfWidthFar,  halfWidthFar ), VectorAligned( m_flFarZ, -halfWidthFar,  halfWidthFar )
		};

		matrix3x4_t matOrientation( vForward, -vRight, vUp, vPos );

		enum
		{
			kNEAR = 0,
			kFAR = 1,
		};
		VectorAligned vOutRects[ 2 ][ 4 ];

		for ( int i = 0; i < 4; ++i )
		{
			VectorTransform( vNearRect[ i ].Base(), matOrientation, vOutRects[ 0 ][ i ].Base() );
		}
		for ( int i = 0; i < 4; ++i )
		{
			VectorTransform( vFarRect[ i ].Base(), matOrientation, vOutRects[ 1 ][ i ].Base() );
		}

		// now take the min and max extents for the bbox, and see if it is visible.
		Vector mins = **vOutRects;
		Vector maxs = **vOutRects;
		for ( int i = 1; i < 8; ++i )
		{
			VectorMin( mins, *( *vOutRects + i ), mins );
			VectorMax( maxs, *( *vOutRects + i ), maxs );
		}

#if 0 //for debugging the visibility frustum we just calculated
		NDebugOverlay::Triangle( vOutRects[ 0 ][ 0 ], vOutRects[ 0 ][ 1 ], vOutRects[ 0 ][ 2 ], 255, 0, 0, 100, true, 0.0f ); //first tri
		NDebugOverlay::Triangle( vOutRects[ 0 ][ 2 ], vOutRects[ 0 ][ 1 ], vOutRects[ 0 ][ 0 ], 255, 0, 0, 100, true, 0.0f ); //make it double sided
		NDebugOverlay::Triangle( vOutRects[ 0 ][ 2 ], vOutRects[ 0 ][ 3 ], vOutRects[ 0 ][ 0 ], 255, 0, 0, 100, true, 0.0f ); //second tri
		NDebugOverlay::Triangle( vOutRects[ 0 ][ 0 ], vOutRects[ 0 ][ 3 ], vOutRects[ 0 ][ 2 ], 255, 0, 0, 100, true, 0.0f ); //make it double sided

		NDebugOverlay::Triangle( vOutRects[ 1 ][ 0 ], vOutRects[ 1 ][ 1 ], vOutRects[ 1 ][ 2 ], 0, 0, 255, 100, true, 0.0f ); //first tri
		NDebugOverlay::Triangle( vOutRects[ 1 ][ 2 ], vOutRects[ 1 ][ 1 ], vOutRects[ 1 ][ 0 ], 0, 0, 255, 100, true, 0.0f ); //make it double sided
		NDebugOverlay::Triangle( vOutRects[ 1 ][ 2 ], vOutRects[ 1 ][ 3 ], vOutRects[ 1 ][ 0 ], 0, 0, 255, 100, true, 0.0f ); //second tri
		NDebugOverlay::Triangle( vOutRects[ 1 ][ 0 ], vOutRects[ 1 ][ 3 ], vOutRects[ 1 ][ 2 ], 0, 0, 255, 100, true, 0.0f ); //make it double sided

		NDebugOverlay::Box( vec3_origin, mins, maxs, 0, 255, 0, 100, 0.0f );
#endif

		bVisible = IsBBoxVisible( mins, maxs );
		if ( !bVisible )
		{
			// Spotlight's extents aren't in view
			if ( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
			{
				ShutDownLightHandle();
			}

			return;
		}

		/*state.m_fQuadraticAtten = r_projtex_quadratic.GetInt(); //0
		state.m_fLinearAtten = r_projtex_linear.GetInt(); //100
		state.m_fConstantAtten = r_projtex_constant.GetInt(); //0*/

		state.m_fQuadraticAtten = m_nQuadratic; //0
		state.m_fLinearAtten = m_nLinear; //100
		state.m_fConstantAtten = m_nConstant;

		state.m_Color[ 0 ] = m_LinearFloatLightColor.x;
		state.m_Color[ 1 ] = m_LinearFloatLightColor.y;
		state.m_Color[ 2 ] = m_LinearFloatLightColor.z;
		state.m_Color[ 3 ] = 0.0f; // fixme: need to make ambient work m_flAmbient;
		state.m_NearZ = m_flNearZ;
		state.m_FarZ = m_flFarZ;
		//state.m_flShadowSlopeScaleDepthBias = mat_slopescaledepthbias_shadowmap.GetFloat();
		state.m_flShadowSlopeScaleDepthBias = g_pMaterialSystemHardwareConfig->GetShadowSlopeScaleDepthBias();
		//state.m_flShadowDepthBias = mat_depthbias_shadowmap.GetFloat();
		state.m_flShadowDepthBias = g_pMaterialSystemHardwareConfig->GetShadowDepthBias();
		state.m_bEnableShadows = m_bEnableShadows;
		//state.m_pSpotlightTexture = materials->FindTexture( m_SpotlightTextureName, TEXTURE_GROUP_OTHER, false );
		state.m_pSpotlightTexture = m_SpotlightTexture;
		state.m_nSpotlightTextureFrame = m_nSpotlightTextureFrame;

		state.m_bUberlight = r_projtex_uberlight_enable.GetBool();
		m_UberlightState.m_bEnabled = r_projtex_uberlight_enable.GetBool();
		m_UberlightState.m_fNearEdge = m_fNearEdge;
		m_UberlightState.m_fFarEdge = m_fFarEdge;
		m_UberlightState.m_fCutOn = m_fCutOn;
		m_UberlightState.m_fCutOff = m_fCutOff;
		m_UberlightState.m_fShearx = m_fShearx;
		m_UberlightState.m_fSheary = m_fSheary;
		m_UberlightState.m_fWidth = m_fWidth;
		m_UberlightState.m_fWedge = m_fWedge;
		m_UberlightState.m_fHeight = m_fHeight;
		m_UberlightState.m_fHedge = m_fHedge;
		m_UberlightState.m_fRoundness = m_fRoundness;

		state.m_flShadowAtten = r_projtex_shadowatten.GetFloat();
		state.m_nShadowQuality = m_nShadowQuality; // Allow entity to affect shadow quality

		// NOTE: need to reload map when changing filtersize for this entity apparently, not for the flashlight though
		// also the filtersize change only takes effect on map reload aaaa
		// need to check to see if one of the values are changed, then set this to -1
		// also this is all setup for DoShadowNvidiaPCF5x5Gaussian lol
		if ( r_projtex_quality.GetInt() == 0 )
		{
			r_shadowmapresolution.SetValue( 512.0f );
			r_projtex_filtersize.SetValue( 2.0f );
		}
		else if ( r_projtex_quality.GetInt() == 1 )
		{
			r_shadowmapresolution.SetValue( 1024.0f );
			r_projtex_filtersize.SetValue( 1.0f );
		}
		else if ( r_projtex_quality.GetInt() == 2 )
		{
			r_shadowmapresolution.SetValue( 2048.0f );
			r_projtex_filtersize.SetValue( 0.5f );
		}
		else if ( r_projtex_quality.GetInt() == 3 )
		{
			r_shadowmapresolution.SetValue( 4096.0f );
			r_projtex_filtersize.SetValue( 0.25f );
		}
		else if ( r_projtex_quality.GetInt() == 4 )
		{
			r_shadowmapresolution.SetValue( 8192.0f );
			r_projtex_filtersize.SetValue( 0.125f );
		}

		state.m_flShadowFilterSize = r_projtex_filtersize.GetFloat();

		if ( m_LightHandle == CLIENTSHADOW_INVALID_HANDLE )
		{
			m_LightHandle = g_pClientShadowMgr->CreateFlashlight( state );

			if ( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
			{
				m_bForceUpdate = false;
			}
		}
		else
		{
			if ( m_hTargetEntity != NULL || m_bForceUpdate == true )
			{
				g_pClientShadowMgr->UpdateFlashlightState( m_LightHandle, state );
			}

			//g_pClientShadowMgr->UpdateFlashlightState( m_LightHandle, state );
			m_bForceUpdate = false;
		}

		g_pClientShadowMgr->GetFrustumExtents( m_LightHandle, m_vecExtentsMin, m_vecExtentsMax );

		m_vecExtentsMin = m_vecExtentsMin - GetAbsOrigin();
		m_vecExtentsMax = m_vecExtentsMax - GetAbsOrigin();
	}

	if( m_bLightOnlyTarget )
	{
		g_pClientShadowMgr->SetFlashlightTarget( m_LightHandle, m_hTargetEntity );
	}
	else
	{
		g_pClientShadowMgr->SetFlashlightTarget( m_LightHandle, NULL );
	}

	g_pClientShadowMgr->SetFlashlightLightWorld( m_LightHandle, m_bLightWorld );

	if ( !m_bForceUpdate )
	{
		g_pClientShadowMgr->UpdateProjectedTexture( m_LightHandle, true );
	}
}

void C_EnvProjectedTexture::Simulate( void )
{
	UpdateLight();

	BaseClass::Simulate();
	//return true;
}

bool C_EnvProjectedTexture::IsBBoxVisible( Vector vecExtentsMin, Vector vecExtentsMax )
{
	// Z position clamped to the min height (but must be less than the max)
	float flVisibleBBoxMinHeight = MIN( vecExtentsMax.z - 1.0f, m_flVisibleBBoxMinHeight );
	vecExtentsMin.z = MAX( vecExtentsMin.z, flVisibleBBoxMinHeight );

	// Check if the bbox is in the view
	return !engine->CullBox( vecExtentsMin, vecExtentsMax );
}
