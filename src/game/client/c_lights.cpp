#include "cbase.h"
#include "c_lights.h"

#include "game_controls/FloatSlider.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/Frame.h"
#include "vgui/IInput.h"
#include "ienginevgui.h"

#include "tier0/memdbgon.h"

C_EnvLight *g_pCSMEnvLight = NULL;

static ConVar r_csm_enabled( "r_csm_enabled", "0", FCVAR_ARCHIVE, "0 = off, 1 = on, 2 = force" );

static ConVar r_csm_preset( "r_csm_preset", "0", FCVAR_ARCHIVE, "The Preset to use for CSM \n\n"
	"Near Cascade \n" "Far Cascade\n"
	"Othro Size, Forward Offset \n\n"
	
	"4 - softest shadows - very far distance \n"
	"2048, 1024 \n" "4096 4096 \n\n"

	"3 - soft shadows - far distance \n"
	"1024, 512 \n" "2048 2048 \n\n"

	"2 - sharp shadows - average distance \n"
	"512, 256 \n" "1024 1024 \n\n"

	"1 - sharpest shadows - lowest distance \n"
	"256, 128 \n" "512 512 \n" );

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_EnvLight, DT_CEnvLight, CEnvLight )
	RecvPropQAngles( RECVINFO( m_angSunAngles ) ),
	RecvPropVector( RECVINFO( m_vecLight ) ),
	RecvPropVector( RECVINFO( m_vecAmbient ) ),
	RecvPropBool( RECVINFO( m_bCascadedShadowMappingEnabled ) ),
END_RECV_TABLE()

C_EnvLight::C_EnvLight()
	: m_angSunAngles( vec3_angle )
	, m_vecLight( vec3_origin )
	, m_vecAmbient( vec3_origin )
	, m_bCascadedShadowMappingEnabled( false )
{
	// BIG OOF
	if ( r_csm_preset.GetInt() == 4 )
	{
		ShadowConfig_t def[] = {
			{ 2048.0f, 1024.0f, 0.25f, 0.0f },
			{ 4096.0f, 4096.0f, 0.75f, 0.0f }
		};

		V_memcpy( shadowConfigs, def, sizeof( shadowConfigs ) );
	}
	else if ( r_csm_preset.GetInt() == 3 )
	{
		ShadowConfig_t def[] = {
			{ 1024.0f, 512.0f, 0.25f, 0.0f },
			{ 2048.0f, 2048.0f, 0.75f, 0.0f }
		};

		V_memcpy( shadowConfigs, def, sizeof( shadowConfigs ) );
	}
	else if ( r_csm_preset.GetInt() == 2 )
	{
		ShadowConfig_t def[] = {
			{ 512.0f, 256.0f, 0.25f, 0.0f },
			{ 1024.0f, 1024.0f, 0.0f, 0.0f }
		};

		V_memcpy( shadowConfigs, def, sizeof( shadowConfigs ) );
	}
	else if ( r_csm_preset.GetInt() == 1 )
	{
		ShadowConfig_t def[] = {
			{ 256.0f, 128.0f, 0.25f, 0.0f },
			{ 512.0f, 512.0f, 0.0f, 0.0f }
		};

		V_memcpy( shadowConfigs, def, sizeof( shadowConfigs ) );
	}
	else // Default
	{
		ShadowConfig_t def[] = {
			{ 512.0f, 256.0f, 0.25f, 0.0f },
			{ 1024.0f, 1024.0f, 0.0f, 0.0f }
		};

		V_memcpy( shadowConfigs, def, sizeof( shadowConfigs ) );
	}

	// TODO: make this into presets you can use with a convar

	// softest shadows - very far distance
	//	{ 2048.0f, 1024.0f, 0.25f, 0.0f },
	//	{ 4096.0f, 4096.0f, 0.75f, 0.0f }

	// soft shadows - far distance
	//	{ 1024.0f, 512.0f, 0.25f, 0.0f },
	//	{ 2048.0f, 2048.0f, 0.75f, 0.0f }

	// sharp shadows - average distance
	//	{ 512.0f, 256.0f, 0.25f, 0.0f },
	//	{ 1024.0f, 1024.0f, 0.0f, 0.0f }

	// sharpest shadows - lowest distance
	//	{ 256.0f, 128.0f, 0.25f, 0.0f },
	//	{ 512.0f, 512.0f, 0.0f, 0.0f }
	//};

	//V_memcpy( shadowConfigs, def, sizeof( shadowConfigs ) );
}

C_EnvLight::~C_EnvLight()
{
	if ( g_pCSMEnvLight == this )
	{
		g_pCSMEnvLight = NULL;
	}
}

void C_EnvLight::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( g_pCSMEnvLight == NULL ||
		m_bCascadedShadowMappingEnabled && !g_pCSMEnvLight->m_bCascadedShadowMappingEnabled ||
		m_vecLight.Length() > g_pCSMEnvLight->m_vecLight.Length() ) // If there are multiple lights, use the brightest one if CSM is forced
	{
		g_pCSMEnvLight = this;
	}
}

bool C_EnvLight::IsCascadedShadowMappingEnabled() const
{
	const int &iCSMCvarEnabled = r_csm_enabled.GetInt();

	// bad workaround for when light_environment is not on a map, should change this to be more specific
	if ( this != NULL )
	{
		return m_bCascadedShadowMappingEnabled && iCSMCvarEnabled == 1 || iCSMCvarEnabled == 2;
	}
	else
	{
		return false;
	}
}


//-----------------------------------------------------------------------------
// CPrecisionSlider
// A drop-in replacement for the slider class that contains a text entry that
// can be used to read and set the current value.
// Also provides mousewheel support.   
//-----------------------------------------------------------------------------
class CPrecisionSlider : public vgui::FloatSlider
{
	DECLARE_CLASS_SIMPLE( CPrecisionSlider, vgui::FloatSlider );

public:
	CPrecisionSlider( Panel *parent, const char *panelName );
	~CPrecisionSlider();

	virtual void SetValue( float value, bool bTriggerChangeMessage = true );

	virtual void OnSizeChanged( int wide, int tall );

	virtual void GetTrackRect( int &x, int &y, int &w, int &h );

	virtual void SetEnabled( bool state );

protected:

	MESSAGE_FUNC_PARAMS( OnTextNewLine, "TextNewLine", data );

	virtual void OnMouseWheeled( int delta );

private:

	vgui::TextEntry	*m_pTextEntry;

	int				 m_nTextEntryWidth;
	int				 m_nSpacing;
};

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CPrecisionSlider::CPrecisionSlider( Panel *parent, const char *panelName ) : BaseClass( parent, panelName )
{
	m_pTextEntry = new vgui::TextEntry( this, "PrecisionEditPanel" );
	m_pTextEntry->SendNewLine( true );
	m_pTextEntry->SetCatchEnterKey( true );
	m_pTextEntry->AddActionSignalTarget( this );

	m_nTextEntryWidth = 32;
	m_nSpacing = 8;
	AddActionSignalTarget( parent );
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
CPrecisionSlider::~CPrecisionSlider()
{
}

//-----------------------------------------------------------------------------
// Override OnSizeChanged to update text entry size as well
//-----------------------------------------------------------------------------
void CPrecisionSlider::OnSizeChanged( int wide, int tall )
{
	m_pTextEntry->SetBounds( wide - ( m_nSpacing + m_nTextEntryWidth ) + m_nSpacing, 0, m_nTextEntryWidth, tall - 12 );

	BaseClass::OnSizeChanged( wide, tall );
}

//-----------------------------------------------------------------------------
// Override GetTrackRect in order to adjust for the text entry 
//-----------------------------------------------------------------------------
void CPrecisionSlider::GetTrackRect( int &x, int &y, int &w, int &h )
{
	int wide, tall;
	GetPaintSize( wide, tall );

	x = 0;
	y = 8;
	w = wide - ( _nobSize + m_nTextEntryWidth + m_nSpacing );
	h = 4;
}

//-----------------------------------------------------------------------------
// Override SetValue to update the text entry data
//-----------------------------------------------------------------------------
void CPrecisionSlider::SetValue( float value, bool bTriggerChangeMessage )
{
	BaseClass::SetValue( value, bTriggerChangeMessage );

	char szValueString[256];
	sprintf( szValueString, "%.3f", _value );
	m_pTextEntry->SetText( szValueString );
}

//-----------------------------------------------------------------------------
// Override SetEnabled to also effect the text entry field
//-----------------------------------------------------------------------------
void CPrecisionSlider::SetEnabled( bool state )
{
	BaseClass::SetEnabled( state );
	m_pTextEntry->SetEnabled( state );
}

//-----------------------------------------------------------------------------
// Handle updates from the text entry field
//-----------------------------------------------------------------------------
void CPrecisionSlider::OnTextNewLine( KeyValues *data )
{
	char buf[256];
	m_pTextEntry->GetText( buf, 256 );

	float value;
	sscanf( buf, "%f", &value );

	SetValue( value );
}

//-----------------------------------------------------------------------------
// Handle mousewheel updates
//-----------------------------------------------------------------------------
void CPrecisionSlider::OnMouseWheeled( int delta )
{
	BaseClass::OnMouseWheeled( delta );

	if ( IsEnabled() )
	{
		float value = GetValue();

		if ( vgui::input()->IsKeyDown( KEY_LCONTROL ) || vgui::input()->IsKeyDown( KEY_RCONTROL ) )
			SetValue( value + delta * 4 );
		else
			SetValue( value + delta );
	}
}

class CCSMTweakPanel : vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CCSMTweakPanel, vgui::Frame );
public:
	CCSMTweakPanel( vgui::VPANEL parent );
	~CCSMTweakPanel();

	void	OnMessage( const KeyValues *params, vgui::VPANEL fromPanel );

private:
	void LoadSettings();

	struct
	{
		CPrecisionSlider* pOrthoSize;
		CPrecisionSlider* pForwardOffset;
	//	CPrecisionSlider* pUVOffsetX;
	//	CPrecisionSlider* pViewDepthBiasHack;
	} near, far;
};

// remove this shitty hard coding when you move to asw gameui
CCSMTweakPanel::CCSMTweakPanel( vgui::VPANEL parent ): Frame( NULL, "CCSMTweakPanel" )
{
	SetParent( parent );
	near.pOrthoSize = new CPrecisionSlider( this, "nearOrthoSize" );
	near.pOrthoSize->SetRange( 0, 8192 );
	near.pForwardOffset = new CPrecisionSlider( this, "nearForwardOffset" );
	near.pForwardOffset->SetRange( 0, 8192 );
	// below is useless
	//near.pViewDepthBiasHack = new CPrecisionSlider( this, "nearDepthBiasHack" );
	//near.pViewDepthBiasHack->SetRange( 0, 1024 );

	far.pOrthoSize = new CPrecisionSlider( this, "farOrthoSize" );
	far.pOrthoSize->SetRange( 0, 8192 );
	far.pForwardOffset = new CPrecisionSlider( this, "farForwardOffset" );
	far.pForwardOffset->SetRange( 0, 8192 );
	// below is useless
	//far.pUVOffsetX = new CPrecisionSlider( this, "farForwardOffsetX" );
	//far.pUVOffsetX->SetRange( 0, 1024 );
	//far.pViewDepthBiasHack = new CPrecisionSlider( this, "farDepthBiasHack" );
	//far.pViewDepthBiasHack->SetRange( 0, 1024 );

	SetSize( 128, 256 );

	LoadControlSettings( "Resource\\CSMTweakPanel.res" );
	SetDeleteSelfOnClose( true );
	SetTitle( "CSM tweaks", true );
	SetVisible( true );
	MoveToFront();
	SetZPos( 1500 );
	SetMouseInputEnabled( true );
//	SetCursorAlwaysVisible( true );
	LoadSettings();
}

CCSMTweakPanel::~CCSMTweakPanel()
{
	//SetCursorAlwaysVisible( false );
}

void CCSMTweakPanel::OnMessage( const KeyValues *params, vgui::VPANEL fromPanel )
{
	BaseClass::OnMessage( params, fromPanel );

	if ( !g_pCSMEnvLight )
		return;

	if ( !Q_stricmp( "SliderMoved", params->GetName() ) )
	{
		Panel* panel = reinterpret_cast< Panel* >( const_cast< KeyValues* >( params )->GetPtr( "panel", NULL ) );
		if ( panel == near.pOrthoSize )
		{
			g_pCSMEnvLight->shadowConfigs[0].flOrthoSize = near.pOrthoSize->GetValue();
		}
		else if( panel == near.pForwardOffset )
		{
			g_pCSMEnvLight->shadowConfigs[0].flForwardOffset = near.pForwardOffset->GetValue();
		}
		/*else if ( panel == near.pUVOffsetX )
		{
			g_pCSMEnvLight->shadowConfigs[0].flUVOffsetX = near.pUVOffsetX->GetValue();
		}
		else if ( panel == near.pViewDepthBiasHack )
		{
			g_pCSMEnvLight->shadowConfigs[0].flViewDepthBiasHack = near.pViewDepthBiasHack->GetValue();
		}*/

		else if ( panel == far.pOrthoSize )
		{
			g_pCSMEnvLight->shadowConfigs[1].flOrthoSize = far.pOrthoSize->GetValue();
		}
		else if ( panel == far.pForwardOffset )
		{
			g_pCSMEnvLight->shadowConfigs[1].flForwardOffset = far.pForwardOffset->GetValue();
		}
		/*else if ( panel == far.pUVOffsetX )
		{
			g_pCSMEnvLight->shadowConfigs[1].flUVOffsetX = far.pUVOffsetX->GetValue();
		}
		else if ( panel == far.pViewDepthBiasHack )
		{
			g_pCSMEnvLight->shadowConfigs[1].flViewDepthBiasHack = far.pViewDepthBiasHack->GetValue();
		}*/
	}
}

void CCSMTweakPanel::LoadSettings()
{
	if (g_pCSMEnvLight)
	{
		const auto& data = g_pCSMEnvLight->shadowConfigs;
		near.pForwardOffset->SetValue(data[0].flForwardOffset, false);
		near.pOrthoSize->SetValue(data[0].flOrthoSize, false);
	//	near.pUVOffsetX->SetValue(data[0].flUVOffsetX, false);
	//	near.pViewDepthBiasHack->SetValue(data[0].flViewDepthBiasHack, false);

		far.pForwardOffset->SetValue(data[1].flForwardOffset, false);
		far.pOrthoSize->SetValue(data[1].flOrthoSize, false);
	//	far.pUVOffsetX->SetValue(data[1].flUVOffsetX, false);
	//	far.pViewDepthBiasHack->SetValue(data[1].flViewDepthBiasHack, false);
	}
}

CON_COMMAND( csm_tweak, "" )
{
	if (g_pCSMEnvLight)
	{
		new CCSMTweakPanel(enginevgui->GetPanel(PANEL_CLIENTDLL));
	}
}