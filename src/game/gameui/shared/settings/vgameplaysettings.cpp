//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "VGameplaySettings.h"
#include "VFooterPanel.h"
#include "VDropDownMenu.h"
#include "VSliderControl.h"
#include "VHybridButton.h"
#include "EngineInterface.h"
#include "gameui_util.h"
#include "vgui/ISurface.h"
#include "VGenericConfirmation.h"
#include "ivoicetweak.h"
#include "materialsystem/materialsystem_config.h"
#include "cdll_util.h"
#include "nb_header_footer.h"
#include "vgui_controls/ImagePanel.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;
using namespace BaseModUI;


int GetScreenAspectMode( int width, int height );
void SetFlyoutButtonText( const char *pchCommand, FlyoutMenu *pFlyout, const char *pchNewText );

extern ConVar ui_gameui_modal;

//=============================================================================
Gameplay::Gameplay(Panel *parent, const char *panelName):
BaseClass(parent, panelName)
{
	if ( ui_gameui_modal.GetBool() )
	{
		GameUI().PreventEngineHideGameUI();
	}

	m_pHeaderFooter = new CNB_Header_Footer( this, "HeaderFooter" );
	m_pHeaderFooter->SetTitle( "" );
	m_pHeaderFooter->SetHeaderEnabled( false );
	m_pHeaderFooter->SetFooterEnabled( true );
	m_pHeaderFooter->SetGradientBarEnabled( true );
	m_pHeaderFooter->SetGradientBarPos( 200, 140 );

	SetDeleteSelfOnClose(true);

	SetProportional( true );

	SetUpperGarnishEnabled(true);
	SetLowerGarnishEnabled(true);

	m_sldGameVolume = NULL;
	m_drpSkillLevel = NULL;

	m_btnCancel = NULL;
}

//=============================================================================
Gameplay::~Gameplay()
{
	GameUI().AllowEngineHideGameUI();

	UpdateFooter( false );
}

//=============================================================================
void Gameplay::Activate()
{
	BaseClass::Activate();

	if ( m_sldGameVolume )
	{
		m_sldGameVolume->Reset();
	}

	if ( m_drpSkillLevel )
	{
		CGameUIConVarRef skill("skill");

		switch ( skill.GetInt() )
		{
		case 2:
			m_drpSkillLevel->SetCurrentSelection( "#HL2PORTUI_Skills_Medium" );
			break;
		case 3:
			m_drpSkillLevel->SetCurrentSelection( "#HL2PORTUI_Skills_Hard" );
			break;
		case 1:
		default:
			m_drpSkillLevel->SetCurrentSelection( "#HL2PORTUI_Skills_Easy" );
			break;
		}

		FlyoutMenu *pFlyout = m_drpSkillLevel->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}
	UpdateFooter( true );

	if ( m_sldGameVolume )
	{
		if ( m_ActiveControl )
			m_ActiveControl->NavigateFrom( );
		m_sldGameVolume->NavigateTo();
		m_ActiveControl = m_sldGameVolume;
	}
}

void Gameplay::OnThink()
{
	BaseClass::OnThink();

	bool needsActivate = false;

	if( !m_sldGameVolume )
	{
		m_sldGameVolume = dynamic_cast< SliderControl* >( FindChildByName( "SldGameVolume" ) ); //what's this?! [str]
		needsActivate = true;
	}

	if( !m_drpSkillLevel )
	{
		m_drpSkillLevel = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpGameDifficulty" ) );
		needsActivate = true;
	}

	if( needsActivate )
	{
		Activate();
	}

}

void Gameplay::PerformLayout()
{
	BaseClass::PerformLayout();

	SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );
}

void Gameplay::OnKeyCodePressed(KeyCode code)
{
	int joystick = GetJoystickForCode( code );
	int userId = CBaseModPanel::GetSingleton().GetLastActiveUserId();
	if ( joystick != userId || joystick < 0 )
	{	
		return;
	}

	switch ( GetBaseButtonCode( code ) )
	{
	case KEY_XBUTTON_B:
		{
			// Ready to write that data... go ahead and nav back
			BaseClass::OnKeyCodePressed(code);
		}
		break;

	default:
		BaseClass::OnKeyCodePressed(code);
		break;
	}
}

//=============================================================================
void Gameplay::OnCommand(const char *command)
{
	if( Q_stricmp( "#HL2PORTUI_Skills_Hard", command ) == 0 )
	{
		// 3 is Hard
		CGameUIConVarRef skill( "skill" );
		skill.SetValue( 3 );
	}
	else if( Q_stricmp( "#HL2PORTUI_Skills_Medium", command ) == 0 )
	{
		// 2 is Medium
		CGameUIConVarRef skill( "skill" );
		skill.SetValue( 2 );
	}
	else if( Q_stricmp( "#HL2PORTUI_Skills_Easy", command ) == 0 )
	{
		// 1 is Easy
		CGameUIConVarRef skill( "skill" );
		skill.SetValue( 1 );
	}
	else if( Q_stricmp( "Back", command ) == 0 )
	{
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void Gameplay::OnNotifyChildFocus( vgui::Panel* child )
{
}

void Gameplay::UpdateFooter( bool bEnableCloud )
{
	if ( !BaseModUI::CBaseModPanel::GetSingletonPtr() )
		return;

	CBaseModFooterPanel *footer = BaseModUI::CBaseModPanel::GetSingleton().GetFooterPanel();
	if ( footer )
	{
		footer->SetButtons( FB_ABUTTON | FB_BBUTTON, FF_AB_ONLY, false );
		footer->SetButtonText( FB_ABUTTON, "#L4D360UI_Select" );
		footer->SetButtonText( FB_BBUTTON, "#L4D360UI_Controller_Done" );

		footer->SetShowCloud( bEnableCloud );
	}
}

void Gameplay::OnFlyoutMenuClose( vgui::Panel* flyTo )
{
	UpdateFooter( true );
}

void Gameplay::OnFlyoutMenuCancelled()
{
}

//=============================================================================
Panel* Gameplay::NavigateBack()
{
	engine->ClientCmd_Unrestricted( VarArgs( "host_writeconfig_ss %d", XBX_GetPrimaryUserId() ) );

	return BaseClass::NavigateBack();
}

void Gameplay::PaintBackground()
{
	//BaseClass::DrawDialogBackground( "#GameUI_Audio", NULL, "#L4D360UI_AudioVideo_Desc", NULL, NULL, true );
}

void Gameplay::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// required for new style
	SetPaintBackgroundEnabled( true );
	SetupAsDialogStyle();
}
