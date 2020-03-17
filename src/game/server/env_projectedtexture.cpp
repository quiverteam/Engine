//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: Entity to control screen overlays on a player
//
//=============================================================================

#include "cbase.h"
#include "shareddefs.h"
#include "env_projectedtexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar create_flashlight_quadratic( "create_flashlight_quadratic", "150", 0 );

LINK_ENTITY_TO_CLASS( env_projectedtexture, CEnvProjectedTexture );

BEGIN_DATADESC( CEnvProjectedTexture )
	DEFINE_FIELD( m_hTargetEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bState, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_flLightFOV, FIELD_FLOAT, "lightfov" ),
	DEFINE_KEYFIELD( m_bEnableShadows, FIELD_BOOLEAN, "enableshadows" ),
	DEFINE_KEYFIELD( m_bLightOnlyTarget, FIELD_BOOLEAN, "lightonlytarget" ),
	DEFINE_KEYFIELD( m_bLightWorld, FIELD_BOOLEAN, "lightworld" ),
	DEFINE_KEYFIELD( m_bCameraSpace, FIELD_BOOLEAN, "cameraspace" ),
	DEFINE_KEYFIELD( m_flAmbient, FIELD_FLOAT, "ambient" ),
	DEFINE_AUTO_ARRAY_KEYFIELD( m_SpotlightTextureName, FIELD_CHARACTER, "texturename" ),
	DEFINE_KEYFIELD( m_nSpotlightTextureFrame, FIELD_INTEGER, "textureframe" ),
	DEFINE_KEYFIELD( m_flNearZ, FIELD_FLOAT, "nearz" ),
	DEFINE_KEYFIELD( m_flFarZ, FIELD_FLOAT, "farz" ),
	DEFINE_KEYFIELD( m_nShadowQuality, FIELD_INTEGER, "shadowquality" ),

	DEFINE_FIELD( m_LinearFloatLightColor, FIELD_VECTOR ), 
	DEFINE_KEYFIELD( m_nLinear, FIELD_INTEGER, "linear" ),
	DEFINE_KEYFIELD( m_nQuadratic, FIELD_INTEGER, "quadratic" ),
	DEFINE_KEYFIELD( m_nConstant, FIELD_INTEGER, "constant" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"AlwaysUpdateOn",	InputAlwaysUpdateOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"AlwaysUpdateOff",	InputAlwaysUpdateOff ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "FOV", InputSetFOV ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "Target", InputSetTarget ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "CameraSpace", InputSetCameraSpace ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "LightOnlyTarget", InputSetLightOnlyTarget ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "LightWorld", InputSetLightWorld ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "EnableShadows", InputSetEnableShadows ),
	// this is broken . . need to be able to set color and intensity like light_dynamic
//	DEFINE_INPUTFUNC( FIELD_COLOR32, "LightColor", InputSetLightColor ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Ambient", InputSetAmbient ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SpotlightTexture", InputSetSpotlightTexture ),
	DEFINE_THINKFUNC( InitialThink ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CEnvProjectedTexture, DT_EnvProjectedTexture )
	SendPropEHandle( SENDINFO( m_hTargetEntity ) ),
	SendPropBool( SENDINFO( m_bState ) ),
	SendPropBool( SENDINFO( m_bAlwaysUpdate ) ),
	SendPropFloat( SENDINFO( m_flLightFOV ) ),
	SendPropBool( SENDINFO( m_bEnableShadows ) ),
	SendPropBool( SENDINFO( m_bLightOnlyTarget ) ),
	SendPropBool( SENDINFO( m_bLightWorld ) ),
	SendPropBool( SENDINFO( m_bCameraSpace ) ),

	SendPropVector( SENDINFO( m_LinearFloatLightColor ) ),
	SendPropInt( SENDINFO( m_nLinear ) ),
	SendPropInt( SENDINFO( m_nQuadratic ) ),
	SendPropInt( SENDINFO( m_nConstant ) ),

	SendPropFloat( SENDINFO( m_flAmbient ) ),
	SendPropString( SENDINFO( m_SpotlightTextureName ) ),
	SendPropInt( SENDINFO( m_nSpotlightTextureFrame ) ),
	SendPropFloat( SENDINFO( m_flNearZ ), 16, SPROP_ROUNDDOWN, 0.0f,  500.0f ),
	SendPropFloat( SENDINFO( m_flFarZ ),  18, SPROP_ROUNDDOWN, 0.0f, 1500.0f ),
	SendPropInt( SENDINFO( m_nShadowQuality ), 1, SPROP_UNSIGNED ),  // Just one bit for now
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEnvProjectedTexture::CEnvProjectedTexture( void )
{
	m_bState = true;
	m_bAlwaysUpdate = false;
	m_flLightFOV = 45.0f;
	m_bEnableShadows = false;
	m_bLightOnlyTarget = false;
	m_bLightWorld = true;
	m_bCameraSpace = false;

	/*m_bUberlight = false;
	m_fNearEdge = 0.f;
	m_fFarEdge = 0.f;
	m_fCutOn = 0.f;
	m_fCutOff = 0.f;
	m_fShearx = 0.f;
	m_fSheary = 0.f;
	m_fWidth = 0.f;
	m_fWedge = 0.f;
	m_fHeight = 0.f;
	m_fHedge = 0.f;
	m_fRoundness = 0.f;*/

// this is here as a reminder since it's not removed from everything else yet,
// only remove it once this is completely gone, whatever it is
// if ( g_pHardwareConfig->SupportsBorderColor() )
	//Q_strcpy( m_SpotlightTextureName.GetForModify(), "effects/flashlight_border" );
	Q_strcpy( m_SpotlightTextureName.GetForModify(), "effects/flashlight001" );

	m_nSpotlightTextureFrame = 0;
	m_LinearFloatLightColor.Init( 1.0f, 1.0f, 1.0f );
	m_flAmbient = 0.0f;
	m_flNearZ = 4.0f;
	m_flFarZ = 750.0f;
	m_nShadowQuality = 0;
}

void UTIL_ColorStringToLinearFloatColor( Vector &color, const char *pString )
{
	float tmp[4];
	UTIL_StringToFloatArray( tmp, 4, pString );
	if( tmp[3] <= 0.0f )
	{
		tmp[3] = 255.0f;
	}
	tmp[3] *= ( 1.0f / 255.0f );
	color.x = GammaToLinear( tmp[0] * ( 1.0f / 255.0f ) ) * tmp[3];
	color.y = GammaToLinear( tmp[1] * ( 1.0f / 255.0f ) ) * tmp[3];
	color.z = GammaToLinear( tmp[2] * ( 1.0f / 255.0f ) ) * tmp[3];
}

bool CEnvProjectedTexture::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "lightcolor" ) )
	{
		Vector tmp;
		UTIL_ColorStringToLinearFloatColor( tmp, szValue );
		m_LinearFloatLightColor = tmp;
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

void CEnvProjectedTexture::InputTurnOn( inputdata_t &inputdata )
{
	m_bState = true;
}

void CEnvProjectedTexture::InputTurnOff( inputdata_t &inputdata )
{
	m_bState = false;
}

void CEnvProjectedTexture::InputAlwaysUpdateOn( inputdata_t &inputdata )
{
	m_bAlwaysUpdate = true;
}

void CEnvProjectedTexture::InputAlwaysUpdateOff( inputdata_t &inputdata )
{
	m_bAlwaysUpdate = false;
}

void CEnvProjectedTexture::InputSetFOV( inputdata_t &inputdata )
{
	m_flLightFOV = inputdata.value.Float();
}

void CEnvProjectedTexture::InputSetTarget( inputdata_t &inputdata )
{
	m_hTargetEntity = inputdata.value.Entity();
}

void CEnvProjectedTexture::InputSetCameraSpace( inputdata_t &inputdata )
{
	m_bCameraSpace = inputdata.value.Bool();
}

void CEnvProjectedTexture::InputSetLightOnlyTarget( inputdata_t &inputdata )
{
	m_bLightOnlyTarget = inputdata.value.Bool();
}

void CEnvProjectedTexture::InputSetLightWorld( inputdata_t &inputdata )
{
	m_bLightWorld = inputdata.value.Bool();
}

void CEnvProjectedTexture::InputSetEnableShadows( inputdata_t &inputdata )
{
	m_bEnableShadows = inputdata.value.Bool();
}

//void CEnvProjectedTexture::InputSetLightColor( inputdata_t &inputdata )
//{
//	m_cLightColor = inputdata.value.Color32();
//}

void CEnvProjectedTexture::InputSetAmbient( inputdata_t &inputdata )
{
	m_flAmbient = inputdata.value.Float();
}

void CEnvProjectedTexture::InputSetSpotlightTexture( inputdata_t &inputdata )
{
	Q_strcpy( m_SpotlightTextureName.GetForModify(), inputdata.value.String() );
}

void CEnvProjectedTexture::Activate( void )
{
	m_bState = ( ( GetSpawnFlags() & ENV_PROJECTEDTEXTURE_STARTON ) != 0 );
	m_bAlwaysUpdate = ( ( GetSpawnFlags() & ENV_PROJECTEDTEXTURE_ALWAYSUPDATE ) != 0 );

	SetThink( &CEnvProjectedTexture::InitialThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	BaseClass::Activate();
}

void CEnvProjectedTexture::InitialThink( void )
{
	if ( m_hTargetEntity == NULL && m_target != NULL_STRING )
		m_hTargetEntity = gEntList.FindEntityByName( NULL, m_target );
	if ( m_hTargetEntity == NULL )
		return;

	Vector vecToTarget = ( m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin() );
	QAngle vecAngles;
	VectorAngles( vecToTarget, vecAngles );
	SetAbsAngles( vecAngles );

	SetNextThink( gpGlobals->curtime + 0.1 );
}

int CEnvProjectedTexture::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

// Console command for creating env_projectedtexture entities
void CC_CreateFlashlight( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if( !pPlayer )
		return;

	QAngle angles = pPlayer->EyeAngles();
	Vector origin = pPlayer->EyePosition();		

	CEnvProjectedTexture *pFlashlight = dynamic_cast< CEnvProjectedTexture * >( CreateEntityByName("env_projectedtexture") );
	if( args.ArgC() > 1 )
	{
		pFlashlight->SetName( AllocPooledString( args[1] ) );
	}
	
	pFlashlight->m_bAlwaysUpdate = true;
	pFlashlight->m_bEnableShadows = true;
	pFlashlight->m_nQuadratic = create_flashlight_quadratic.GetInt();
	pFlashlight->m_flLightFOV = 90;
	pFlashlight->m_flNearZ = 4;
	pFlashlight->m_flFarZ = 1500;

	pFlashlight->Teleport( &origin, &angles, NULL );
}
static ConCommand create_flashlight("create_flashlight", CC_CreateFlashlight, 0, FCVAR_CHEAT);
