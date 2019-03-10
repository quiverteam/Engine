//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "vmouse.h"
#include "VFooterPanel.h"
#include "VDropDownMenu.h"
#include "VSliderControl.h"
#include "VHybridButton.h"
#include "EngineInterface.h"
#include "gameui_util.h"
#include "vgui/ISurface.h"
#include "VGenericConfirmation.h"
#include "materialsystem/materialsystem_config.h"
#include "cdll_util.h"
#include "nb_header_footer.h"
#include "../settings_old/optionssubmouse.h"
#include "vcontrolslistpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;

//=============================================================================
VMouse::VMouse(Panel *parent, const char *panelName):
BaseClass(parent, panelName)
{
	GameUI().PreventEngineHideGameUI();

	SetDeleteSelfOnClose(true);

	SetProportional( true );

	SetUpperGarnishEnabled(true);
	SetLowerGarnishEnabled(true);

	m_pHeaderFooter = new CNB_Header_Footer( this, "HeaderFooter" );
	m_pHeaderFooter->SetTitle( "" );
	m_pHeaderFooter->SetHeaderEnabled( false );
	m_pHeaderFooter->SetFooterEnabled( true );
	m_pHeaderFooter->SetGradientBarEnabled( true );
	m_pHeaderFooter->SetGradientBarPos( 60, 320 );

	m_pOptionsSubMouse = new COptionsSubMouse( this );
	m_pOptionsSubMouse->OnResetData();
}

//=============================================================================
VMouse::~VMouse()
{
	GameUI().AllowEngineHideGameUI();
}

//=============================================================================
void VMouse::Activate()
{
	BaseClass::Activate();

	UpdateFooter();
}

void VMouse::UpdateFooter()
{
	CBaseModFooterPanel *footer = BaseModUI::CBaseModPanel::GetSingleton().GetFooterPanel();
	if ( footer )
	{
		footer->SetButtons( FB_ABUTTON | FB_BBUTTON, FF_AB_ONLY, false );
		footer->SetButtonText( FB_ABUTTON, "#L4D360UI_Select" );
		footer->SetButtonText( FB_BBUTTON, "#L4D360UI_Controller_Done" );
	}
}

void VMouse::OnThink()
{
	BaseClass::OnThink();
}

void VMouse::OnKeyCodePressed(KeyCode code)
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
		// Ready to write that data... go ahead and nav back
		BaseClass::OnKeyCodePressed(code);
		break;

	default:
		BaseClass::OnKeyCodePressed(code);
		break;
	}
}

#ifndef _X360
void VMouse::OnKeyCodeTyped( vgui::KeyCode code )
{
	// For PC, this maps space bar to OK and esc to cancel
	switch ( code )
	{
	case KEY_SPACE:
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_A, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
		break;

	case KEY_ESCAPE:
		// close active menu if there is one, else navigate back
		if ( FlyoutMenu::GetActiveMenu() )
		{
			FlyoutMenu::CloseActiveMenu( FlyoutMenu::GetActiveMenu()->GetNavFrom() );
		}
		/*
		else if ( !m_pOptionsSubMouse->GetControlsList()->IsInEditMode() )
		{
			m_pOptionsSubMouse->OnApplyChanges();
			OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
		}
		*/
		break;
	}

	BaseClass::OnKeyTyped( code );
}
#endif

//=============================================================================
void VMouse::OnCommand(const char *command)
{
	if( Q_stricmp( "Back", command ) == 0 )
	{
		m_pOptionsSubMouse->OnApplyChanges();
		OnKeyCodePressed( KEY_XBUTTON_B );
	}
	else if( Q_stricmp( "Cancel", command ) == 0 )
	{
		OnKeyCodePressed( KEY_XBUTTON_B );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void VMouse::OnNotifyChildFocus( vgui::Panel* child )
{
}

void VMouse::OnFlyoutMenuClose( vgui::Panel* flyTo )
{
	UpdateFooter();
}

void VMouse::OnFlyoutMenuCancelled()
{
}

//=============================================================================
Panel* VMouse::NavigateBack()
{
	return BaseClass::NavigateBack();
}

void VMouse::PaintBackground()
{
	//BaseClass::DrawDialogBackground( "#L4D360UI_Cloud_Title", NULL, "#L4D360UI_Cloud_Subtitle", NULL, NULL, true );
}

void VMouse::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// required for new style
	SetPaintBackgroundEnabled( true );
	SetupAsDialogStyle();
}
