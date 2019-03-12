//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: spawn and think functions for editor-placed lights
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "lights.h"
#include "world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( light, CLight );

BEGIN_DATADESC( CLight )

	DEFINE_FIELD( m_iCurrentFade, FIELD_CHARACTER),
	DEFINE_FIELD( m_iTargetFade, FIELD_CHARACTER),

	DEFINE_KEYFIELD( m_iStyle, FIELD_INTEGER, "style" ),
	DEFINE_KEYFIELD( m_iDefaultStyle, FIELD_INTEGER, "defaultstyle" ),
	DEFINE_KEYFIELD( m_iszPattern, FIELD_STRING, "pattern" ),

	// Fuctions
	DEFINE_FUNCTION( FadeThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "SetPattern", InputSetPattern ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FadeToPattern", InputFadeToPattern ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"TurnOff", InputTurnOff ),

END_DATADESC()



//
// Cache user-entity-field values until spawn is called.
//
bool CLight::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "pitch"))
	{
		QAngle angles = GetAbsAngles();
		angles.x = atof(szValue);
		SetAbsAngles( angles );
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

// Light entity
// If targeted, it will toggle between on or off.
void CLight::Spawn( void )
{
	if (!GetEntityName())
	{       // inert light
		UTIL_Remove( this );
		return;
	}
	
	if (m_iStyle >= 32)
	{
		if ( m_iszPattern == NULL_STRING && m_iDefaultStyle > 0 )
		{
			m_iszPattern = MAKE_STRING(GetDefaultLightstyleString(m_iDefaultStyle));
		}

		if (FBitSet(m_spawnflags, SF_LIGHT_START_OFF))
			engine->LightStyle(m_iStyle, "a");
		else if (m_iszPattern != NULL_STRING)
			engine->LightStyle(m_iStyle, (char *)STRING( m_iszPattern ));
		else
			engine->LightStyle(m_iStyle, "m");
	}
}


void CLight::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_iStyle >= 32)
	{
		if ( !ShouldToggle( useType, !FBitSet(m_spawnflags, SF_LIGHT_START_OFF) ) )
			return;

		Toggle();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turn the light on
//-----------------------------------------------------------------------------
void CLight::TurnOn( void )
{
	if ( m_iszPattern != NULL_STRING )
	{
		engine->LightStyle( m_iStyle, (char *) STRING( m_iszPattern ) );
	}
	else
	{
		engine->LightStyle( m_iStyle, "m" );
	}

	CLEARBITS( m_spawnflags, SF_LIGHT_START_OFF );
}

//-----------------------------------------------------------------------------
// Purpose: Turn the light off
//-----------------------------------------------------------------------------
void CLight::TurnOff( void )
{
	engine->LightStyle( m_iStyle, "a" );
	SETBITS( m_spawnflags, SF_LIGHT_START_OFF );
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the light on/off
//-----------------------------------------------------------------------------
void CLight::Toggle( void )
{
	//Toggle it
	if ( FBitSet( m_spawnflags, SF_LIGHT_START_OFF ) )
	{
		TurnOn();
	}
	else
	{
		TurnOff();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle the "turnon" input handler
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CLight::InputTurnOn( inputdata_t &inputdata )
{
	TurnOn();
}

//-----------------------------------------------------------------------------
// Purpose: Handle the "turnoff" input handler
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CLight::InputTurnOff( inputdata_t &inputdata )
{
	TurnOff();
}

//-----------------------------------------------------------------------------
// Purpose: Handle the "toggle" input handler
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CLight::InputToggle( inputdata_t &inputdata )
{
	Toggle();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting a light pattern
//-----------------------------------------------------------------------------
void CLight::InputSetPattern( inputdata_t &inputdata )
{
	m_iszPattern = inputdata.value.StringID();
	engine->LightStyle(m_iStyle, (char *)STRING( m_iszPattern ));

	// Light is on if pattern is set
	CLEARBITS(m_spawnflags, SF_LIGHT_START_OFF);
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for fading from first value in old pattern to 
//			first value in new pattern
//-----------------------------------------------------------------------------
void CLight::InputFadeToPattern( inputdata_t &inputdata )
{
	m_iCurrentFade	= (STRING(m_iszPattern))[0];
	m_iTargetFade	= inputdata.value.String()[0];
	m_iszPattern	= inputdata.value.StringID();
	SetThink(&CLight::FadeThink);
	SetNextThink( gpGlobals->curtime );

	// Light is on if pattern is set
	CLEARBITS(m_spawnflags, SF_LIGHT_START_OFF);
}


//------------------------------------------------------------------------------
// Purpose : Fade light to new starting pattern value then stop thinking
//------------------------------------------------------------------------------
void CLight::FadeThink(void)
{
	if (m_iCurrentFade < m_iTargetFade)
	{
		m_iCurrentFade++;
	}
	else if (m_iCurrentFade > m_iTargetFade)
	{
		m_iCurrentFade--;
	}

	// If we're done fading instantiate our light pattern and stop thinking
	if (m_iCurrentFade == m_iTargetFade)
	{
		engine->LightStyle(m_iStyle, (char *)STRING( m_iszPattern ));
		SetNextThink( TICK_NEVER_THINK );
	}
	// Otherwise instantiate our current fade value and keep thinking
	else
	{
		char sCurString[2];
		sCurString[0] = m_iCurrentFade;
		sCurString[1] = NULL;
		engine->LightStyle(m_iStyle, sCurString);

		// UNDONE: Consider making this settable war to control fade speed
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//
// shut up spawn functions for new spotlights
//
LINK_ENTITY_TO_CLASS( light_spot, CLight );
LINK_ENTITY_TO_CLASS( light_glspot, CLight );


class CEnvLight : public CBaseEntity
{
public:
	DECLARE_CLASS( CEnvLight, CBaseEntity );
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CEnvLight();

	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	
	virtual void Spawn();

	virtual int ObjectCaps()
	{
		return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	}

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

private:
	CNetworkQAngle( m_angSunAngles );
	CNetworkVector( m_vecLight );
	CNetworkVector( m_vecAmbient );
	CNetworkVar( bool, m_bCascadedShadowMappingEnabled );
	bool m_bHasHDRLightSet;
	bool m_bHasHDRAmbientSet;
};

LINK_ENTITY_TO_CLASS( light_environment, CEnvLight );

BEGIN_DATADESC( CEnvLight )
	DEFINE_FIELD( m_angSunAngles, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecLight, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecAmbient, FIELD_VECTOR ),
	DEFINE_FIELD( m_bCascadedShadowMappingEnabled, FIELD_BOOLEAN ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST_NOBASE( CEnvLight, DT_CEnvLight )
	SendPropQAngles( SENDINFO( m_angSunAngles ) ),
	SendPropVector( SENDINFO( m_vecLight ) ),
	SendPropVector( SENDINFO( m_vecAmbient ) ),
	SendPropBool( SENDINFO( m_bCascadedShadowMappingEnabled ) ),
END_SEND_TABLE()

CEnvLight::CEnvLight() : m_bHasHDRLightSet( false ), m_bHasHDRAmbientSet( false )
{}

static Vector ConvertLightmapGammaToLinear( int *iColor4 )
{
	Vector vecColor;
	for ( int i = 0; i < 3; ++i )
	{
		vecColor[i] = powf( iColor4[i] / 255.0f, 2.2f );
	}
	vecColor *= iColor4[3] / 255.0f;
	return vecColor;
}

bool CEnvLight::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "pitch" ) )
	{
		m_angSunAngles.SetX( -atof( szValue ) );
	}
	else if ( FStrEq( szKeyName, "angles" ) )
	{
		Vector vecParsed;
		UTIL_StringToVector( vecParsed.Base(), szValue );
		m_angSunAngles.SetY( vecParsed.y );
	}
	else if ( FStrEq( szKeyName, "_light" ) || FStrEq( szKeyName, "_lightHDR" ) )
	{
		int iParsed[4];
		UTIL_StringToIntArray( iParsed, 4, szValue );

		if ( iParsed[0] <= 0 || iParsed[1] <= 0 || iParsed[2] <= 0 )
			return true;

		if ( FStrEq( szKeyName, "_lightHDR" ) )
		{
			// HDR overrides LDR
			m_bHasHDRLightSet = true;
		}
		else if ( m_bHasHDRLightSet )
		{
			// If this is LDR and we got HDR already, bail out.
			return true;
		}

		m_vecLight = ConvertLightmapGammaToLinear( iParsed );
		Msg( "Parsed light_environment light: %i %i %i %i\n",
			 iParsed[0], iParsed[1], iParsed[2], iParsed[3] );
	}
	else if ( FStrEq( szKeyName, "_ambient" ) || FStrEq( szKeyName, "_ambientHDR" ) )
	{
		int iParsed[4];
		UTIL_StringToIntArray( iParsed, 4, szValue );

		if ( iParsed[0] <= 0 || iParsed[1] <= 0 || iParsed[2] <= 0 )
			return true;

		if ( FStrEq( szKeyName, "_ambientHDR" ) )
		{
			// HDR overrides LDR
			m_bHasHDRLightSet = true;
		}
		else if ( m_bHasHDRLightSet )
		{
			// If this is LDR and we got HDR already, bail out.
			return true;
		}

		m_vecAmbient = ConvertLightmapGammaToLinear( iParsed );
		Msg( "Parsed light_environment ambient: %i %i %i %i\n",
			 iParsed[0], iParsed[1], iParsed[2], iParsed[3] );
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

void CEnvLight::Spawn()
{
	SetName( MAKE_STRING( "light_environment" ) );

	BaseClass::Spawn();

	m_bCascadedShadowMappingEnabled = HasSpawnFlags( 0x01 );
}